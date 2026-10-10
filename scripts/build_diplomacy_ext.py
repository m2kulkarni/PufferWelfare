#!/usr/bin/env python3
"""Build only the PufferWelfare Diplomacy C extension.

The repository's general setup builds every Ocean and training extension and may
download unrelated native dependencies. Diplomacy CI needs a smaller, auditable
build surface.
"""

from pathlib import Path
import platform

import numpy
from setuptools import Extension, setup


ROOT = Path(__file__).resolve().parents[1]
DIPLOMACY = ROOT / "pufferlib" / "ocean" / "diplomacy"

compile_args = ["-O2", "-std=c99", "-DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION"]
if platform.system() == "Darwin":
    # Match the repository build until the existing forward declarations are fixed.
    compile_args.append("-Wno-error=implicit-function-declaration")
elif platform.system() == "Linux":
    compile_args.append("-Wno-implicit-function-declaration")


extension = Extension(
    "pufferlib.ocean.diplomacy.binding",
    sources=[
        str(DIPLOMACY / "binding.c"),
        str(DIPLOMACY / "diplomacy.c"),
        str(DIPLOMACY / "diplomacy_map.c"),
    ],
    include_dirs=[numpy.get_include(), str(DIPLOMACY), str(DIPLOMACY.parent)],
    extra_compile_args=compile_args,
)


setup(
    name="pufferwelfare-diplomacy-extension",
    version="0.0.0",
    packages=[],
    ext_modules=[extension],
)
