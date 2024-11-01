#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ARG_FBOSS_TARBALL = "fboss_tar"

# TODO: paulcruz74 - deduplicate this from docker-build.py
FBOSS_IMAGE_NAME = "fboss_image"
FBOSS_CONTAINER_NAME = "FBOSS_BUILD_CONTAINER"

TEST_PATH_REGEX = re.compile(".*tests?$")
HW_TEST_PATH_REGEX = re.compile(".*hw_tests?$")


# TODO: paulcruz74 - deduplicate this from docker-build.py
def get_docker_path():
    github_actions_path = os.path.dirname(__file__)
    scripts_path = Path(github_actions_path).parent.absolute()
    oss_dir_path = Path(scripts_path).parent.absolute()
    docker_dir_path = os.path.join(oss_dir_path, "docker")
    return docker_dir_path


def build_docker_image(docker_dir_path: str):
    fd, log_path = tempfile.mkstemp(suffix="docker-build.log")
    print(
        f"Attempting to build docker image from {docker_dir_path}/Dockerfile. You can run `sudo tail -f {log_path}` in order to follow along."
    )
    with os.fdopen(fd, "w") as output:
        dockerfile_path = os.path.join(docker_dir_path, "Dockerfile")
        cp = subprocess.run(
            [
                "sudo",
                "docker",
                "build",
                ".",
                "-t",
                FBOSS_IMAGE_NAME,
                "-f",
                dockerfile_path,
            ],
            stdout=output,
            stderr=subprocess.STDOUT,
        )
    if not cp.returncode == 0:
        errMsg = f"An error occurred while trying to build the FBOSS docker image: {cp.stderr}"
        print(errMsg, file=sys.stderr)
        sys.exit(1)


def unpack_tarball(tarball_path: str) -> str:
    output_dir = tempfile.mkdtemp()
    print(f"Unpacking tarball to {output_dir}")
    subprocess.run(["tar", "-xvf", tarball_path, "-C", output_dir])
    return output_dir


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(ARG_FBOSS_TARBALL)

    return parser.parse_args()


def cleanup(output_dir: str):
    shutil.rmtree(output_dir)


def find_tests(output_dir: str) -> list[str]:
    tests = []
    bin_dir = os.path.join(output_dir, "bin")
    for f in os.listdir(bin_dir):
        file_path = os.path.join(bin_dir, f)
        # Make sure to ignore hw tests as they will not pass on GitHub actions runners.
        if is_test(file_path) and not is_hw_test(file_path):
            tests.append(f)
    return tests


def is_test(path: str) -> bool:
    match = TEST_PATH_REGEX.match(path)
    if not match:
        return False

    return os.path.isfile(path) and os.access(path, os.X_OK)


def is_hw_test(path: str) -> bool:
    match = HW_TEST_PATH_REGEX.match(path)
    if not match:
        return False

    return os.path.isfile(path) and os.access(path, os.X_OK)


def run_test(test: str, output_dir: str) -> bool:
    cmd_args = ["sudo", "docker", "run"]
    lib_path = os.path.join(output_dir, "lib")
    cmd_args.extend(["-e", f"LD_LIBRARY_PATH={lib_path}"])
    cmd_args.extend(["-v", f"{output_dir}:{output_dir}:z"])
    # Add required capability for sudo permissions
    cmd_args.append("--cap-add=CAP_AUDIT_WRITE")
    cmd_args.append(f"{FBOSS_IMAGE_NAME}:latest")
    test_path = os.path.join(output_dir, "bin", test)
    cmd_args.append(test_path)
    cp = subprocess.run(cmd_args)
    return cp.returncode == 0


def main():
    args = parse_args()
    docker_path = get_docker_path()
    build_docker_image(docker_path)

    output_dir = unpack_tarball(args.fboss_tar)

    tests = find_tests(output_dir)

    failed_tests = []

    for test in tests:
        is_pass = run_test(test, output_dir)
        if not is_pass:
            failed_tests.append(test)

    failed_test_string = ", ".join(failed_tests)

    exit_code = 0
    if len(failed_tests) == 0:
        print("All tests passed!")
    else:
        print(f"The following tests failed: {failed_test_string}")
        exit_code = 1

    cleanup(output_dir)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
