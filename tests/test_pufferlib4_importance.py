"""IMPALA V-trace: rho scales the complete TD residual, not only reward."""
import numpy as np
import pytest
from pufferlib import _C


@pytest.mark.parametrize('rho', [0.0, 0.25, 1.0, 2.0])
@pytest.mark.parametrize('terminal', [0.0, 1.0])
def test_nonunit_importance_scales_value_residual(rho, terminal):
    values = np.array([1., 4., 3.], dtype=np.float32)
    rewards = np.array([0., 6., 2.], dtype=np.float32)
    dones = np.array([0., terminal, 0.], dtype=np.float32)
    importance = np.array([rho, 0.5, 1.], dtype=np.float32)
    advantages = np.zeros(3, dtype=np.float32)
    _C.puff_advantage_cpu(*(a.ctypes.data for a in [values, rewards, dones, importance, advantages]),
                          1, 3, 0.5, 0.8, 1., 1.)
    last = 0.
    expected = np.zeros(3)
    for t in reversed(range(2)):
        nonterminal = 1 - dones[t + 1]
        residual = min(1., importance[t]) * (rewards[t + 1] + 0.5 * values[t + 1] * nonterminal - values[t])
        last = residual + 0.5 * 0.8 * min(1., importance[t]) * nonterminal * last
        expected[t] = last
    np.testing.assert_allclose(advantages, expected, atol=1e-6)
