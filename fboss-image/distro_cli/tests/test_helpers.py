# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Test helper utilities."""

import logging
import os
import shutil
import subprocess
import tempfile
import time
from collections.abc import Generator
from contextlib import contextmanager
from pathlib import Path

from distro_cli.lib.artifact import ArtifactStore
from distro_cli.lib.docker.image import build_fboss_builder_image

logger = logging.getLogger(__name__)


def ensure_test_docker_image():
    """Ensure fboss_builder Docker image is available for tests.

    Utilize the production build_fboss_builder_image() to ensure
    that the fboss builder image is available.

    Raises:
        RuntimeError: If image is not available.
    """
    build_fboss_builder_image()


@contextmanager
def enter_tempdir(prefix: str = "test_") -> Generator[Path, None, None]:
    """Create a temporary directory that works in sandbox environments.

    When TEST_TMPDIR environment variable is set (e.g., in sandbox tests), creates
    the temporary directory under TEST_TMPDIR which is guaranteed to be writable.
    Otherwise, uses the system's default temporary directory.

    Args:
        prefix: Prefix for the temporary directory name

    Yields:
        Path to the temporary directory

    Example:
        with enter_tempdir("my_test_") as tmpdir:
            test_file = tmpdir / "test.txt"
            test_file.write_text("test content")
    """
    # Determine the parent directory for the temp dir
    parent_dir = (
        Path(os.environ["TEST_TMPDIR"]) if "TEST_TMPDIR" in os.environ else None
    )

    # Create temp directory using tempfile (works in both cases)
    tmpdir_str = tempfile.mkdtemp(prefix=prefix, dir=parent_dir)
    tmpdir = Path(tmpdir_str)

    try:
        yield tmpdir
    finally:
        # Clean up
        if tmpdir.exists():
            shutil.rmtree(tmpdir)


@contextmanager
def override_artifact_store_dir(store_dir: Path) -> Generator[None, None, None]:
    """Temporarily override ArtifactStore.ARTIFACT_STORE_DIR for testing.

    This is necessary in sandboxed test environments where the default artifact
    store directory may be read-only. The override is automatically restored when
    the context exits.

    Args:
        store_dir: Path to use as the artifact store directory

    Yields:
        None

    Example:
        with enter_tempdir("artifacts_") as tmpdir:
            with override_artifact_store_dir(tmpdir):
                # ArtifactStore will now use tmpdir
                store = ArtifactStore()
                # ... test code ...
            # ArtifactStore.ARTIFACT_STORE_DIR is restored
    """
    original = ArtifactStore.ARTIFACT_STORE_DIR
    try:
        ArtifactStore.ARTIFACT_STORE_DIR = store_dir
        yield
    finally:
        ArtifactStore.ARTIFACT_STORE_DIR = original


@contextmanager
def get_writable_copy(
    source_dir: Path, prefix: str = "temp_"
) -> Generator[Path, None, None]:
    """Get a writable copy of a directory in a temporary location.

    This is essential for tests running in read-only sandbox environments
    where directories may be used to store intermediate build artifacts.

    The temporary directory is automatically cleaned up when the context exits.

    Args:
        source_dir: Path to the source directory to copy
        prefix: Prefix for the temporary directory name (default: "temp_")

    Yields:
        Path to the writable copy of the directory

    Example:
        # Get writable copy of integration test data
        integration_data = Path(__file__).parent / "test_topology" / "integration_data"
        with get_writable_copy(integration_data, "integration_") as temp_data:
            manifest = ImageManifest(temp_data / "integration_manifest.json")
            # Build scripts can now write to temp_data/dist/, temp_data/.build/, etc.
    """
    with enter_tempdir(prefix) as tmpdir:
        # Copy the entire source directory to temp location
        dest_dir = tmpdir / source_dir.name
        shutil.copytree(source_dir, dest_dir)

        # Make all shell scripts executable (common requirement)
        for script in dest_dir.rglob("*.sh"):
            script.chmod(0o755)

        yield dest_dir


def waitfor(condition_fn, assert_fn, timeout=60.0, interval=0.1):
    """Wait for a condition to become true with timeout.

    Repeatedly checks a condition function until it returns True or the timeout
    expires. If the timeout is reached, calls the assert function to fail the test.

    Args:
        condition_fn: Callable that returns True when the condition is met
        assert_fn: Callable to invoke if timeout expires (should fail the test)
        timeout: Maximum time to wait in seconds (default: 60.0)
        interval: Time to sleep between checks in seconds (default: 0.1)

    Example:
        waitfor(
            lambda: cache_dir.exists,
            lambda: self.fail("Cache directory not created"),
            timeout=30.0
        )
    """
    start = time.time()
    while True:
        if condition_fn():
            return
        if time.time() - start > timeout:
            assert_fn()
            raise AssertionError("assert_fn should not have returned!")
        time.sleep(interval)
