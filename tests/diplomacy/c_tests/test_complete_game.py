"""Complete-game contract: standard Fall capture and paper section 2.2 welfare."""
import numpy as np
import pytest
from tests.diplomacy.adapters import GameAdapter
from pufferlib.ocean.diplomacy import binding


@pytest.fixture
def game():
    g = GameAdapter()
    yield g
    g.env.close()


def test_fall_capture_and_reset(game):
    game.clear_units()
    game.set_units('FRANCE', ['A BUR'])
    game.set_current_phase('F1901M')
    game.set_orders('FRANCE', ['A BUR - BEL'])
    game.process()
    assert 'BEL' in game.get_centers('FRANCE')
    assert len(game.get_centers('FRANCE')) == 4
    game.env.reset()
    assert len(game.get_centers('FRANCE')) == 3


def test_spring_occupation_does_not_capture(game):
    game.clear_units()
    game.set_units('FRANCE', ['A BUR'])
    game.set_orders('FRANCE', ['A BUR - BEL'])
    game.process()
    assert 'BEL' not in game.get_centers('FRANCE')


def test_switzerland_impassable(game):
    assert not binding.can_move_names(game.env.env_handle, 1, 'BUR', 'SWI')
    game.clear_units()
    game.set_units('FRANCE', ['A BUR'])
    game.set_orders('FRANCE', ['A BUR - SWI'])
    game.process()
    assert game.get_units('FRANCE') == ['A BUR']


def test_movement_disband_invalid(game):
    game.set_orders('FRANCE', ['A PAR D'])
    game.process()
    assert 'A PAR' in game.get_units('FRANCE')


def test_mixed_winter_adjustments_rejected(game):
    game.clear_units()
    game.set_units('FRANCE', ['A PAR'])
    game.set_centers('FRANCE', ['PAR', 'MAR', 'BRE'])
    game.set_current_phase('W1901A')
    game.set_orders('FRANCE', ['A PAR D', 'A MAR B'])
    game.process()
    assert game.get_units('FRANCE') == ['A PAR']


def test_initial_center_ownership_observations(game):
    board = game.env.observations[0, :1620].reshape(81, 20)
    assert board[:, 11:18].sum() == 22
    assert board[:, 18].sum() == 12


def test_direct_movement_matches_pinned_reference_map(game):
    """All army base-province and fleet directed edges, not selected examples."""
    from pathlib import Path
    rows = {}
    source = Path(__file__).parents[1] / 'reference/standard.map'
    for line in source.read_text().splitlines():
        fields = line.split()
        if len(fields) > 3 and fields[2] == 'ABUTS':
            rows[fields[1]] = (fields[0], fields[3:])
    for kind in [1, 2]:
        for origin, (origin_type, adjacent) in rows.items():
            if origin.upper() != origin:
                continue
            for destination, (dest_type, _) in rows.items():
                if destination.upper() != destination:
                    continue
                if kind == 1:
                    if '/' in origin or '/' in destination:
                        continue
                    army_edges = rows.get(origin.lower(), rows[origin])[1]
                    expected = (origin_type not in ('WATER', 'SHUT') and
                                dest_type not in ('WATER', 'SHUT') and
                                destination in [p.upper().split('/')[0] for p in army_edges])
                else:
                    expected = (origin_type in ('WATER', 'COAST') and
                                dest_type in ('WATER', 'COAST') and destination in adjacent)
                assert bool(binding.can_move_names(game.env.env_handle, kind, origin, destination)) == expected, (kind, origin, destination)
