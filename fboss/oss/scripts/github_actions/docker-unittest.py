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
INTEGRATION_TEST_PATH_REGEX = re.compile(".*integration_tests?$")
CONTAINER_WORKDIR = "/var/FBOSS/fboss"


def _is_inside_container() -> bool:
    """Detect if running inside a Docker or Podman container."""
    return os.path.exists("/.dockerenv") or os.path.exists("/run/.containerenv")


_use_sudo = None


def _needs_sudo() -> bool:
    """Check if sudo is needed to run docker/podman."""
    global _use_sudo
    if _use_sudo is None:
        cp = subprocess.run(
            ["docker", "info"],
            capture_output=True,
            check=False,
        )
        _use_sudo = cp.returncode != 0
    return _use_sudo


def _docker_cmd() -> list[str]:
    """Return the docker command prefix, with sudo if needed."""
    if _needs_sudo():
        return ["sudo", "docker"]
    return ["docker"]


# TODO: paulcruz74 - deduplicate this from docker-build.py
def get_docker_path():
    github_actions_path = os.path.dirname(__file__)
    scripts_path = Path(github_actions_path).parent.absolute()
    oss_dir_path = Path(scripts_path).parent.absolute()
    return os.path.join(oss_dir_path, "docker")


def get_repo_path():
    github_actions_path = os.path.dirname(__file__)
    scripts_path = Path(github_actions_path).parent.absolute()
    return Path(scripts_path).parent.parent.parent.absolute()


def use_stable_hashes():
    cwd = os.getcwd()
    os.chdir(get_repo_path())

    rm_cmd = [
        "rm",
        "-rf",
        "build/deps/github_hashes/",
    ]
    subprocess.run(rm_cmd, check=False)

    extract_cmd = [
        "tar",
        "xvzf",
        "fboss/oss/stable_commits/latest_stable_hashes.tar.gz",
    ]
    subprocess.run(extract_cmd, check=False)

    os.chdir(cwd)


def is_container_running() -> bool:
    cmd = _docker_cmd() + [
        "ps",
        "--filter",
        f"name=^/{FBOSS_CONTAINER_NAME}$",
        "--format",
        "{{.Names}}",
    ]
    cp = subprocess.run(cmd, capture_output=True, text=True, check=False)
    return FBOSS_CONTAINER_NAME in cp.stdout


def does_image_exist() -> bool:
    cmd = _docker_cmd() + ["images", "-q", FBOSS_IMAGE_NAME]
    cp = subprocess.run(cmd, capture_output=True, text=True, check=False)
    return len(cp.stdout.strip()) > 0


def build_docker_image(docker_dir_path: str):
    fd, log_path = tempfile.mkstemp(suffix="docker-build.log")
    print(
        f"Attempting to build docker image from {docker_dir_path}/Dockerfile. You can run `sudo tail -f {log_path}` in order to follow along."
    )
    with os.fdopen(fd, "w") as output:
        dockerfile_path = os.path.join(docker_dir_path, "Dockerfile")
        cmd = _docker_cmd() + [
            "build",
            ".",
            "-t",
            FBOSS_IMAGE_NAME,
            "-f",
            dockerfile_path,
        ]
        cp = subprocess.run(
            cmd,
            check=False,
            stdout=output,
            stderr=subprocess.STDOUT,
        )
    if cp.returncode != 0:
        err_msg = f"An error occurred while trying to build the FBOSS docker image: {cp.stderr}"
        print(err_msg, file=sys.stderr)
        sys.exit(1)


def unpack_tarball(tarball_path: str) -> str:
    output_dir = tempfile.mkdtemp()
    print(f"Unpacking tarball to {output_dir}")
    subprocess.run(["tar", "-xvf", tarball_path, "-C", output_dir], check=False)
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
        if is_test(file_path) and not is_e2e_test(file_path):
            tests.append(f)
    return tests


def is_test(path: str) -> bool:
    match = TEST_PATH_REGEX.match(path)
    if not match:
        return False

    return os.path.isfile(path) and os.access(path, os.X_OK)


def is_e2e_test(path: str) -> bool:
    return is_hw_test(path) or is_integration_test(path)


def is_hw_test(path: str) -> bool:
    match = HW_TEST_PATH_REGEX.match(path)
    if not match:
        return False

    return os.path.isfile(path) and os.access(path, os.X_OK)


def is_integration_test(path: str) -> bool:
    match = INTEGRATION_TEST_PATH_REGEX.match(path)
    if not match:
        return False

    return os.path.isfile(path) and os.access(path, os.X_OK)


def run_test(test: str, output_dir: str) -> bool:
    use_stable_hashes()
    cmd_args = _docker_cmd() + ["run"]
    lib_path = os.path.join(output_dir, "lib")
    cmd_args.extend(["-e", f"LD_LIBRARY_PATH={lib_path}"])
    # Mount fboss repository in container
    cmd_args.extend(["-v", f"{get_repo_path()}:{CONTAINER_WORKDIR}:z"])
    cmd_args.extend(["-v", f"{output_dir}:{output_dir}:z"])
    # Add required capability for sudo permissions
    cmd_args.append("--cap-add=CAP_AUDIT_WRITE")
    # Use host network to ensure IPv6 localhost (::1) is available for tests
    cmd_args.append("--network=host")
    cmd_args.append(f"{FBOSS_IMAGE_NAME}:latest")
    test_path = os.path.join(output_dir, "bin", test)
    cmd_args.append(test_path)
    cp = subprocess.run(cmd_args, check=False)
    return cp.returncode == 0


def run_test_local(test: str, output_dir: str) -> bool:
    use_stable_hashes()
    lib_path = os.path.join(output_dir, "lib")
    test_path = os.path.join(output_dir, "bin", test)
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = lib_path
    cp = subprocess.run([test_path], env=env, check=False)
    return cp.returncode == 0


def run_test_exec(test: str, output_dir: str) -> bool:
    use_stable_hashes()
    cmd_args = _docker_cmd() + ["exec"]
    lib_path = os.path.join(output_dir, "lib")
    cmd_args.extend(["-e", f"LD_LIBRARY_PATH={lib_path}"])
    cmd_args.append(FBOSS_CONTAINER_NAME)
    test_path = os.path.join(output_dir, "bin", test)
    cmd_args.append(test_path)
    cp = subprocess.run(cmd_args, check=False)
    return cp.returncode == 0


def main():
    args = parse_args()

    inside_container = _is_inside_container()
    container_running = False

    if inside_container:
        print("Running inside container, executing tests directly.")
    else:
        container_running = is_container_running()
        if container_running:
            print(f"Reusing running container '{FBOSS_CONTAINER_NAME}'.")
        elif does_image_exist():
            print(f"Image '{FBOSS_IMAGE_NAME}' already exists, skipping build.")
        else:
            docker_path = get_docker_path()
            build_docker_image(docker_path)

    output_dir = unpack_tarball(args.fboss_tar)

    # If reusing a running container, copy test artifacts in once
    if container_running:
        print(f"Copying test artifacts into container '{FBOSS_CONTAINER_NAME}'...")
        subprocess.run(
            _docker_cmd()
            + [
                "cp",
                output_dir,
                f"{FBOSS_CONTAINER_NAME}:{output_dir}",
            ],
            check=False,
        )

    tests = find_tests(output_dir)

    failed_tests = []

    for test in tests:
        if inside_container:
            is_pass = run_test_local(test, output_dir)
        elif container_running:
            is_pass = run_test_exec(test, output_dir)
        else:
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
