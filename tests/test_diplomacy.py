"""Public full-order interface, complete games, observations and replay."""
import numpy as np
import pytest
from pufferlib.ocean.diplomacy import Game, POWERS, binding


def set_position(game, units, phase='S1901M', centers=None):
    handle = game._env.env_handle
    for i, p in enumerate(POWERS):
        binding.game_clear_units(handle, i)
        binding.game_set_units(handle, i, [tuple(u.split()) for u in units.get(p, [])])
    if centers is not None:
        binding.game_clear_centers(handle)
        for p, locations in centers.items():
            binding.game_set_centers(handle, POWERS.index(p), locations)
    binding.game_set_phase(handle, phase)


def test_complete_initial_observations():
    with Game() as game:
        state = game.state
        assert len(state['locations']) == 82
        assert sum(l['is_supply_center'] for l in state['locations']) == 34
        assert sum(len(v['units']) for v in state['powers'].values()) == 22
        assert 'F STP/SC' in state['powers']['RUSSIA']['units']
        assert len(game.observations()) == 7
        for p in POWERS:
            assert game.observe(p)['actor'] == p
            assert game.observe(p)['unit_slots'] == state['powers'][p]['units']
        assert sum(l['owner_power'] < 0 and l['is_supply_center'] for l in state['locations']) == 12
        assert all(l['home_power'] == -1 for l in state['locations'] if not l['is_supply_center'])


def test_all_units_support_and_retreat():
    with Game() as game:
        set_position(game, {'FRANCE': ['A BUR', 'A PIC'], 'GERMANY': ['A BEL']})
        game.step({'FRANCE': ['A BUR - BEL', 'A PIC S A BUR - BEL'], 'GERMANY': ['A BEL H']})
        assert game.state['phase'] == 'SPRING_RETREAT'
        assert 'A BEL' in game.state['powers']['FRANCE']['units']
        retreat = game.state['retreats'][0]
        assert retreat['power'] == 'GERMANY'
        assert 'HOL' in retreat['destinations']
        assert 'A BEL R HOL' in game.possible_orders('GERMANY')['A BEL']
        game.step({'GERMANY': ['A BEL R HOL']})
        assert game.state['powers']['GERMANY']['units'] == ['A HOL']


def test_convoy_through_public_interface():
    with Game() as game:
        set_position(game, {'ENGLAND': ['A LON', 'F NTH']})
        game.step({'ENGLAND': ['A LON - BEL VIA', 'F NTH C A LON - BEL']})
        assert 'A BEL' in game.state['powers']['ENGLAND']['units']


def test_empty_power_build_and_coast_build():
    with Game() as game:
        set_position(game, {}, 'W1901A', {'RUSSIA': ['STP', 'MOS']})
        assert 'F STP/SC B' in game.possible_orders('RUSSIA')['STP/SC']
        game.step({'RUSSIA': ['F STP/SC B', 'A MOS B']})
        assert sorted(game.state['powers']['RUSSIA']['units']) == ['A MOS', 'F STP/SC']


def test_invalid_submission_is_atomic():
    with Game() as game:
        before = game.state_hash()
        with pytest.raises(ValueError):
            game.step({'FRANCE': ['A PAR - BUR'], 'GERMANY': ['nonsense']})
        assert before == game.state_hash()
        game.step({})
        assert 'A PAR' in game.state['powers']['FRANCE']['units']


@pytest.mark.parametrize('seed', [0, 1, 2])
@pytest.mark.parametrize('welfare', [True, False])
def test_complete_seeded_games_and_replay(seed, welfare, tmp_path):
    with Game(max_years=3, welfare_mode=welfare, seed=seed) as game:
        rewards_total = {p: 0 for p in POWERS}
        for step in range(30):
            orders = game.scripted_orders(seed * 100 + step)
            _, rewards, done, truncated, _ = game.step(orders)
            assert not truncated
            state = game.state
            occupied = []
            for p, data in state['powers'].items():
                rewards_total[p] += rewards[p]
                assert np.isfinite(rewards[p])
                assert len(data['units']) <= 34
                occupied += [u.split()[1].split('/')[0] for u in data['units']]
                assert data['welfare'] == rewards_total[p]
            assert len(occupied) == len(set(occupied))
            assert 'SWI' not in occupied
            if done:
                break
        else:
            pytest.fail('three-year game did not finish')
        assert state['year'] == 1904
        path = tmp_path / 'replay.json'
        game.save_replay(path)
        with Game.from_replay(path) as restored:
            assert restored.state_hash() == game.state_hash()
        with pytest.raises(RuntimeError):
            game.step({})
        game.reset()
        assert game.state['year'] == 1901
    game.close()
    with pytest.raises(RuntimeError):
        game.observe('FRANCE')


def test_support_must_reach_destination_not_origin():
    with Game() as game:
        set_position(game, {'FRANCE': ['A PAR', 'A BUR'], 'GERMANY': ['A MUN']})
        assert 'A PAR S A BUR - MUN' not in game.possible_orders('FRANCE')['A PAR']
        game.step({'FRANCE': ['A BUR - MUN', 'A PAR S A BUR - MUN']})
        assert 'A MUN' in game.state['powers']['GERMANY']['units']


def test_army_coast_build_normalizes_parent():
    with Game() as game:
        set_position(game, {}, 'W1901A', {'RUSSIA': ['STP']})
        game.step({'RUSSIA': ['A STP/SC B']})
        assert game.state['powers']['RUSSIA']['units'] == ['A STP']


def test_convoys_require_distinct_coastal_provinces():
    with Game() as game:
        set_position(game, {'ENGLAND': ['A LON', 'F NTH']})
        options = game.possible_orders('ENGLAND')['F NTH']
        for destination in ['ADR', 'MOS', 'LON']:
            assert 'F NTH C A LON - ' + destination not in options


def test_wrong_type_winter_disband_is_void():
    with Game() as game:
        game.step({})
        game.step({})
        game.step({'FRANCE': ['F PAR D']})
        assert 'A PAR' in game.state['powers']['FRANCE']['units']


def test_retreat_options_exclude_occupied_provinces():
    with Game() as game:
        set_position(game, {'FRANCE': ['A BUR', 'A PIC'], 'GERMANY': ['A BEL'], 'ITALY': ['A HOL']})
        game.step({'FRANCE': ['A BUR - BEL', 'A PIC S A BUR - BEL']})
        choices = game.state['retreats'][0]['destinations']
        assert 'HOL' not in choices
        assert 'PIC' not in choices
        assert 'RUH' in choices


def test_wrong_type_retreat_disbands_instead_of_moving():
    with Game() as game:
        set_position(game, {'FRANCE': ['A BUR', 'A PIC'], 'GERMANY': ['A BEL']})
        game.step({'FRANCE': ['A BUR - BEL', 'A PIC S A BUR - BEL']})
        _, _, _, _, info = game.step({'GERMANY': ['F BEL R HOL']})
        assert game.state['powers']['GERMANY']['units'] == []
        assert info['results']['GERMANY'] == [5]


def test_duplicate_coast_orders_are_rejected_atomically():
    with Game() as game:
        before = game.state_hash()
        with pytest.raises(ValueError):
            game.step({'RUSSIA': ['F STP/SC - BOT', 'F STP/NC - FIN']})
        assert before == game.state_hash()


def test_retreats_to_opposite_coasts_conflict():
    with Game() as game:
        set_position(game, {'FRANCE': ['F MAO'], 'ITALY': ['F LYO'],
                            'ENGLAND': ['F NAO', 'F IRI'], 'TURKEY': ['F TYS', 'F WES']})
        game.step({'ENGLAND': ['F NAO - MAO', 'F IRI S F NAO - MAO'],
                   'TURKEY': ['F TYS - LYO', 'F WES S F TYS - LYO']})
        assert game.state['phase'] == 'SPRING_RETREAT'
        assert len(game.state['retreats']) == 2
        game.step({'FRANCE': ['F MAO R SPA/NC'], 'ITALY': ['F LYO R SPA/SC']})
        assert game.state['powers']['FRANCE']['units'] == []
        assert game.state['powers']['ITALY']['units'] == []


def test_retreat_results_follow_unit_not_submission_order():
    with Game() as game:
        set_position(game, {'FRANCE': ['F MAO', 'F LYO'], 'ITALY': ['F GAS'],
                            'ENGLAND': ['F NAO', 'F IRI'], 'TURKEY': ['F TYS', 'F WES'],
                            'GERMANY': ['A PAR', 'A BRE']})
        game.step({'ENGLAND': ['F NAO - MAO', 'F IRI S F NAO - MAO'],
                   'TURKEY': ['F TYS - LYO', 'F WES S F TYS - LYO'],
                   'GERMANY': ['A PAR - GAS', 'A BRE S A PAR - GAS']})
        assert len(game.state['retreats']) == 3
        _, _, _, _, info = game.step({'FRANCE': ['F LYO R MAR', 'F MAO R SPA/NC'],
                                     'ITALY': ['F GAS R SPA/NC']})
        assert game.state['powers']['FRANCE']['units'] == ['F MAR']
        assert game.state['powers']['ITALY']['units'] == []
        assert info['results']['FRANCE'] == [1, 2]


def test_army_retreat_coast_alias_normalizes_parent():
    with Game() as game:
        set_position(game, {'RUSSIA': ['A LVN'], 'GERMANY': ['A PRU', 'A WAR']})
        game.step({'GERMANY': ['A PRU - LVN', 'A WAR S A PRU - LVN']})
        choices = game.possible_orders('RUSSIA')['A LVN']
        assert 'A LVN R STP' in choices
        assert 'A LVN R STP/SC' not in choices
        game.step({'RUSSIA': ['A LVN R STP/SC']})
        assert game.state['powers']['RUSSIA']['units'] == ['A STP']
