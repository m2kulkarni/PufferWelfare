"""Observe the real Torch trainer's input to the compiled CPU advantage function.

Run separately from the old package's test process, with Python >=3.10.
Synthetic rewards identify each of seven powers; these are not game-rule fixtures.
"""
import argparse
import ast
import configparser
import importlib.util
from pathlib import Path
import sys
import sysconfig
import unittest
from unittest.mock import patch

import torch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from pufferlib4_runtime import FINAL_ACTION_PATCH, REWARD_CLIPPING_PATCH, ROLLOUT_BOUNDARY_PATCH, isolated_runtime


class RewardClippingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runtime_context = isolated_runtime(
            UPSTREAM, patches=[ROLLOUT_BOUNDARY_PATCH, REWARD_CLIPPING_PATCH, FINAL_ACTION_PATCH])
        cls.runtime = cls.runtime_context.__enter__()
        sys.path.insert(0, str(cls.runtime))
        import pufferlib
        extension = ROOT / "build/pufferlib4" / ("_C" + sysconfig.get_config_var("EXT_SUFFIX"))
        spec = importlib.util.spec_from_file_location("pufferlib._C", extension)
        cls.backend = importlib.util.module_from_spec(spec)
        sys.modules["pufferlib._C"] = cls.backend
        spec.loader.exec_module(cls.backend)
        pufferlib._C = cls.backend
        import pufferlib.torch_pufferl
        cls.module = pufferlib.torch_pufferl
        torch.set_num_threads(1)

    @classmethod
    def tearDownClass(cls):
        cls.runtime_context.__exit__(None, None, None)

    def args(self, setting):
        config = configparser.ConfigParser()
        config.read(self.runtime / "config/default.ini")
        args = {}
        for section in config.sections():
            values = {}
            for key, value in config[section].items():
                try:
                    values[key] = ast.literal_eval(value)
                except (SyntaxError, ValueError):
                    values[key] = value
            if section == "base":
                args.update(values)
            else:
                args[section] = values
        args.update(wandb=False, load_id=None, load_model_path=None)
        args["vec"].update(total_agents=7, num_buffers=1, num_threads=1)
        args["env"].update(max_years=2, welfare_mode=1)
        args["policy"].update(hidden_size=8, num_layers=1)
        args["torch"]["network"] = "MLP"
        args["train"].update(horizon=4, minibatch_size=28, total_timesteps=28,
                             replay_ratio=1.0, gamma=0.0, gae_lambda=0.0)
        if setting != "missing":
            args["train"]["clip_rewards"] = setting
        return args

    def observe_update(self, setting):
        torch.manual_seed(42)
        args = self.args(setting)
        vec = self.backend.create_vec(args, 0)
        trainer = None
        try:
            policy = self.module.load_policy(args, vec)
            trainer = self.module.PuffeRL(args, vec, policy, verbose=False)
            trainer.observations[:] = trainer.vec_obs
            # Different magnitudes retain power identity. Negative sentinels
            # also check the default lower clipping bound, not welfare scoring.
            rewards = torch.arange(1, 8, dtype=torch.float32).repeat(4, 1)
            rewards[1] *= 2
            rewards[2] *= -3
            rewards[3] *= 4
            trainer.rewards.copy_(rewards)
            trainer.values.zero_()
            trainer.terminals.fill_(1)
            # The extra row contains the final action's transition, not a
            # fifth sampled action. Check its clipping bounds explicitly too.
            final_reward = -5 * torch.arange(1, 8, dtype=torch.float32)
            trainer.bootstrap_rewards.copy_(final_reward)
            trainer.bootstrap_values.zero_()
            trainer.bootstrap_terminals.fill_(1)
            trainer.actions.zero_()
            with torch.no_grad():
                logits, _ = policy(trainer.observations.transpose(0, 1).contiguous())
                _, logprob, _ = self.module.sample_logits(
                    logits, action=trainer.actions.transpose(0, 1).contiguous())
                trainer.logprobs.copy_(logprob.reshape(7, 4).T)
            observations = []
            compiled_calls = []
            original_compute = self.module.compute_puff_advantage
            original_compiled = self.backend.puff_advantage_cpu

            def compiled(*arguments):
                compiled_calls.append(True)
                return original_compiled(*arguments)

            def compute(values, reward_input, *arguments):
                result = original_compute(values, reward_input, *arguments)
                observations.append((reward_input.clone(), result.clone()))
                return result

            with patch.object(self.module, "compute_puff_advantage", compute), \
                    patch.object(self.backend, "puff_advantage_cpu", compiled):
                trainer.train()
            self.assertEqual(len(observations), 1)
            self.assertEqual(len(compiled_calls), 1)
            torch.testing.assert_close(trainer.rewards, rewards, rtol=0, atol=0)
            torch.testing.assert_close(trainer.bootstrap_rewards, final_reward, rtol=0, atol=0)
            raw = torch.cat((rewards.T, final_reward[:, None]), dim=1)
            return raw, observations[0]
        finally:
            if trainer is not None:
                trainer.close()
            else:
                vec.close()

    def test_disabled_preserves_per_power_magnitude_at_real_advantage_input(self):
        raw, (actual, advantages) = self.observe_update(False)
        torch.testing.assert_close(actual, raw, rtol=0, atol=0)
        expected = torch.zeros_like(raw)
        expected[:, :-1] = raw[:, 1:]
        # Four actions now consume four following transition rows. Only the
        # extra bootstrap row (which has no action) has zero advantage.
        torch.testing.assert_close(advantages, expected, rtol=0, atol=0)

    def test_missing_or_enabled_preserves_existing_clipping(self):
        for setting in ("missing", True):
            with self.subTest(setting=setting):
                raw, (actual, advantages) = self.observe_update(setting)
                torch.testing.assert_close(actual, raw.clamp(-1, 1), rtol=0, atol=0)
                expected = torch.zeros_like(raw)
                expected[:, :-1] = raw[:, 1:].clamp(-1, 1)
                torch.testing.assert_close(advantages, expected, rtol=0, atol=0)

    def test_rejects_non_boolean_settings(self):
        for setting in (0, 1, "False", None, 1.0):
            with self.subTest(setting=setting):
                args = self.args(setting)
                with self.assertRaisesRegex(TypeError, "clip_rewards must be a boolean"):
                    self.module.PuffeRL(args, None, None, verbose=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--upstream", type=Path, required=True)
    UPSTREAM = parser.parse_args().upstream.resolve()
    unittest.main(argv=[sys.argv[0]])
