"""Reject malformed layouts at the native boundary before native allocation."""
import math
import pytest
from pufferlib import _C


def args():
    return {'vec': {'total_agents': 7, 'num_buffers': 1},
            'env': {'max_years': 2, 'welfare_mode': 1}}


@pytest.mark.parametrize('key,value', [
    ('total_agents', 7.5), ('total_agents', True), ('total_agents', float('nan')),
    ('total_agents', float('inf')), ('total_agents', 0), ('total_agents', -7),
    ('total_agents', 8), ('total_agents', 28679), ('total_agents', 10**100),
    ('num_buffers', 2), ('num_buffers', 0), ('num_buffers', True),
    ('max_years', 0), ('max_years', 1001), ('max_years', 1.5),
    ('max_years', True), ('welfare_mode', 2), ('welfare_mode', 0.5),
])
def test_rejects_invalid_native_config(key, value):
    config = args()
    config['vec' if key in config['vec'] else 'env'][key] = value
    before = _C._resource_counts()
    with pytest.raises((ValueError, TypeError)):
        _C.create_vec(config, 0)
    assert _C._resource_counts() == before


def test_rejects_gpu():
    with pytest.raises(ValueError):
        _C.create_vec(args(), 1)


@pytest.mark.parametrize('count', [7, 14, 21])
def test_valid_complete_games(count):
    config = args()
    config['vec']['total_agents'] = count
    vec = _C.create_vec(config, 0)
    try:
        vec.reset()
        assert vec.total_agents == count
    finally:
        vec.close()
