# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Docker image management utilities."""

import hashlib
import json
import logging
import os
import subprocess
import time
from pathlib import Path

from distro_cli.lib.constants import FBOSS_BUILDER_IMAGE
from distro_cli.lib.paths import get_abs_path

logger = logging.getLogger(__name__)

# Default cache expiration time in seconds (24 hours)
DEFAULT_CACHE_EXPIRATION_SECONDS = 24 * 60 * 60

# Container registry for caching builder images across ephemeral VMs
CONTAINER_REGISTRY = "container-registry.sw.internal.nexthop.ai"


def _hash_directory_tree(
    directory: Path, exclude_patterns: list[str] | None = None
) -> str:
    """Hash all files in a directory tree.

    Args:
        directory: Directory to hash
        exclude_patterns: List of patterns to exclude (e.g., '__pycache__', '.pyc')

    Returns:
        SHA256 hexdigest of all files in the directory
    """
    if exclude_patterns is None:
        exclude_patterns = []

    hasher = hashlib.sha256()

    # Get all files, sorted for deterministic ordering
    for file_path in sorted(directory.rglob("*")):
        if not file_path.is_file():
            continue

        # Skip excluded patterns
        skip = False
        for pattern in exclude_patterns:
            if pattern in str(file_path):
                skip = True
                break
        if skip:
            continue

        # Hash the relative path (for structure changes)
        rel_path = file_path.relative_to(directory)
        hasher.update(str(rel_path).encode())

        # Hash the file content
        hasher.update(file_path.read_bytes())

    return hasher.hexdigest()


def _compute_dependency_checksum(root_dir: Path) -> str:
    """Compute checksum of Dockerfile and all its dependencies.

    This includes:
    - Dockerfile
    - All files in build/ directory (manifests, getdeps.py, Python modules, etc.)

    Args:
        root_dir: Root directory of the repository

    Returns:
        SHA256 hexdigest of all dependency files
    """
    hasher = hashlib.sha256()

    # Hash Dockerfile
    dockerfile = root_dir / "fboss" / "oss" / "docker" / "Dockerfile"
    if not dockerfile.exists():
        raise RuntimeError(f"Dockerfile not found: {dockerfile}")
    hasher.update(dockerfile.read_bytes())

    # Hash entire build/ directory (excluding ephemeral files)
    build_dir = root_dir / "build"
    if not build_dir.exists():
        raise RuntimeError(f"build/ directory not found: {build_dir}")

    # Exclude Python bytecode and cache files
    exclude_patterns = ["__pycache__", ".pyc", ".pyo"]
    build_hash = _hash_directory_tree(build_dir, exclude_patterns)
    hasher.update(build_hash.encode())

    return hasher.hexdigest()


def _get_image_build_timestamp(image_tag: str) -> int | None:
    """Get the build timestamp from a Docker image label.

    Args:
        image_tag: Full image tag (e.g., "fboss_builder:abc123")

    Returns:
        Unix timestamp when image was built, or None if not found
    """
    try:
        result = subprocess.run(
            [
                "docker",
                "image",
                "inspect",
                image_tag,
                "--format",
                "{{json .Config.Labels}}",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            return None

        labels = json.loads(result.stdout.strip())
        if not labels:
            return None

        timestamp_str = labels.get("build_timestamp")
        if not timestamp_str:
            return None

        return int(timestamp_str)
    except (json.JSONDecodeError, ValueError, FileNotFoundError):
        return None


def _try_pull_from_registry(checksum: str) -> bool:
    """Try to pull the builder image from the container registry.

    Args:
        checksum: The checksum tag to pull

    Returns:
        True if pull succeeded and image is now available locally, False otherwise
    """
    registry_tag = f"{CONTAINER_REGISTRY}/{FBOSS_BUILDER_IMAGE}:{checksum}"
    local_tag = f"{FBOSS_BUILDER_IMAGE}:{checksum}"

    logger.info(
        f"Trying to pull {FBOSS_BUILDER_IMAGE}:{checksum[:12]} from registry..."
    )

    try:
        result = subprocess.run(
            ["docker", "pull", registry_tag],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            logger.debug(f"Pull failed: {result.stderr}")
            return False

        # Tag as local image
        subprocess.run(
            ["docker", "tag", registry_tag, local_tag],
            check=True,
        )
        subprocess.run(
            ["docker", "tag", registry_tag, f"{FBOSS_BUILDER_IMAGE}:latest"],
            check=True,
        )

        logger.info(
            f"Successfully pulled {FBOSS_BUILDER_IMAGE}:{checksum[:12]} from registry"
        )
        return True

    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        logger.debug(f"Failed to pull from registry: {e}")
        return False


def _push_to_registry(checksum: str) -> None:
    """Push the builder image to the container registry.

    Pushes fboss_builder:latest (the actual built image) with the checksum tag.
    We don't push the timestamp-labeled image because that has an extra layer
    and we want to cache the exact image that was built.

    Args:
        checksum: The checksum tag to use in the registry
    """
    # Push the :latest image (the actual build output) with the checksum tag
    local_tag = f"{FBOSS_BUILDER_IMAGE}:latest"
    registry_tag = f"{CONTAINER_REGISTRY}/{FBOSS_BUILDER_IMAGE}:{checksum}"

    logger.info(f"Pushing {FBOSS_BUILDER_IMAGE}:{checksum[:12]} to registry...")

    try:
        # Tag for registry
        subprocess.run(
            ["docker", "tag", local_tag, registry_tag],
            check=True,
        )

        # Push to registry
        result = subprocess.run(
            ["docker", "push", registry_tag],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            logger.warning(f"Failed to push to registry: {result.stderr}")
            return

        logger.info(
            f"Successfully pushed {FBOSS_BUILDER_IMAGE}:{checksum[:12]} to registry"
        )

    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        logger.warning(f"Failed to push to registry: {e}")


def _should_build_image(root_dir: Path) -> tuple[bool, str, str]:
    """Determine if the fboss_builder image should be rebuilt.

    Args:
        root_dir: Repository root directory

    Returns:
        Tuple of (should_build, checksum, reason)
    """
    # Get cache expiration from environment variable (in hours)
    expiration_hours = int(os.getenv("FBOSS_BUILDER_CACHE_EXPIRATION_HOURS", "24"))
    expiration_seconds = expiration_hours * 60 * 60

    # Compute checksum of known dependencies
    logger.debug("Computing checksum of Dockerfile dependencies...")
    checksum = _compute_dependency_checksum(root_dir)
    checksum_tag = f"{FBOSS_BUILDER_IMAGE}:{checksum}"

    logger.debug(f"Dockerfile checksum: {checksum[:12]}")

    # Get the image timestamp once
    timestamp = _get_image_build_timestamp(checksum_tag)

    # If image doesn't exist, rebuild
    if timestamp is None:
        return (True, checksum, "not found")

    # Check if image is expired
    current_time = int(time.time())
    age_seconds = current_time - timestamp
    if age_seconds >= expiration_seconds:
        return (True, checksum, f"expired (>{expiration_hours}h old)")

    return (False, checksum, "exists and is not expired")


def build_fboss_builder_image() -> None:
    """Build the fboss_builder Docker image if needed.

    Uses a three-tier caching strategy:
    1. Local cache: Check if image with checksum tag exists locally
    2. Registry cache: Try to pull from container registry if not local
    3. Build: Build the image if not found in either cache, then push to registry

    Time-based expiration: Even if checksum matches, rebuilds if image is older
    than the expiration time (default: 24 hours, configurable via
    FBOSS_BUILDER_CACHE_EXPIRATION_HOURS environment variable).

    Raises:
        RuntimeError: If the build script is not found or build fails
    """

    # Find paths
    dockerfile = get_abs_path("fboss/oss/docker/Dockerfile")
    build_script = get_abs_path("fboss/oss/scripts/build_docker.sh")

    if not dockerfile.exists():
        raise RuntimeError(f"Dockerfile not found: {dockerfile}")

    if not build_script.exists():
        raise RuntimeError(f"Build script not found: {build_script}")

    # Check if we should rebuild (checks local cache)
    root_dir = get_abs_path(".")
    should_build, checksum, reason = _should_build_image(root_dir)

    if not should_build:
        logger.info(
            f"{FBOSS_BUILDER_IMAGE} image with checksum {checksum[:12]} "
            f"{reason}, skipping build"
        )
        return

    # Image not in local cache - try pulling from registry
    if _try_pull_from_registry(checksum):
        logger.info(
            f"{FBOSS_BUILDER_IMAGE} image with checksum {checksum[:12]} "
            f"pulled from registry, skipping build"
        )
        return

    logger.info(
        f"{FBOSS_BUILDER_IMAGE} image with checksum {checksum[:12]} "
        f"{reason} (not in registry either), building"
    )

    # Build the image
    logger.info(f"Building {FBOSS_BUILDER_IMAGE} image...")
    checksum_tag = f"{FBOSS_BUILDER_IMAGE}:{checksum}"

    try:
        # Build with label containing current timestamp
        current_timestamp = int(time.time())
        subprocess.run(
            [str(build_script)],
            check=True,
            cwd=str(root_dir),
            env={
                **os.environ,
                "DOCKER_BUILDKIT": "1",
                "BUILDKIT_PROGRESS": "plain",
            },
        )
        logger.info(f"Successfully built {FBOSS_BUILDER_IMAGE} image")

        # Tag with checksum and add timestamp label
        subprocess.run(
            ["docker", "tag", f"{FBOSS_BUILDER_IMAGE}:latest", checksum_tag], check=True
        )

        # Add build timestamp label to the checksum-tagged image
        # We do this by creating a new image with the label
        subprocess.run(
            [
                "docker",
                "build",
                "--label",
                f"build_timestamp={current_timestamp}",
                "--tag",
                checksum_tag,
                "-",
            ],
            input=f"FROM {FBOSS_BUILDER_IMAGE}:latest\n",
            text=True,
            check=True,
            capture_output=True,
        )

        logger.info(f"Tagged image with checksum: {checksum[:12]}")

        # Push to registry for future builds on other VMs
        _push_to_registry(checksum)

    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Failed to build {FBOSS_BUILDER_IMAGE} image: {e}") from e
