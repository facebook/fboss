# pyre-strict
#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
Helper script for building and running any FBOSS OSS target.

This should be run from the root of the FBOSS repository.

Sample invocation:

   [~/src/fboss-repo]$ python3 fboss/lib/oss/run-helper.py --target fboss-bspmapping-gen
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple


def get_command_line_args() -> Tuple[str, List[str]]:
    parser = argparse.ArgumentParser(
        description="OSS FBOSS build and run helper script."
    )
    parser.add_argument("--target", type=str, required=True, help="Target to build")
    args, unknown_args = parser.parse_known_args()

    return args.target, unknown_args


def main() -> None:
    target, command_line_args = get_command_line_args()

    run_path = os.getcwd()
    parents = Path(__file__).parents
    if len(parents) <= 3:
        print(
            "Please run the script from the root of the FBOSS repository",
            file=sys.stderr,
        )
        exit(1)
    expected_path = parents[3].absolute().as_posix()
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
    print(get_deps_paths)
    get_deps_path = None
    for maybe_path in get_deps_paths:
        if os.path.isfile(maybe_path):
            get_deps_path = maybe_path
    if get_deps_path is None:
        print("Could not find getdeps.py", file=sys.stderr)
        exit(1)
    else:
        print(get_deps_path)

    hostname = str(subprocess.check_output("hostname"))
    is_facebook_machine = "facebook" in hostname or "fbinfra" in hostname
    proxy_env_vars = "https_proxy=fwdproxy:8080 http_proxy=fwdproxy:8080"
    if is_facebook_machine:
        get_deps_path = f"{proxy_env_vars} {get_deps_path}"

    print(f"Starting build for {target}")
    subprocess.run(
        f"""{get_deps_path} build """
        + '--allow-system-packages --num-jobs 32 --extra-cmake-defines=\'{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}\' --cmake-target'
        + f" {target} fboss",
        shell=True,
    )

    print(f"Completed build for {target}")

    show_build_dir_proc = subprocess.run(
        f"""{get_deps_path} show-build-dir fboss""",
        shell=True,
        capture_output=True,
        text=True,
    )

    fboss_oss_target = (
        show_build_dir_proc.stdout.rstrip()
        + f"/{target}"
        + " "
        + " ".join(command_line_args)
    )

    result = subprocess.run(
        fboss_oss_target,
        shell=True,
    )

    if result.returncode != 0:
        print(f"Failed to run target {target}", file=sys.stderr)
        exit(1)

    print("Configs have been written to specified output directory")
    exit(0)


if __name__ == "__main__":
    main()
