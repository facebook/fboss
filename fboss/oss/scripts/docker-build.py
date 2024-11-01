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
OPT_ARG_NO_DOCKER_OUTPUT = "--no-docker-output"
OPT_ARG_NO_SYSTEM_DEPS = "--no-system-deps"
OPT_ARG_ADD_BUILD_ENV_VAR = "--env-var"
OPT_ARG_LOCAL = "--local"

FBOSS_IMAGE_NAME = "fboss_image"
FBOSS_CONTAINER_NAME = "FBOSS_BUILD_CONTAINER"
CONTAINER_SCRATCH_PATH = "/var/FBOSS/tmp_bld_dir"


def get_linux_type() -> tuple[Optional[str], Optional[str], Optional[str]]:
    try:
        with open("/etc/os-release") as f:
            data = f.read()
    except OSError:
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
    parser.add_argument(
        OPT_ARG_NO_DOCKER_OUTPUT,
        dest="docker_output",
        default=True,
        action="store_false",
        help="Skips step to attach TTY outputs to the docker container.",
    )
    parser.add_argument(
        OPT_ARG_NO_SYSTEM_DEPS,
        dest="use_system_deps",
        default=True,
        action="store_false",
        help="Prevents usage of system libraries to satisfy dependency requirements. If this flag is used, all dependencies will be built from source.",
    )
    parser.add_argument(
        OPT_ARG_ADD_BUILD_ENV_VAR,
        dest="env_vars",
        default=[],
        action="append",
        help=(
            "Usage: --env-var <VAR>:<VAL>. "
            "Adds a new environment variable to be set before performing the build. "
            "This is particularly useful as some CMake targets are hidden behind flags, e.g. BUILD_SAI_FAKE=1"
        ),
    )
    parser.add_argument(
        OPT_ARG_LOCAL,
        required=False,
        help="Compiles using the local clone of the FBOSS git repository. By default, a separate clone of the repository with the head commit is used.",
        default=False,
        action="store_true",
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


def run_fboss_build(
    scratch_path: str,
    target: Optional[str],
    docker_output: bool,
    use_system_deps: bool,
    env_vars: list[str],
    use_local: bool,
):
    cmd_args = ["sudo", "docker", "run"]
    # Add build environment variables, if any.
    for ev in env_vars:
        if ":" not in ev:
            cmd_args.extend(["-e", f"{ev}=1"])
        elif ev.count(":") == 1:
            cmd_args.extend(["-e", ev])
        else:
            errMsg = f"Ignoring environment variable string {ev} as it does not match a supported pattern."
            print(errMsg, file=sys.stderr)

    # Add args for directory mount for build output.
    cmd_args.append("-v")
    cmd_args.append(f"{scratch_path}:{CONTAINER_SCRATCH_PATH}:z")
    # Add required capability for sudo permissions
    cmd_args.append("--cap-add=CAP_AUDIT_WRITE")
    # Add TTY flags
    if docker_output:
        cmd_args.append("-it")
    # Add args for docker container name
    cmd_args.append(f"--name={FBOSS_CONTAINER_NAME}")
    # Add args for image name
    cmd_args.append(f"{FBOSS_IMAGE_NAME}:latest")
    # Add build command args
    build_cmd = [
        "./build/fbcode_builder/getdeps.py",
        "build",
        '--extra-cmake-defines={"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}',
        "--scratch-path",
        f"{CONTAINER_SCRATCH_PATH}",
    ]
    if use_system_deps:
        build_cmd.append("--allow-system-packages")
    if target is not None:
        build_cmd.append("--cmake-target")
        build_cmd.append(target)
    if use_local:
        build_cmd.extend(["--src-dir", "."])
    build_cmd.append("fboss")
    cmd_args.extend(build_cmd)
    build_cp = subprocess.run(cmd_args)
    if build_cp.returncode != 0:
        print(
            "[ERROR] Encountered a failure while attempting to build. Check the logs to root cause.",
            file=sys.stderr,
        )
    return build_cp.returncode


def cleanup_fboss_build_container():
    stop_docker_cp = subprocess.run(
        ["sudo", "docker", "container", "stop", FBOSS_CONTAINER_NAME],
        capture_output=True,
    )
    if stop_docker_cp.returncode != 0:
        print(
            f"There was an error stopping the docker container used to build FBOSS: {stop_docker_cp.stderr}. You can try manually stopping the container via `sudo docker container stop {FBOSS_CONTAINER_NAME}`.",
            file=sys.stderr,
        )
        sys.exit(stop_docker_cp.returncode)
    rm_docker_cp = subprocess.run(
        ["sudo", "docker", "container", "rm", FBOSS_CONTAINER_NAME],
        capture_output=True,
    )
    if rm_docker_cp.returncode != 0:
        print(
            f"There was an error stopping the docker container used to build FBOSS: {rm_docker_cp.stderr}. You can try manually removing the container via `sudo docker container rm {FBOSS_CONTAINER_NAME}`.",
            file=sys.stderr,
        )
        sys.exit(stop_docker_cp.returncode)


def main():
    args = parse_args()
    create_scratch_path(args.scratch_path)

    errCode = test_prerequisites()
    if errCode != 0:
        return errCode

    docker_dir_path = get_docker_path()
    build_docker_image(docker_dir_path)

    status_code = run_fboss_build(
        args.scratch_path,
        args.target,
        args.docker_output,
        args.use_system_deps,
        args.env_vars,
        args.local,
    )

    cleanup_fboss_build_container()

    return status_code


if __name__ == "__main__":
    sys.exit(main())
