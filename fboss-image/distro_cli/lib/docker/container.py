# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Docker container lifecycle management."""

import logging
import subprocess
from pathlib import Path

logger = logging.getLogger(__name__)


def run_container(  # noqa: PLR0913
    image: str,
    command: list[str],
    volumes: dict[Path, Path] | None = None,
    env: dict[str, str] | None = None,
    privileged: bool = False,
    interactive: bool = False,
    ephemeral: bool = True,
    working_dir: str | None = None,
    name: str | None = None,
) -> int:
    """Run a command in a Docker container.

    Args:
        image: Docker image name to run
        command: Command to execute in container (as list)
        volumes: Dictionary mapping host paths to container paths
        env: Dictionary of environment variables
        privileged: Run container in privileged mode
        interactive: Run container in interactive mode (-it)
        ephemeral: Remove container after it exits (--rm, default: True)
        working_dir: Working directory inside container
        name: Name for the container

    Returns:
        Exit code from the container

    Raises:
        RuntimeError: If docker command fails to start
    """
    logger.info(f"Running container from image: {image}")
    logger.info(f"Executing: {command}")

    # Build docker run command
    cmd = ["docker", "run"]

    # Start with default flags
    cmd.append("--network=host")

    # Add flags
    if ephemeral:
        cmd.append("--rm")

    if interactive:
        cmd.extend(["-i", "-t"])

    if privileged:
        cmd.append("--privileged")

    if name:
        cmd.extend(["--name", name])

    if working_dir:
        cmd.extend(["-w", working_dir])

    if volumes:
        for host_path, container_path in volumes.items():
            cmd.extend(["-v", f"{host_path}:{container_path}"])

    if env:
        for key, value in env.items():
            cmd.extend(["-e", f"{key}={value}"])

    cmd.append(image)

    # Finally, add the actual command to run
    cmd.extend(command)

    logger.debug(f"Running: {' '.join(str(c) for c in cmd)}")

    try:
        result = subprocess.run(
            cmd,
            check=False,  # Don't raise on non-zero exit
        )
        logger.info(f"Container exited with code: {result.returncode}")
        return result.returncode
    except FileNotFoundError as e:
        raise RuntimeError(
            "Docker command not found. Is Docker installed and in PATH?"
        ) from e
