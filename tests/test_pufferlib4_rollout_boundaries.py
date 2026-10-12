"""Regression for rewards and terminals at Torch rollout batch boundaries."""
import argparse
from pathlib import Path
import sys
import types
import unittest

import numpy as np
import torch


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from pufferlib4_runtime import ROLLOUT_BOUNDARY_PATCH, isolated_runtime  # noqa: E402


class DeterministicVec:
    """Small pointer-backed vector stream with transitions 1 through 6."""

    gpu = False
    total_agents = 2
    obs_size = 1
    obs_dtype = "FloatTensor"
    num_atns = 1
    act_sizes = [1]

    def __init__(self):
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
        self.terminals.fill(float(self.steps in (3, 6)))

    def log(self):
        return {}

    def close(self):
        pass


class ConstantPolicy(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.bias = torch.nn.Parameter(torch.zeros(1))

    def initial_state(self, _total_agents, device=None):
        return ()

    def forward_eval(self, observations, state):
        batch = observations.shape[0]
        logits = self.bias.expand(batch, 1)
        values = self.bias.expand(batch)
        return logits, values, state

    def forward(self, observations):
        logits, values, _ = self.forward_eval(observations, ())
        return logits, values


def trainer_args(horizon):
    return {
        "world_size": 1,
        "train": {
            "horizon": horizon,
            "minibatch_size": 2 * horizon,
            "total_timesteps": 2 * horizon,
            "learning_rate": 0.001,
            "beta1": 0.9,
            "eps": 1e-8,
        },
    }


def collect(PuffeRL, horizons):
    vec = DeterministicVec()
    trainer = PuffeRL(trainer_args(horizons[0]), vec, ConstantPolicy(), verbose=False)
    rewards = []
    terminals = []
    try:
        for _ in horizons:
            trainer.rollouts()
            rewards.append(trainer.rewards.clone())
            terminals.append(trainer.terminals.clone())
        return (
            torch.cat(rewards), torch.cat(terminals),
            trainer.vec_rewards.clone(), trainer.vec_terminals.clone(),
        )
    finally:
        trainer.close()


class RolloutBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runtime = isolated_runtime(UPSTREAM, patches=[ROLLOUT_BOUNDARY_PATCH])
        runtime = cls.runtime.__enter__()
        sys.path.insert(0, str(runtime))
        import pufferlib
        backend = types.ModuleType("pufferlib._C")
        backend.precision_bytes = 4
        backend.gpu = 0
        backend.get_utilization = lambda _device: {}
        pufferlib._C = backend
        sys.modules["pufferlib._C"] = backend
        from pufferlib.torch_pufferl import PuffeRL
        cls.batched = collect(PuffeRL, (3, 3))
        cls.uninterrupted = collect(PuffeRL, (6,))

    @classmethod
    def tearDownClass(cls):
        cls.runtime.__exit__(None, None, None)

    def test_reward_stream_matches_uninterrupted_rollout(self):
        torch.testing.assert_close(self.batched[0], self.uninterrupted[0])
        torch.testing.assert_close(self.batched[0][3], torch.tensor([3.0, 30.0]))

    def test_terminal_stream_matches_uninterrupted_rollout(self):
        torch.testing.assert_close(self.batched[1], self.uninterrupted[1])
        torch.testing.assert_close(self.batched[1][3], torch.ones(2))

    def test_initial_reset_and_final_transition_remain_well_defined(self):
        torch.testing.assert_close(self.batched[0][0], torch.zeros(2))
        torch.testing.assert_close(self.batched[1][0], torch.zeros(2))
        torch.testing.assert_close(self.batched[2], torch.tensor([6.0, 60.0]))
        torch.testing.assert_close(self.batched[3], torch.ones(2))
        torch.testing.assert_close(self.batched[2], self.uninterrupted[2])
        torch.testing.assert_close(self.batched[3], self.uninterrupted[3])
        self.assertFalse(torch.any(torch.all(self.batched[0] == self.batched[2], dim=1)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--upstream", type=Path, required=True)
    args = parser.parse_args()
    UPSTREAM = args.upstream.resolve()
    unittest.main(argv=[sys.argv[0]])
