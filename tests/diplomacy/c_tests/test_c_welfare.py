"""Test C welfare calculations.

This module tests that welfare points are correctly calculated
in welfare diplomacy mode.
"""

import pytest
from pufferlib.ocean.diplomacy import Diplomacy, binding


class TestCWelfare:
    """Tests for C welfare calculations."""

    def test_welfare_mode_enabled(self):
        """Test that welfare mode can be enabled."""
        env = Diplomacy(welfare_mode=True)
        assert env is not None

    def test_welfare_mode_disabled(self):
        """Test that standard diplomacy mode works."""
        env = Diplomacy(welfare_mode=False)
        assert env is not None

    def test_initial_welfare_points(self):
        """Test that all powers start with 0 welfare points."""
        env = Diplomacy(welfare_mode=True)
        state = binding.query_game_state(env.env_handle)
        powers = state["powers"]

        # All powers should start with 0 welfare points
        for power in powers:
            assert power["welfare_points"] == 0

    def test_welfare_calculation_basic(self):
        from pufferlib.ocean.diplomacy import Game
        with Game(max_years=2) as game:
            game.step({})
            game.step({})
            game.step({'FRANCE': ['A PAR D']})
            assert game.state['powers']['FRANCE']['welfare'] == 1

    def test_welfare_with_unit_disband(self):
        from pufferlib.ocean.diplomacy import Game
        with Game(max_years=2) as game:
            game.step({})
            game.step({})
            game.step({'FRANCE': ['A PAR D']})
            assert 'A PAR' not in game.state['powers']['FRANCE']['units']

    def test_welfare_never_decreases(self):
        from pufferlib.ocean.diplomacy import Game
        with Game(max_years=2) as game:
            game.step({})
            game.step({})
            game.step({'FRANCE': ['A PAR D']})
            before = game.state['powers']['FRANCE']['welfare']
            for _ in range(3): game.step({})
            assert game.state['powers']['FRANCE']['welfare'] == before + 1

    def test_welfare_reward_calculation(self):
        from pufferlib.ocean.diplomacy import Game
        with Game(max_years=2) as game:
            game.step({})
            game.step({})
            game.step({'FRANCE': ['A PAR D']})
            assert game._env.rewards[2] == 1

    def test_voluntary_disband_order(self):
        from pufferlib.ocean.diplomacy import Game
        with Game(max_years=2) as game:
            game.step({})
            game.step({})
            game.step({'FRANCE': ['A PAR D']})
            assert len(game.state['powers']['FRANCE']['units']) == 2

    def test_no_victory_condition(self):
        from pufferlib.ocean.diplomacy import Game
        from tests.test_diplomacy import set_position
        with Game() as game:
            centers = [l['name'] for l in game.state['locations'] if l['is_supply_center']]
            set_position(game, {}, 'F1901M', {'FRANCE': centers})
            game.step({})
            assert not game.state['terminated']
            assert len(game.state['powers']['FRANCE']['centers']) == 34

    def test_welfare_observation_encoding(self):
        from pufferlib.ocean.diplomacy import Game
        with Game(max_years=2) as game:
            game.step({})
            game.step({})
            game.step({'FRANCE': ['A PAR D']})
            assert game.observe('FRANCE')['powers']['FRANCE']['welfare'] == 1
