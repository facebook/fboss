#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import os
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Optional, Tuple


OPT_ARG_SCRATCH_PATH = "--scratch-path"
OPT_ARG_CMAKE_TARGET = "--target"

FBOSS_IMAGE_NAME = "fboss_image"
FBOSS_CONTAINER_NAME = "FBOSS_BUILD_CONTAINER"
CONTAINER_SCRATCH_PATH = "/var/FBOSS/tmp_bld_dir"


def get_linux_type() -> Tuple[Optional[str], Optional[str], Optional[str]]:
    try:
        with open("/etc/os-release") as f:
            data = f.read()
    except EnvironmentError:
        return (None, None, None)

    os_vars = {}
    for line in data.splitlines():
        parts = line.split("=", 1)
        if len(parts) != 2:
            continue
        key = parts[0].strip()
        value_parts = shlex.split(parts[1].strip())
        if not value_parts:
            value = ""
        else:
            value = value_parts[0]
        os_vars[key] = value

    name = os_vars.get("NAME")
    if name:
        name = name.lower()
        name = re.sub("linux", "", name)
        name = name.strip().replace(" ", "_")

    version_id = os_vars.get("VERSION_ID")
    if version_id:
        version_id = version_id.lower()

    return "linux", name, version_id


def centos_prerequisites(version_id: str):
    if int(version_id) < 9:
        print("Build only supported on CentOS Stream 9", file=sys.stderr)
        return 1

    print("Installing podman-docker via dnf")
    cp = subprocess.run(
        ["sudo", "dnf", "install", "-y", "podman-docker"], capture_output=True
    )
    if cp.returncode != 0:
        errorMsg = f"An error occurred while attempting to installed podman-docker: {cp.stderr}"
        print(errorMsg, file=sys.stderr)
        return cp.returncode
    return 0


def test_prerequisites():
    host_type = get_linux_type()
    (ostype, distro, distro_version) = host_type
    if ostype != "linux":
        errorMsg = f"Running on unsupported OS type: {ostype}"
        print(errorMsg, file=sys.stderr)
        return 1
    else:
        print(f"Running on {ostype}:{distro}")

    if distro == "centos_stream":
        errCode = centos_prerequisites(distro_version)
        if errCode != 0:
            return errCode
    elif distro == "ubuntu":
        print("Ubuntu")
    else:
        print("Unknown Linux distrubtion")

    return 0


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        OPT_ARG_SCRATCH_PATH,
        type=str,
        required=True,
        help=(
            "use this path for build and install files e.g. "
            + OPT_ARG_SCRATCH_PATH
            + "=/opt/app. If this path does not already exist, it will be created."
        ),
    )
    parser.add_argument(
        OPT_ARG_CMAKE_TARGET,
        type=str,
        required=False,
        default=None,
        help="CMake Target to be built (default: build all FBOSS targets)",
    )

    return parser.parse_args()


def create_scratch_path(scratch_path: str):
    exists = os.path.isdir(scratch_path)
    if exists:
        return 0
    os.makedirs(scratch_path)


def get_docker_path():
    scripts_path = os.path.dirname(__file__)
    oss_dir_path = Path(scripts_path).parent.absolute()
    docker_dir_path = os.path.join(oss_dir_path, "docker")
    return docker_dir_path


def build_docker_image(docker_dir_path: str):
    fd, log_path = tempfile.mkstemp(suffix="docker-build.log")
    print(
        f"Attempting to build docker image from {docker_dir_path}/Dockerfile. You can run `sudo tail -f {log_path}` in order to follow along."
    )
    with os.fdopen(fd, "w") as output:
        cp = subprocess.run(
            ["sudo", "docker", "build", docker_dir_path, "-t", FBOSS_IMAGE_NAME],
            stdout=output,
            stderr=subprocess.STDOUT,
        )
    if not cp.returncode == 0:
        errMsg = f"An error occurred while trying to build the FBOSS docker image: {cp.stderr}"
        print(errMsg, file=sys.stderr)
        sys.exit(1)


def run_fboss_build(scratch_path: str, target: Optional[str]):
    cmd_args = ["sudo", "docker", "run"]
    # Add args for directory mount for build output.
    cmd_args.append("-v")
    cmd_args.append(f"{scratch_path}:{CONTAINER_SCRATCH_PATH}:z")
    # Add required capability for sudo permissions
    cmd_args.append("--cap-add=CAP_AUDIT_WRITE")
    # Add TTY flags
    cmd_args.append("-it")
    # Add args for docker container name
    cmd_args.append(f"--name={FBOSS_CONTAINER_NAME}")
    # Add args for image name
    cmd_args.append(f"{FBOSS_IMAGE_NAME}:latest")
    # Add build command args
    build_cmd = [
        "./build/fbcode_builder/getdeps.py",
        "build",
        "--allow-system-packages",
        '--extra-cmake-defines={"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}',
        "--scratch-path",
        f"{CONTAINER_SCRATCH_PATH}",
    ]
    if target is not None:
        build_cmd.append("--cmake-target")
        build_cmd.append(target)
    build_cmd.append("fboss")
    cmd_args.extend(build_cmd)
    subprocess.run(cmd_args)


def main():
    args = parse_args()
    create_scratch_path(args.scratch_path)

    errCode = test_prerequisites()
    if errCode != 0:
        return errCode

    docker_dir_path = get_docker_path()
    build_docker_image(docker_dir_path)

    run_fboss_build(args.scratch_path, args.target)

    return 0


if __name__ == "__main__":
    sys.exit(main())
