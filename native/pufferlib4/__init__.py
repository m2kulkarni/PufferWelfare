"""Checked Python entry point for the PufferLib 4 CPU environment pilot."""
import ctypes
import importlib.util
from pathlib import Path
import sysconfig

import numpy as np

_backend = None


def _load_backend():
    global _backend
    if _backend is None:
        path = Path(__file__).resolve().parents[2] / "build/pufferlib4" / ("_C" + sysconfig.get_config_var("EXT_SUFFIX"))
        spec = importlib.util.spec_from_file_location("_C", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _backend = module
    return _backend


class DiplomacyVec:
    """Whole seven-power games; copies returned buffers before exposing them.

    The raw upstream binding accepts unchecked pointers and sizes. Use this
    entry point for the pilot, not _C.create_vec directly. CPU stepping in this
    upstream revision supports one buffer. GPU training is not enabled here.
    """

    def __init__(self, total_agents=7, max_years=10, welfare_mode=True, num_buffers=1):
        if type(total_agents) is not int or total_agents <= 0 or total_agents % 7:
            raise ValueError("total_agents must be a positive multiple of seven")
        if type(num_buffers) is not int or num_buffers != 1:
            raise ValueError("the CPU pilot supports exactly one buffer")
        if type(max_years) is not int or not 1 <= max_years <= 1000:
            raise ValueError("max_years must be an integer between 1 and 1000")
        if type(welfare_mode) is not bool:
            raise ValueError("welfare_mode must be a boolean")
        self.total_agents = total_agents
        self._vec = _load_backend().create_vec({
            "vec": {"total_agents": total_agents, "num_buffers": num_buffers},
            "env": {"max_years": max_years, "welfare_mode": int(welfare_mode)},
        })
        self.reset()

    def _check_open(self):
        if self._vec is None:
            raise RuntimeError("environment is closed")

    def _copy(self, pointer, size):
        return np.ctypeslib.as_array((ctypes.c_float * size).from_address(pointer)).copy()

    def reset(self):
        self._check_open()
        self._vec.reset()
        return self._copy(self._vec.obs_ptr, self.total_agents * 1641).reshape(self.total_agents, 1641)

    def step(self, actions):
        self._check_open()
        actions = np.asarray(actions)
        if actions.shape != (self.total_agents,) or not np.issubdtype(actions.dtype, np.number):
            raise ValueError("actions must be one numeric value per power")
        if np.iscomplexobj(actions) or not np.all(np.isfinite(actions)) or not np.all((actions >= 0) & (actions < 1088) & (actions == np.floor(actions))):
            raise ValueError("actions must be integer values in [0, 1088)")
        actions = np.ascontiguousarray(actions, dtype=np.float32)
        self._vec.cpu_step(actions.ctypes.data)
        return (
            self._copy(self._vec.obs_ptr, self.total_agents * 1641).reshape(self.total_agents, 1641),
            self._copy(self._vec.rewards_ptr, self.total_agents),
            self._copy(self._vec.terminals_ptr, self.total_agents),
        )

    def log(self):
        self._check_open()
        return self._vec.log()

    def close(self):
        if self._vec is not None:
            self._vec.close()
            self._vec = None

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
