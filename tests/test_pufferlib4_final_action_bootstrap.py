"""Numerical regression for PufferLib 4's final sampled rollout action.

Run separately from the old package's test process with Python >=3.10. The
fixtures are transition-interface values, not Welfare Diplomacy rule examples.
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

import numpy as np
import torch


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from pufferlib4_runtime import (  # noqa: E402
    FINAL_ACTION_PATCH,
    REWARD_CLIPPING_PATCH,
    ROLLOUT_BOUNDARY_PATCH,
    isolated_runtime,
)


class DeterministicVec:
    """Two-agent stream whose Nth action produces rewards N and 10*N."""

    gpu = False
    total_agents = 2
    obs_size = 1
    obs_dtype = "FloatTensor"
    num_atns = 1
    act_sizes = [1]

    def __init__(self, terminal_steps=()):
        self.terminal_steps = frozenset(terminal_steps)
        self.observations = np.zeros((self.total_agents, self.obs_size), dtype=np.float32)
        self.rewards = np.zeros(self.total_agents, dtype=np.float32)
        self.terminals = np.zeros(self.total_agents, dtype=np.float32)
        self.obs_ptr = self.observations.ctypes.data
        self.rewards_ptr = self.rewards.ctypes.data
        self.terminals_ptr = self.terminals.ctypes.data
        self.steps = 0

    def reset(self):
        self.observations.fill(0)
        self.rewards.fill(0)
        self.terminals.fill(0)
        self.steps = 0

    def cpu_step(self, _actions_ptr):
        self.steps += 1
        self.observations.fill(self.steps)
        self.rewards[:] = (self.steps, 10 * self.steps)
        self.terminals.fill(float(self.steps in self.terminal_steps))

    def log(self):
        return {}

    def close(self):
        pass


class OraclePolicy(torch.nn.Module):
    """One legal action and V(s)=the scalar observation."""

    def __init__(self):
        super().__init__()
        self.logit = torch.nn.Parameter(torch.zeros(1, 1))

    def initial_state(self, _total_agents, device=None):
        return ()

    def forward_eval(self, observations, state):
        values = observations[:, 0].float() + 0 * self.logit.sum()
        return self.logit.expand(observations.shape[0], 1), values, state

    def forward(self, observations):
        flat = observations.reshape(-1, observations.shape[-1])
        logits = self.logit.expand(flat.shape[0], 1)
        values = flat[:, 0].float().reshape(observations.shape[:2]) + 0 * self.logit.sum()
        return logits, values


def oracle(values, rewards, terminals, gamma, gae_lambda):
    """GAE for H actions and H+1 state/transition rows, with unit ratios."""
    horizon = values.shape[1] - 1
    advantages = torch.zeros(values.shape[0], horizon)
    for row in range(values.shape[0]):
        tail = 0.0
        for step in range(horizon - 1, -1, -1):
            next_nonterminal = 1.0 - terminals[row, step + 1].item()
            delta = (
                rewards[row, step + 1].item()
                + gamma * values[row, step + 1].item() * next_nonterminal
                - values[row, step].item()
            )
            tail = delta + gamma * gae_lambda * tail * next_nonterminal
            advantages[row, step] = tail
    return advantages


class FinalActionBootstrapTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runtime_context = isolated_runtime(
            UPSTREAM,
            patches=[ROLLOUT_BOUNDARY_PATCH, REWARD_CLIPPING_PATCH, FINAL_ACTION_PATCH],
        )
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

    def args(self, horizon, gamma, gae_lambda):
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
        args["train"].update(
            horizon=horizon,
            minibatch_size=2 * horizon,
            total_timesteps=2 * horizon,
            replay_ratio=1.0,
            gamma=gamma,
            gae_lambda=gae_lambda,
            clip_rewards=False,
        )
        return args

    def trainer(self, horizon=3, gamma=0.5, gae_lambda=0.25, terminal_steps=()):
        vec = DeterministicVec(terminal_steps=terminal_steps)
        trainer = self.module.PuffeRL(
            self.args(horizon, gamma, gae_lambda),
            vec,
            OraclePolicy(),
            verbose=False,
        )
        return trainer

    def train_and_capture_advantage(self, trainer):
        calls = []
        original = self.module.compute_puff_advantage

        def observe(values, rewards, terminals, ratio, advantages, *parameters):
            result = original(values, rewards, terminals, ratio, advantages, *parameters)
            calls.append(tuple(tensor.detach().clone() for tensor in (
                values, rewards, terminals, ratio, result,
            )))
            return result

        with patch.object(self.module, "compute_puff_advantage", observe):
            trainer.train()
        self.assertEqual(len(calls), 1)
        return calls[0]

    def test_every_sampled_action_uses_its_own_reward(self):
        trainer = self.trainer(gamma=0.0, gae_lambda=0.0)
        try:
            trainer.rollouts()
            trainer.values.zero_()
            trainer.bootstrap_values.zero_()
            values, rewards, terminals, ratio, advantages = self.train_and_capture_advantage(trainer)
            expected_rewards = torch.tensor([[1.0, 2.0, 3.0], [10.0, 20.0, 30.0]])
            self.assertEqual(values.shape, (2, 4))
            torch.testing.assert_close(rewards[:, 1:], expected_rewards, rtol=0, atol=0)
            torch.testing.assert_close(advantages[:, :3], expected_rewards, rtol=0, atol=0)
            torch.testing.assert_close(trainer.actions, torch.zeros_like(trainer.actions))
            self.assertEqual(trainer.global_step, 6)
        finally:
            trainer.close()

    def test_gamma_lambda_oracle_covers_bootstrap_and_terminal_cutoff(self):
        for terminal_steps in ((), (3,)):
            with self.subTest(terminal_steps=terminal_steps):
                trainer = self.trainer(terminal_steps=terminal_steps)
                try:
                    trainer.rollouts()
                    values, rewards, terminals, ratio, advantages = self.train_and_capture_advantage(trainer)
                    self.assertEqual(values.shape, (2, 4))
                    torch.testing.assert_close(values[:, -1], torch.full((2,), 3.0))
                    torch.testing.assert_close(rewards[:, -1], torch.tensor([3.0, 30.0]))
                    torch.testing.assert_close(
                        terminals[:, -1],
                        torch.full((2,), float(bool(terminal_steps))),
                    )
                    expected = oracle(values, rewards, terminals, gamma=0.5, gae_lambda=0.25)
                    torch.testing.assert_close(advantages[:, :3], expected, rtol=1e-6, atol=1e-6)
                    self.assertNotEqual(advantages[0, -2].item(), 0.0)
                finally:
                    trainer.close()

    def collect_advantages(self, horizons):
        trainer = self.trainer(horizon=horizons[0], gamma=0.5, gae_lambda=0.0)
        rewards = []
        advantages = []
        actions = []
        try:
            for _ in horizons:
                trainer.rollouts()
                _, reward_input, _, _, advantage = self.train_and_capture_advantage(trainer)
                rewards.append(reward_input[:, 1:])
                advantages.append(advantage[:, :-1])
                actions.append(trainer.actions.transpose(0, 1).clone())
            return (
                torch.cat(rewards, dim=1),
                torch.cat(advantages, dim=1),
                torch.cat(actions, dim=1),
                trainer.global_step,
                trainer.state,
            )
        finally:
            trainer.close()

    def test_split_matches_uninterrupted_when_lambda_is_zero(self):
        split = self.collect_advantages((3, 3))
        uninterrupted = self.collect_advantages((6,))
        for split_tensor, uninterrupted_tensor in zip(split[:3], uninterrupted[:3]):
            torch.testing.assert_close(split_tensor, uninterrupted_tensor, rtol=0, atol=0)
        self.assertEqual(split[3], uninterrupted[3])
        self.assertEqual(split[3], 12)
        self.assertEqual(split[4], uninterrupted[4])
        self.assertEqual(split[4], ())

    def test_bootstrap_snapshots_do_not_alias_environment_buffers(self):
        trainer = self.trainer(terminal_steps=(3,))
        try:
            trainer.rollouts()
            snapshots = [tensor.clone() for tensor in (
                trainer.bootstrap_rewards, trainer.bootstrap_terminals,
                trainer.bootstrap_values,
            )]
            trainer.vec_rewards.fill_(999)
            trainer.vec_terminals.zero_()
            trainer.vec_obs.fill_(999)
            for actual, expected in zip((trainer.bootstrap_rewards,
                    trainer.bootstrap_terminals, trainer.bootstrap_values), snapshots):
                torch.testing.assert_close(actual, expected, rtol=0, atol=0)
        finally:
            trainer.close()

    def test_extra_value_query_does_not_advance_saved_state(self):
        trainer = self.trainer()
        trainer.state = (torch.zeros(2, 1),)
        original = trainer.policy.forward_eval
        queries = []

        def mutate_state(observations, state):
            self.assertFalse(torch.is_grad_enabled())
            state[0].add_(1)
            queries.append(state[0].clone())
            logits, values, _ = original(observations, state)
            return logits, values, state

        try:
            with patch.object(trainer.policy, "forward_eval", mutate_state):
                trainer.rollouts()
            self.assertEqual(len(queries), 4)
            torch.testing.assert_close(queries[-1], torch.full((2, 1), 4.0))
            torch.testing.assert_close(trainer.state[0], torch.full((2, 1), 3.0))
        finally:
            trainer.close()

    def test_one_action_rollout_keeps_its_final_transition(self):
        trainer = self.trainer(horizon=1, gamma=0.0, gae_lambda=0.0)
        try:
            trainer.rollouts()
            trainer.values.zero_()
            trainer.bootstrap_values.zero_()
            _, rewards, _, _, advantages = self.train_and_capture_advantage(trainer)
            torch.testing.assert_close(rewards[:, 1:], torch.tensor([[1.0], [10.0]]))
            torch.testing.assert_close(advantages[:, :1], rewards[:, 1:])
            self.assertEqual(trainer.global_step, 2)
        finally:
            trainer.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--upstream", type=Path, required=True)
    UPSTREAM = parser.parse_args().upstream.resolve()
    unittest.main(argv=[sys.argv[0]])
