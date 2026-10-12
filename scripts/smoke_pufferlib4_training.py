#!/usr/bin/env python3
"""Check the pinned PufferLib 4 CPU trainer with explicit local patches.

Run as a separate process: the old package and upstream package share a name.
The explicit Torch patch preserves welfare reward magnitudes. Actions and
final-action bootstrapping remain incomplete; this is not a useful baseline.
"""
import argparse
import ast
import configparser
import importlib.util
import json
from pathlib import Path
import sys
import sysconfig
import tempfile

from pufferlib4_runtime import PIN, REWARD_CLIPPING_PATCH, ROLLOUT_BOUNDARY_PATCH, isolated_runtime


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, required=True)
    cli = parser.parse_args()
    if sys.version_info < (3, 10):
        parser.error("the upstream Python trainer requires Python >=3.10")
    upstream = cli.upstream.resolve()
    runtime_context = isolated_runtime(
        upstream, patches=[ROLLOUT_BOUNDARY_PATCH, REWARD_CLIPPING_PATCH],
    )
    runtime = runtime_context.__enter__()
    sys.path.insert(0, str(runtime))
    import pufferlib
    if Path(pufferlib.__file__).resolve() != runtime / "pufferlib/__init__.py":
        raise RuntimeError("imported outside the isolated pinned PufferLib 4 runtime")
    extension = ROOT / "build/pufferlib4" / ("_C" + sysconfig.get_config_var("EXT_SUFFIX"))
    spec = importlib.util.spec_from_file_location("pufferlib._C", extension)
    backend = importlib.util.module_from_spec(spec)
    sys.modules["pufferlib._C"] = backend
    spec.loader.exec_module(backend)
    pufferlib._C = backend
    import torch
    from pufferlib.torch_pufferl import PuffeRL, load_policy

    class WinterOnlySmokePolicy(torch.nn.Module):
        """A deliberately small policy: HOLD, or unit-zero DISBAND in Winter.

        Mask both evaluation and training identically. Do not exploit the known
        engine bug allowing voluntary disbands during movement/retreat phases.
        This exercises the upstream Torch policy hook, not a new trainer.
        """
        def __init__(self, base):
            super().__init__()
            self.base = base

        def initial_state(self, *args, **kwargs):
            return self.base.initial_state(*args, **kwargs)

        def mask(self, observations, logits):
            flat = observations.reshape(-1, observations.shape[-1])
            allowed = torch.zeros_like(logits, dtype=torch.bool)
            allowed[:, 0] = True
            allowed[:, 61] = flat[:, 1624] == 1  # phase one-hot: Winter
            return logits.masked_fill(~allowed, torch.finfo(logits.dtype).min)

        def forward_eval(self, observations, state):
            logits, value, state = self.base.forward_eval(observations, state)
            return self.mask(observations, logits), value, state

        def forward(self, observations):
            logits, value = self.base(observations)
            return self.mask(observations, logits), value

    torch.set_num_threads(1)
    torch.manual_seed(42)
    cfg = configparser.ConfigParser()
    cfg.read(runtime / "config/default.ini")
    args = {}
    for section in cfg.sections():
        values = {}
        for key, value in cfg[section].items():
            try:
                values[key] = ast.literal_eval(value)
            except (SyntaxError, ValueError):
                values[key] = value
        if section == "base":
            args.update(values)
        else:
            args[section] = values
    args.update(env_name="diplomacy", wandb=False, load_id=None, load_model_path=None)
    args["vec"].update(total_agents=14, num_buffers=1, num_threads=1)
    args["env"].update(max_years=2, welfare_mode=1)
    args["policy"].update(hidden_size=32, num_layers=1)
    args["torch"]["network"] = "MLP"
    args["train"].update(horizon=12, minibatch_size=168, total_timesteps=336,
                         replay_ratio=1.0, learning_rate=0.001, clip_rewards=False)
    vec = backend.create_vec(args, 0)
    trainer = None
    try:
        policy = WinterOnlySmokePolicy(load_policy(args, vec))
        # Make rare disband actions observable in a tiny smoke run. This is an
        # explicit initialization bias, not a trained or benchmark policy.
        with torch.no_grad():
            policy.base.decoder.decoder.bias[61] = 2
        trainer = PuffeRL(args, vec, policy, verbose=False)
        initial = {name: value.detach().clone() for name, value in policy.named_parameters()}
        reward_total = 0.0
        outside_unit_range_count = 0
        batch_boundaries = []
        epochs = []
        for _ in range(2):
            pending_rewards = trainer.vec_rewards.clone()
            pending_terminals = trainer.vec_terminals.clone()
            trainer.rollouts()
            first_rewards = trainer.rewards[0]
            first_terminals = trainer.terminals[0]
            if not torch.equal(first_rewards, pending_rewards):
                raise RuntimeError("rollout dropped pending rewards at its first row")
            if not torch.equal(first_terminals, pending_terminals):
                raise RuntimeError("rollout dropped pending terminals at its first row")
            batch_boundaries.append({
                "pending_reward_sum": pending_rewards.sum().item(),
                "stored_first_row_reward_sum": first_rewards.sum().item(),
                "pending_terminal_count": pending_terminals.sum().item(),
                "stored_first_row_terminal_count": first_terminals.sum().item(),
                "handoff_matches": True,
            })
            is_winter = trainer.observations[:, :, 1624] == 1
            rollout_actions = trainer.actions[:, :, 0]
            if not torch.all((rollout_actions == 0) | ((rollout_actions == 61) & is_winter)):
                raise RuntimeError("smoke policy used an action outside its stated restriction")
            reward_total += trainer.rewards.sum().item()
            outside_unit_range_count += (trainer.rewards.abs() > 1).sum().item()
            trainer.train()
            if not all(torch.isfinite(torch.tensor(value)).item() for value in trainer.losses.values()):
                raise RuntimeError(f"non-finite losses: {trainer.losses}")
            epochs.append(dict(trainer.losses))
        if reward_total <= 0:
            raise RuntimeError("the real trainer received no positive welfare reward")
        changed = sum(not torch.equal(initial[name], value) for name, value in policy.named_parameters())
        if changed == 0:
            raise RuntimeError("training did not change policy parameters")
        obs = trainer.vec_obs.clone()
        state = policy.initial_state(vec.total_agents, device="cpu")
        with torch.no_grad():
            expected = policy.forward_eval(obs, state)[:2]
        with tempfile.TemporaryDirectory(prefix="pufferlib4-checkpoint-") as scratch:
            path = str(Path(scratch) / "weights.pt")
            trainer.save_weights(path)
            with torch.no_grad():
                for parameter in policy.parameters():
                    parameter.add_(1)
            trainer.load_weights(path)
            with torch.no_grad():
                actual = policy.forward_eval(obs, state)[:2]
            for before, after in zip(expected, actual):
                torch.testing.assert_close(before, after, rtol=0, atol=0)
        print(json.dumps({
            "upstream_commit": PIN, "python": sys.version.split()[0],
            "torch": torch.__version__, "network": "MLP", "device": trainer.device,
            "runtime_patches": [ROLLOUT_BOUNDARY_PATCH.name, REWARD_CLIPPING_PATCH.name],
            "steps": trainer.global_step, "stored_rollout_reward_sum": reward_total,
            "batch_boundaries": batch_boundaries,
            "final_pending_reward_sum": trainer.vec_rewards.sum().item(),
            "final_pending_terminal_count": trainer.vec_terminals.sum().item(),
            "stored_reward_entries_outside_unit_range": outside_unit_range_count,
            "clip_rewards": trainer.clip_rewards,
            "changed_parameter_tensors": changed, "checkpoint_prediction_match": True,
            "config": {key: args[key] for key in ["vec", "env", "policy", "torch", "train"]},
            "losses": epochs,
            "limitations": ["restricted HOLD-or-Winter-DISBAND policy with initial bias, not a learned baseline",
                "incomplete actions and existing game defects",
                "native GPU reward clipping is unchanged and untested",
                "the final environment transition remains pending until the next rollout",
                "the upstream advantage update does not consume a carried first-row reward",
                "no recurrent, GPU, opponent-pool or full training-resume check"],
        }, indent=2))
    finally:
        try:
            if trainer is not None:
                trainer.close()
            else:
                vec.close()
        finally:
            runtime_context.__exit__(None, None, None)


if __name__ == "__main__":
    main()
