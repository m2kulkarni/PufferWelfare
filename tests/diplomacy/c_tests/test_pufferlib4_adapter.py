"""Exercise actual pinned 4.0 vecenv + bindings_cpu, not a mock interface.

Build both extensions first. These tests do not establish correct welfare
rewards: the first port deliberately preserves the old engine's reward bug.
"""
import numpy as np
import pytest

from native.pufferlib4 import DiplomacyVec, _load_backend
from pufferlib.ocean.diplomacy import Diplomacy


def test_metadata():
    backend = _load_backend()
    assert backend.env_name == "diplomacy"
    assert backend.gpu == 0
    assert backend.precision_bytes == 4
    with DiplomacyVec() as env:
        assert env._vec.obs_size == 1641
        assert env._vec.obs_dtype == "FloatTensor"
        assert env._vec.obs_elem_size == 4
        assert env._vec.act_sizes == [1088]
        assert env._vec.num_atns == 1


@pytest.mark.parametrize("welfare_mode", [False, True])
def test_matches_old_engine_before_terminal(welfare_mode):
    old = Diplomacy(welfare_mode=welfare_mode, max_years=3)
    try:
        with DiplomacyVec(welfare_mode=welfare_mode, max_years=3) as new:
            np.testing.assert_array_equal(new.reset(), old.reset()[0])
            rng = np.random.default_rng(123)
            for _ in range(6):
                actions = rng.integers(0, 1088, size=7, dtype=np.int32)
                # The old wrapper ignores step(actions); write its buffer explicitly.
                old.actions[:] = actions
                old_obs, old_rewards, old_dones, _, _ = old.step(actions)
                obs, rewards, dones = new.step(actions)
                assert not old_dones.any()
                np.testing.assert_array_equal(obs, old_obs)
                np.testing.assert_array_equal(rewards, old_rewards)
                np.testing.assert_array_equal(dones, old_dones.astype(np.float32))
    finally:
        old.close()


def test_parallel_games_are_isolated():
    actions = np.arange(21, dtype=np.float32)
    with DiplomacyVec(total_agents=21) as many:
        singles = [DiplomacyVec() for _ in range(3)]
        try:
            for _ in range(5):
                combined = many.step(actions)
                individual = [env.step(actions[7*i:7*(i+1)]) for i, env in enumerate(singles)]
                for channel in range(3):
                    np.testing.assert_array_equal(combined[channel], np.concatenate([step[channel] for step in individual]))
        finally:
            for env in singles:
                env.close()


def test_terminal_autoreset_and_log():
    old = Diplomacy(max_years=1)
    try:
        old.reset()
        with DiplomacyVec(max_years=1) as env:
            initial = env.reset()
            for _ in range(10):
                old.actions[:] = 0
                _, expected_reward, expected_done, _, _ = old.step(old.actions)
                obs, reward, done = env.step(np.zeros(7))
                np.testing.assert_array_equal(done, expected_done.astype(np.float32))
                np.testing.assert_array_equal(reward, expected_reward)
                if done.all():
                    break
            else:
                pytest.fail("one-year game did not terminate")
            np.testing.assert_array_equal(obs, initial)
            log = env.log()
            assert log["n"] == 1
            assert log["avg_centers"] == pytest.approx(22 / 7)
            assert log["avg_units"] == pytest.approx(22 / 7)
            assert env.log() == {}
            _, reward, done = env.step(np.zeros(7))
            assert not done.any()
            assert not reward.any()
    finally:
        old.close()


def test_repeated_episodes_log_counts_and_reset():
    with DiplomacyVec(total_agents=14, max_years=1) as env:
        initial = env.reset()
        completed = 0
        for _ in range(12):
            _, _, done = env.step(np.zeros(14))
            if done.all():
                completed += 2
        assert completed >= 4
        assert env.log()["n"] == completed
        np.testing.assert_array_equal(env.reset(), initial)
        assert not env.step(np.zeros(14))[2].any()


@pytest.mark.parametrize("kwargs", [
    {"total_agents": 0}, {"total_agents": 8}, {"total_agents": -7},
    {"total_agents": 7.0}, {"total_agents": True}, {"num_buffers": 2},
    {"num_buffers": 0}, {"num_buffers": True}, {"max_years": 0},
    {"max_years": 1001}, {"max_years": 1.5}, {"welfare_mode": "false"},
])
def test_rejects_unsafe_layout_and_config(kwargs):
    with pytest.raises(ValueError):
        DiplomacyVec(**kwargs)


@pytest.mark.parametrize("actions", [
    np.zeros(6), np.zeros((7, 1)), ["hold"] * 7,
    [np.nan] * 7, [np.inf] * 7, [-1] * 7, [1088] * 7,
    [0.5] * 7, [1j] * 7,
])
def test_rejects_invalid_actions_without_advancing(actions):
    with DiplomacyVec() as env:
        initial = env.reset()
        with pytest.raises(ValueError):
            env.step(actions)
        np.testing.assert_array_equal(env.reset(), initial)


def test_results_survive_close_and_close_is_idempotent():
    env = DiplomacyVec()
    obs, reward, done = env.step(np.zeros(7))
    expected = obs.copy()
    env.close()
    env.close()
    np.testing.assert_array_equal(obs, expected)
    assert reward.shape == done.shape == (7,)
    for call in [env.reset, env.log, lambda: env.step(np.zeros(7))]:
        with pytest.raises(RuntimeError, match="closed"):
            call()
