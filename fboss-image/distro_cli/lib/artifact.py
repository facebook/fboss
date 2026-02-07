# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Artifact storage with file caching for FBOSS image builder."""

import hashlib
import logging
import shutil
import tempfile
from collections.abc import Callable
from pathlib import Path

from distro_cli.lib.paths import get_abs_path

from .exceptions import ArtifactError

logger = logging.getLogger(__name__)


def get_artifact_store_dir() -> Path:
    """Get the default artifact store directory.

    Returns:
        Path to the default artifact store directory.
        Falls back to a temp directory if git repository is not available.
    """
    try:
        return get_abs_path("fboss-image/distro_cli/.artifacts")
    except RuntimeError:
        # Not in a git repository (e.g., CMake build copies code outside git)
        # Use a temp directory instead
        temp_base = Path(tempfile.gettempdir()) / "fboss-distro-cli-artifacts"
        temp_base.mkdir(parents=True, exist_ok=True)
        logger.warning(
            f"Git repository not found, using temp directory for artifacts: {temp_base}"
        )
        return temp_base


class ArtifactStore:
    """Artifact storage with external cache evaluation.

    get() delegates cache evaluation and related fetching to a caller-provided function.
    store() persists data and metadata files separately in storage subdirectories.
    """

    # Artifact store directory - class attribute (lazy initialization)
    # Can be overridden by tests before creating ArtifactStore instances
    ARTIFACT_STORE_DIR: Path | None = None

    def __init__(self):
        """Initialize artifact store."""
        # Use class attribute if set, otherwise compute default
        if self.ARTIFACT_STORE_DIR is None:
            self.store_dir = get_artifact_store_dir()
        else:
            self.store_dir = self.ARTIFACT_STORE_DIR
        self.store_dir.mkdir(parents=True, exist_ok=True)
        logger.debug(f"Artifact store initialized at: {self.store_dir}")

    def get(
        self,
        store_key: str,
        fetch_fn: Callable[
            [list[Path], list[Path]], tuple[bool, list[Path], list[Path]]
        ],
    ) -> tuple[list[Path], list[Path]]:
        """Retrieve artifact files using caller-provided fetch function.

        Args:
            store_key: Unique identifier for the artifact
            fetch_fn: Evaluates stored files and returns (store_hit, data_files, metadata_files)

        Returns:
            Tuple of (data_files, metadata_files)
        """
        store_subdir = self._get_store_subdir(store_key)
        stored_data_files = self._get_stored_files_in_dir(store_subdir / "data")
        stored_metadata_files = self._get_stored_files_in_dir(store_subdir / "metadata")

        logger.info(f"Executing fetch function for: {store_key}")
        store_hit, new_data_files, new_metadata_files = fetch_fn(
            stored_data_files, stored_metadata_files
        )

        if store_hit:
            logger.info(f"Store hit: {store_key}")
            return (stored_data_files, stored_metadata_files)

        logger.info(f"Store miss: {store_key}, storing new files")
        return self.store(store_key, new_data_files, new_metadata_files)

    def _get_store_subdir(self, store_key: str) -> Path:
        """Get the storage subdirectory for a given store key.

        Args:
            store_key: Store key for the artifact

        Returns:
            Path to the storage subdirectory
        """
        # Use full SHA256 hash to create a directory name
        key_hash = hashlib.sha256(store_key.encode()).hexdigest()
        return self.store_dir / key_hash

    def _get_stored_files_in_dir(self, dir_path: Path) -> list[Path]:
        """Get files from a directory.

        Args:
            dir_path: Directory path

        Returns:
            List of file paths
        """
        if not dir_path.exists():
            return []
        return [f for f in dir_path.iterdir() if f.is_file()]

    def store(
        self, store_key: str, data_files: list[Path], metadata_files: list[Path]
    ) -> tuple[list[Path], list[Path]]:
        """Store data and metadata files in the storage.

        Files/directories are moved to store_subdir/data/ and store_subdir/metadata/.
        If a path is a file, it's moved directly.
        If a path is a directory, all its contents are moved.

        Args:
            store_key: Store key for the artifact
            data_files: List of data file/directory paths to store
            metadata_files: List of metadata file/directory paths to store

        Returns:
            Tuple of (stored_data_files, stored_metadata_files)
        """
        store_subdir = self._get_store_subdir(store_key)
        data_dir = store_subdir / "data"
        metadata_dir = store_subdir / "metadata"

        # Store data files
        if data_files:
            # Replace any previously stored data for this key so we don't mix
            # old and new artifacts (e.g., uncompressed + compressed variants).
            if data_dir.exists():
                shutil.rmtree(data_dir)
            data_dir.mkdir(parents=True, exist_ok=True)
            for file_path in data_files:
                self._move_to_dir(file_path, data_dir)
            logger.info(f"Stored {len(data_files)} data file(s): {store_key}")

        # Store metadata files
        if metadata_files:
            # Likewise, keep metadata for this key in a clean directory so callers
            # always see the current set from the latest operation.
            if metadata_dir.exists():
                shutil.rmtree(metadata_dir)
            metadata_dir.mkdir(parents=True, exist_ok=True)
            for file_path in metadata_files:
                self._move_to_dir(file_path, metadata_dir)
            logger.info(f"Stored {len(metadata_files)} metadata file(s): {store_key}")

        # Return all stored files (after updating the directories)
        return (
            self._get_stored_files_in_dir(data_dir),
            self._get_stored_files_in_dir(metadata_dir),
        )

    def _move_to_dir(self, source: Path, dest_dir: Path) -> None:
        """Move a file or directory contents to destination directory.

        Uses move instead of copy for better performance when source and dest
        are on the same filesystem.

        Args:
            source: Source file or directory
            dest_dir: Destination directory
        """
        dest_path = dest_dir / source.name
        if source.is_dir():
            # For directories, move the entire tree
            if dest_path.exists():
                shutil.rmtree(dest_path)
            shutil.move(str(source), str(dest_path))
        else:
            # For files, move directly
            shutil.move(str(source), str(dest_path))

    def invalidate(self, store_key: str) -> None:
        """Remove an artifact from the store.

        Args:
            store_key: Store key for the artifact to remove
        """
        store_subdir = self._get_store_subdir(store_key)
        if store_subdir.exists():
            shutil.rmtree(store_subdir)
            logger.info(f"Invalidated store entry: {store_key}")

    def clear(self) -> None:
        """Clear all stored artifacts."""
        if self.store_dir.exists():
            shutil.rmtree(self.store_dir)
            self.store_dir.mkdir(parents=True, exist_ok=True)
            logger.info("All stored artifacts cleared")

    @classmethod
    def create_temp_dir(cls, prefix: str = "temp-") -> Path:
        """Create a temporary directory within the artifact store.

        Creates temp directory on same filesystem as artifact store to enable
        fast atomic moves and avoid filling up /tmp. Useful for parallel builds
        that need isolation.

        This is a class method so it can be called without an instance.

        Args:
            prefix: Prefix for the temporary directory name

        Returns:
            Path to the created temporary directory
        """
        # Get artifact store directory
        store_dir = (
            get_artifact_store_dir()
            if cls.ARTIFACT_STORE_DIR is None
            else cls.ARTIFACT_STORE_DIR
        )

        temp_base = store_dir / ".tmp"
        temp_base.mkdir(parents=True, exist_ok=True)
        return Path(tempfile.mkdtemp(dir=temp_base, prefix=prefix))

    @staticmethod
    def delete_temp_dir(temp_dir: Path) -> None:
        """Delete a temporary directory created by create_temp_dir().

        This is a static method so it can be called without an instance.

        Args:
            temp_dir: Path to the temporary directory to delete
        """
        if temp_dir.exists():
            shutil.rmtree(temp_dir, ignore_errors=True)
            logger.debug(f"Deleted temporary directory: {temp_dir}")


def find_artifact_in_dir(
    output_dir: Path, pattern: str, component_name: str = "Component"
) -> Path:
    """Find a single artifact matching a glob pattern in a directory.

    Supports both uncompressed (.tar) and zstd-compressed (.tar.zst) variants.

    Args:
        output_dir: Directory to search in
        pattern: Glob pattern to match (e.g., "kernel-*.rpms.tar")
        component_name: Name of component for error messages

    Returns:
        Path to the found artifact

    Raises:
        ArtifactError: If no artifacts found

    Note:
        If multiple artifacts match, returns the most recent one based on modification time.
    """
    # Find both uncompressed and compressed versions
    artifacts = list(output_dir.glob(pattern)) + list(output_dir.glob(f"{pattern}.zst"))

    if not artifacts:
        raise ArtifactError(
            f"{component_name} build output not found in: {output_dir} "
            f"(patterns: {pattern}, {pattern}.zst)"
        )

    # If multiple artifacts found, use the most recent one based on modification time
    if len(artifacts) > 1:
        artifacts.sort(key=lambda p: p.stat().st_mtime, reverse=True)
        logger.warning(
            f"Multiple artifacts found matching '{pattern}' or '{pattern}.zst', "
            f"using most recent: {artifacts[0]}"
        )

    logger.info(f"Found {component_name} artifact: {artifacts[0]}")
    return artifacts[0]
