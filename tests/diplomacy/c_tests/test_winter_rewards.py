"""Paper section 2.2: welfare earned after Winter is delivered that step.

These tests check reward timing, not map ownership or complete order encoding.
The only non-HOLD action is a Winter disband of unit zero (action 61).
"""
import numpy as np
import pytest

from native.pufferlib4 import DiplomacyVec
from pufferlib.ocean.diplomacy import Diplomacy, binding


def old_step(env, action):
    env.actions[:] = action
    obs, reward, done, _, _ = env.step(env.actions)
    return obs.copy(), reward.copy(), done.copy()


@pytest.mark.parametrize("max_years", [1, 2])
def test_old_binding_delivers_winter_delta_even_on_final_step(max_years):
    env = Diplomacy(max_years=max_years)
    try:
        env.reset()
        for _ in range(2):
            assert not old_step(env, 0)[1].any()
        assert binding.query_game_state(env.env_handle)["phase"] == 4
        _, reward, done = old_step(env, 61)
        state = binding.query_game_state(env.env_handle)
        assert [p["welfare_points"] for p in state["powers"]] == [1] * 7
        np.testing.assert_array_equal(reward, np.ones(7))
        assert bool(done.all()) == (max_years == 1)
        if max_years == 2:
            for _ in range(2):
                assert not old_step(env, 0)[1].any()
            _, reward, done = old_step(env, 0)
            np.testing.assert_array_equal(reward, np.ones(7))
            assert done.all()
            assert [p["welfare_points"] for p in binding.query_game_state(env.env_handle)["powers"]] == [2] * 7
        env.reset()
        for _ in range(3):
            assert not old_step(env, 0)[1].any()
    finally:
        env.close()


@pytest.mark.parametrize("max_years", [1, 2])
def test_native_adapter_preserves_winter_delta_through_autoreset(max_years):
    with DiplomacyVec(max_years=max_years) as env:
        initial = env.reset()
        for _ in range(2):
            assert not env.step(np.zeros(7))[1].any()
        obs, reward, done = env.step(np.full(7, 61))
        np.testing.assert_array_equal(reward, np.ones(7))
        assert bool(done.all()) == (max_years == 1)
        if max_years == 2:
            for _ in range(2):
                assert not env.step(np.zeros(7))[1].any()
            obs, reward, done = env.step(np.zeros(7))
            np.testing.assert_array_equal(reward, np.ones(7))
            assert done.all()
        np.testing.assert_array_equal(obs, initial)
        # The next game starts with fresh welfare bookkeeping and units.
        for _ in range(3):
            assert not env.step(np.zeros(7))[1].any()


def test_standard_mode_has_no_welfare_reward():
    old = Diplomacy(welfare_mode=False, max_years=1)
    try:
        old.reset()
        with DiplomacyVec(welfare_mode=False, max_years=1) as new:
            for action in [0, 0, 61]:
                old_result = old_step(old, action)
                new_result = new.step(np.full(7, action))
                assert not old_result[1].any()
                assert not new_result[1].any()
                np.testing.assert_array_equal(old_result[2], new_result[2])
            assert [p["welfare_points"] for p in binding.query_game_state(old.env_handle)["powers"]] == [0] * 7
    finally:
        old.close()
