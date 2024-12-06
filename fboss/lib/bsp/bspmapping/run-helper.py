#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
Helper script for building and running the BSP Mapping script generation.

This should be run from the root of the FBOSS repository.

Sample invocation:

   [~/src/fboss-repo]$ ./fboss/lib/bsp/bspmapping/run-helper.py
"""

import os
import subprocess
import sys
from pathlib import Path


def main() -> None:
    run_path = os.getcwd()
    parents = Path(__file__).parents
    if len(parents) <= 4:
        print(
            "Please run the script from the root of the FBOSS repository",
            file=sys.stderr,
        )
        exit(1)
    expected_path = parents[4].absolute().as_posix()
    if run_path != expected_path:
        error_string = f"""Please executed the script from the root of the FBOSS repository
Expected run path: {expected_path}
Current run path: {run_path}"""
        print(error_string, file=sys.stderr)
        exit(1)

    get_deps_paths = [
        expected_path + "/opensource/fbcode_builder/getdeps.py",
        expected_path + "/build/fbcode_builder/getdeps.py",
    ]
    get_deps_path = None
    for maybe_path in get_deps_paths:
        if os.path.isfile(maybe_path):
            get_deps_path = maybe_path
    if get_deps_path is None:
        print("Could not find getdeps.py", file=sys.stderr)
        exit(1)
    else:
        print(get_deps_path)

    print("Starting build for fboss-bspmapping-gen")
    subprocess.run(
        f"""https_proxy=fwdproxy:8080 http_proxy=fwdproxy:8080 {get_deps_path} build """
        + '--allow-system-packages --num-jobs 32 --extra-cmake-defines=\'{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}\' --cmake-target fboss-bspmapping-gen fboss',
        shell=True,
    )

    print("Completed build for fboss-bspmapping-gen")

    show_build_dir_proc = subprocess.run(
        f"""https_proxy=fwdproxy:8080 http_proxy=fwdproxy:8080 {get_deps_path} show-build-dir fboss""",
        shell=True,
        capture_output=True,
        text=True,
    )

    fboss_bspmapping_gen = show_build_dir_proc.stdout.rstrip() + "/fboss-bspmapping-gen"

    subprocess.run(
        fboss_bspmapping_gen,
        shell=True,
    )

    print("Configs have been written to generated_configs")

    exit(0)


if __name__ == "__main__":
    main()
