#!/usr/bin/env python3
"""Check the real pinned PufferLib 4 CPU trainer, not learning performance.

Run as a separate process: the old package and upstream package share a name.
The upstream trainer clips rewards and does not mask our incomplete actions.
Those limitations must be resolved before claiming a useful welfare baseline.
"""
import argparse
import ast
import configparser
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import sysconfig
import tempfile

PIN = "42f70d6932c30ac977736f861006809c50168ba9"
ROOT = Path(__file__).resolve().parents[1]
FILES = ["pufferlib/__init__.py", "pufferlib/pufferl.py", "pufferlib/selfplay.py",
         "pufferlib/torch_pufferl.py", "pufferlib/models.py", "pufferlib/muon.py",
         "config/default.ini"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, required=True)
    cli = parser.parse_args()
    if sys.version_info < (3, 10):
        parser.error("the upstream Python trainer requires Python >=3.10")
    upstream = cli.upstream.resolve()
    for name in FILES:
        pinned = subprocess.check_output(["git", "-C", str(upstream), "show", f"{PIN}:{name}"])
        if (upstream / name).read_bytes() != pinned:
            parser.error(f"{name} differs from the pinned upstream source")
    sys.path.insert(0, str(upstream))
    import pufferlib
    if Path(pufferlib.__file__).resolve() != upstream / "pufferlib/__init__.py":
        raise RuntimeError("imported the old package instead of pinned PufferLib 4")
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
    cfg.read(upstream / "config/default.ini")
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
                         replay_ratio=1.0, learning_rate=0.001)
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
        clipped_reward_count = 0
        epochs = []
        for _ in range(2):
            trainer.rollouts()
            is_winter = trainer.observations[:, :, 1624] == 1
            rollout_actions = trainer.actions[:, :, 0]
            if not torch.all((rollout_actions == 0) | ((rollout_actions == 61) & is_winter)):
                raise RuntimeError("smoke policy used an action outside its stated restriction")
            reward_total += trainer.rewards.sum().item()
            clipped_reward_count += (trainer.rewards.abs() > 1).sum().item()
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
            "steps": trainer.global_step, "reward_sum": reward_total,
            "reward_entries_clipped_by_upstream_trainer": clipped_reward_count,
            "changed_parameter_tensors": changed, "checkpoint_prediction_match": True,
            "config": {key: args[key] for key in ["vec", "env", "policy", "torch", "train"]},
            "losses": epochs,
            "limitations": ["restricted HOLD-or-Winter-DISBAND policy with initial bias, not a learned baseline",
                "incomplete actions and existing game defects", "upstream reward clipping",
                "no recurrent, GPU, opponent-pool or full training-resume check"],
        }, indent=2))
    finally:
        if trainer is not None:
            trainer.close()
        else:
            vec.close()


if __name__ == "__main__":
    main()
