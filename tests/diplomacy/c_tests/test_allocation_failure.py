"""A C harness injects allocator failures without modifying production allocators."""
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def test_engine_allocation_failure_leaves_empty_safe_environment(tmp_path):
    engine = ROOT / 'pufferlib/ocean/diplomacy'
    source = tmp_path / 'failure.c'
    source.write_text('''
#include <stdlib.h>
#include <assert.h>
static int calls, fail_at;
static void* test_calloc(size_t count, size_t size) {
    if (++calls == fail_at) return NULL;
    return calloc(count, size);
}
#define calloc test_calloc
#include "diplomacy.c"
#undef calloc
int main(void) {
    for (fail_at = 1; fail_at <= 2; fail_at++) {
        Env env = {0};
        calls = 0;
        c_init(&env);
        assert(env.game == NULL);
        c_reset(&env);
        c_step(&env);
        c_close(&env);
        c_close(&env);
    }
    return 0;
}
''')
    executable = tmp_path / 'failure'
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    subprocess.run(compiler + ['-std=c99', '-I', str(engine), str(source),
                              str(engine / 'diplomacy_map.c'), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True, timeout=10)
