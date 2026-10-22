"""Exercise native lifecycle failures in child processes, containing C crashes."""
import subprocess
import sys
import textwrap
import unittest


class LifecycleTests(unittest.TestCase):
    def run_child(self, body):
        result = subprocess.run([sys.executable, '-c', textwrap.dedent('''
from pufferlib import _C
args = {'vec': {'total_agents': 7, 'num_buffers': 1, 'num_threads': 1},
        'env': {'max_years': 2, 'welfare_mode': 1}}
v = _C.create_vec(args, 0)
''') + textwrap.dedent(body)], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_close_twice(self):
        self.run_child('v.close(); v.close()')

    def test_post_close_calls(self):
        for operation in ['v.reset()', 'v.cpu_step(0)', 'v.log()',
                          'v.render(0)', 'v.obs_ptr', 'v.rewards_ptr', 'v.terminals_ptr']:
            with self.subTest(operation=operation):
                self.run_child(f'''
v.close()
try:
    {operation}
except RuntimeError as exc:
    assert 'closed' in str(exc)
else:
    raise AssertionError('post-close operation accepted')
''')

    def test_repeated_implicit_and_explicit_close(self):
        self.run_child('''
import gc
assert _C._resource_counts() == (0, 1)
for i in range(50):
    v.reset()
    if i % 2:
        v.close()
    del v
    gc.collect()
    assert _C._resource_counts() == (0, 0)
    v = _C.create_vec(args, 0)
v.close()
assert _C._resource_counts() == (0, 0)
''')

    def test_constructor_exception_releases_dictionaries(self):
        self.run_child('''
v.close()
for _ in range(20):
    bad = dict(args, env={1: 2})
    try:
        _C.create_vec(bad, 0)
    except TypeError:
        pass
    else:
        raise AssertionError('invalid key accepted')
    assert _C._resource_counts() == (0, 0)
''')

    def test_repeated_training_and_trainer_close_twice(self):
        self.run_child('''
v.close()
import contextlib
import gc
import io
import runpy
from pufferlib.torch_pufferl import PuffeRL
original_close = PuffeRL.close
def close_twice(self):
    original_close(self)
    original_close(self)
PuffeRL.close = close_twice
for _ in range(2):
    with contextlib.redirect_stdout(io.StringIO()):
        runpy.run_path('scripts/smoke_pufferlib4_training.py', run_name='__main__')
    gc.collect()
    assert _C._resource_counts() == (0, 0)
''')
