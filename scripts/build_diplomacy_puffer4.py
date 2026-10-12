#!/usr/bin/env python3
"""Build the checked-in PufferLib 4 CPU backend for Diplomacy.

No downloads, temporary source patches, or rendering libraries.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig

import pybind11

ROOT = Path(__file__).resolve().parents[1]


def run(command):
    print(shlex.join([str(part) for part in command]), flush=True)
    subprocess.run(command, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--torch-openmp", action="store_true",
                        help="on macOS, share the OpenMP runtime shipped with Torch")
    args = parser.parse_args()
    out = ROOT / "build" / "pufferlib4"
    out.mkdir(parents=True, exist_ok=True)
    includes = [ROOT / "native/pufferlib4", ROOT / "pufferlib/ocean/diplomacy", ROOT / "src"]
    common = ["-O2", "-fPIC", "-fvisibility=hidden"]
    omp_link = ["-fopenmp"]
    torch_omp = None
    if sys.platform == "darwin":
        if args.torch_openmp:
            import torch
            omp = Path(torch.__file__).resolve().parent
            torch_omp = omp / "lib/libomp.dylib"
        else:
            omp = Path(os.environ.get("PUFFERWELFARE_LIBOMP", "/opt/homebrew/opt/libomp"))
        if not (omp / "include/omp.h").is_file():
            parser.error("libomp is required; set PUFFERWELFARE_LIBOMP to its prefix")
        common += ["-Xpreprocessor", "-fopenmp", f"-I{omp / 'include'}"]
        omp_link = [f"-L{omp / 'lib'}", f"-Wl,-rpath,{omp / 'lib'}", "-lomp"]
    else:
        if args.torch_openmp:
            parser.error("--torch-openmp is only needed/supported on macOS")
        # Strict C11 otherwise hides clock_gettime/CLOCK_MONOTONIC on Linux.
        common += ["-fopenmp", "-D_POSIX_C_SOURCE=200809L"]
    cc = shlex.split(os.environ.get("CC", "clang"))
    cxx = shlex.split(os.environ.get("CXX", "clang++"))
    definitions = ["-DEnv=DiplomacyEngine", "-DLog=DiplomacyEngineLog"]
    for callback in ["init", "reset", "step", "render", "close", "configure"]:
        definitions.append(f"-Dc_{callback}=diplomacy_engine_{callback}")
    objects = []
    for name, source, defines in [
        ("engine", ROOT / "pufferlib/ocean/diplomacy/diplomacy.c", definitions),
        ("map", ROOT / "pufferlib/ocean/diplomacy/diplomacy_map.c", definitions),
        ("adapter", ROOT / "native/pufferlib4/binding.c", []),
    ]:
        obj = out / f"{name}.o"
        run(cc + common + ["-std=c11", "-Werror=implicit-function-declaration"]
            + [f"-I{path}" for path in includes] + defines + ["-c", str(source), "-o", str(obj)])
        objects.append(str(obj))
    extension = ROOT / "pufferlib" / ("_C" + sysconfig.get_config_var("EXT_SUFFIX"))
    link = ["-undefined", "dynamic_lookup"] if sys.platform == "darwin" else []
    run(cxx + common + ["-std=c++17", "-shared", "-DENV_NAME=diplomacy"]
        + [f"-I{path}" for path in includes + [Path(pybind11.get_include()), Path(sysconfig.get_path("include"))]]
        + [str(ROOT / "src/bindings_cpu.cpp")] + objects + omp_link + link + ["-o", str(extension)])
    if torch_omp is not None:
        # Torch's bundled library retains a Homebrew install name. Resolve it
        # through the Torch rpath instead of loading a second Homebrew runtime.
        install_name = subprocess.check_output(["otool", "-D", str(torch_omp)], text=True).splitlines()[1]
        run(["install_name_tool", "-change", install_name, "@rpath/libomp.dylib", str(extension)])
        run(["codesign", "--force", "--sign", "-", str(extension)])
    print(f"Built CPU environment backend: {extension}")


if __name__ == "__main__":
    main()
