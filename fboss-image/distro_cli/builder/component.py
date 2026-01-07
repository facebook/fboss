# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Generic component builder for FBOSS image components."""

import hashlib
import logging
from pathlib import Path

from distro_cli.lib.artifact import find_artifact_in_dir
from distro_cli.lib.constants import FBOSS_BUILDER_IMAGE
from distro_cli.lib.download import download_artifact
from distro_cli.lib.exceptions import ComponentError
from distro_cli.lib.execute import execute_build_in_container

logger = logging.getLogger(__name__)


class ComponentBuilder:
    """Generic builder for FBOSS image components.

    Supports two modes:
    - download: Download pre-built artifact from URL
    - execute: Build component using a build script in Docker container

    Component-specific logic (argument parsing, paths, etc.) should be
    embedded in the component build script, not in this builder.
    """

    def __init__(
        self,
        component_name: str,
        component_data: dict,
        manifest_dir: Path,
        store,
        root_dir: Path | None = None,
        build_artifact_subdir: str | None = None,
        artifact_pattern: str | None = None,
        dependency_artifacts: dict[str, Path] | None = None,
    ):
        """Initialize the component builder.

        Args:
            component_name: Name of the component
            component_data: Component data dict from manifest
            manifest_dir: Path to the manifest directory
            store: ArtifactStore instance
            root_dir: Path to the root directory (workspace root).
                         If None, component cannot use execute mode
            build_artifact_subdir: Subpath under root_dir where .build and dist directories
                         will be created (e.g., "fboss-image/kernel").
                         If None, component cannot use execute mode
            artifact_pattern: Glob pattern for finding build artifacts (e.g., "kernel-*.rpms.tar.gz")
                             If None, component cannot use execute mode
            dependency_artifacts: Optional dict mapping dependency names to their artifact paths
        """
        self.component_name = component_name
        self.component_data = component_data
        self.manifest_dir = manifest_dir
        self.store = store
        self.root_dir = root_dir
        self.build_artifact_subdir = build_artifact_subdir
        self.artifact_pattern = artifact_pattern
        self.dependency_artifacts = dependency_artifacts or {}

    def build(self) -> Path:
        """Build or download the component.

        Returns:
            Path to the component artifact (or None for empty components)

        Raises:
            ComponentError: If component has invalid structure
        """
        if self.component_data is None:
            raise ComponentError(f"Component '{self.component_name}' has no data")

        # ComponentBuilder handles single component instances only
        # Array components should be handled at a higher level (e.g., ImageBuilder)
        # by creating one ComponentBuilder instance per array element
        if isinstance(self.component_data, list):
            raise ComponentError(
                f"Component '{self.component_name}' data is an array. "
                "ComponentBuilder only handles single component instances. "
                "Create one ComponentBuilder per array element instead."
            )

        # Check for both download and execute (invalid)
        has_download = "download" in self.component_data
        has_execute = "execute" in self.component_data

        if has_download and has_execute:
            raise ComponentError(
                f"Component '{self.component_name}' has both 'download' and 'execute' fields. "
                "Only one is allowed."
            )

        # Allow empty components
        if not has_download and not has_execute:
            logger.info(f"Component '{self.component_name}' is empty, skipping")
            return None

        if has_download:
            return self._download_component(self.component_data["download"])

        # this must be an "execute"
        # Use artifact pattern from manifest if specified, otherwise use default
        artifact_pattern = self.component_data.get("artifact", self.artifact_pattern)
        return self._execute_component(self.component_data["execute"], artifact_pattern)

    def _download_component(self, url: str) -> Path:
        """Download component artifact from URL.

        Args:
            url: URL to download from

        Returns:
            Path to downloaded artifact (cached)
        """
        store_key = (
            f"{self.component_name}-download-{hashlib.sha256(url.encode()).hexdigest()}"
        )
        data_files, metadata_files = self.store.get(
            store_key,
            lambda data, meta: download_artifact(url, self.manifest_dir, data, meta),
        )

        if not data_files:
            raise ComponentError(f"No artifact files found for {self.component_name}")

        if len(data_files) != 1:
            raise ComponentError(
                f"Expected exactly 1 data file for {self.component_name}, got {len(data_files)}"
            )

        artifact_path = data_files[0]
        logger.info(f"{self.component_name} artifact ready: {artifact_path}")
        if metadata_files:
            logger.debug(f"  with {len(metadata_files)} metadata file(s)")
        return artifact_path

    def _execute_component(
        self, cmd_line: str | list[str], artifact_pattern: str | None = None
    ) -> Path:
        """Execute component build in Docker container.

        Args:
            cmd_line: Command line to execute (string or list of strings)
            artifact_pattern: Optional artifact pattern override from manifest

        Returns:
            Path to build artifact in cache
        """
        # For store key, convert to string (works for both str and list)
        store_key_str = str(cmd_line)

        # _execute_build as a fetch_fn always starts a build expecting the underlying
        # build system to provide build specific optimizations. The objects are returned
        # back to the store with a store-miss indication.
        store_key = f"{self.component_name}-build-{hashlib.sha256(store_key_str.encode()).hexdigest()[:8]}"
        data_files, _ = self.store.get(
            store_key,
            lambda _data, _meta: (
                False,
                [self._execute_build(cmd_line, artifact_pattern)],
                [],
            ),
        )

        if not data_files:
            raise ComponentError(f"No artifact files found for {self.component_name}")

        if len(data_files) != 1:
            raise ComponentError(
                f"Expected exactly 1 data file for {self.component_name}, got {len(data_files)}"
            )

        artifact_path = data_files[0]
        logger.info(f"{self.component_name} build complete: {artifact_path}")
        return artifact_path

    def _execute_build(
        self, cmd_line: str | list[str], artifact_pattern: str | None = None
    ) -> Path:
        """Execute build in Docker container.

        Args:
            cmd_line: Command line to execute (string or list of strings)
            artifact_pattern: Optional artifact pattern override from manifest

        Returns:
            Path to build artifact

        Raises:
            ComponentError: If build fails or artifact not found
        """
        if not self.root_dir:
            raise ComponentError(
                f"Component '{self.component_name}' cannot use execute mode. "
                "No root_dir specified."
            )

        # Create build and dist directories under build_artifact_subdir
        if self.build_artifact_subdir:
            artifact_base_dir = self.root_dir / self.build_artifact_subdir
        else:
            artifact_base_dir = self.root_dir
            logger.warning(
                f"Component '{self.component_name}' has no build_artifact_subdir specified. "
                f"Using root directory: {artifact_base_dir}"
            )

        # Use artifact_pattern from parameter, or fall back to instance pattern, or use generic pattern
        if artifact_pattern is None:
            artifact_pattern = self.artifact_pattern or "*.tar.gz"
        if not artifact_pattern:
            logger.warning(
                f"Component '{self.component_name}' has no artifact_pattern specified. "
                f"Using generic pattern: {artifact_pattern}"
            )

        build_dir = artifact_base_dir / ".build"
        build_dir.mkdir(parents=True, exist_ok=True)

        dist_dir = artifact_base_dir / "dist"
        dist_dir.mkdir(parents=True, exist_ok=True)

        volumes = {
            self.root_dir: Path("/workspace"),
            build_dir: Path("/build"),
            dist_dir: Path("/output"),
        }

        # Mount dependency artifacts into the container
        # Each dependency is mounted at /dependencies/{dep_name}/
        # The build_entrypoint.py will handle extraction and RPM installation
        dependency_install_paths = {}
        for dep_name, dep_artifact in self.dependency_artifacts.items():
            dep_mount_point = Path(f"/dependencies/{dep_name}")
            volumes[dep_artifact] = dep_mount_point
            dependency_install_paths[dep_name] = dep_mount_point
            logger.info(
                f"Mounting dependency '{dep_name}' at {dep_mount_point}: {dep_artifact}"
            )

        # Working directory is always /workspace (root)
        # Execute command paths are relative to /workspace
        working_dir = "/workspace"

        # Normalize command to list (handle both string and list from manifest)
        cmd_list = [cmd_line] if isinstance(cmd_line, str) else cmd_line

        # Execute build command
        execute_build_in_container(
            image_name=FBOSS_BUILDER_IMAGE,
            command=cmd_list,
            volumes=volumes,
            component_name=self.component_name,
            privileged=False,
            working_dir=working_dir,
            dependency_install_paths=(
                dependency_install_paths if dependency_install_paths else None
            ),
        )

        return find_artifact_in_dir(
            output_dir=dist_dir,
            pattern=artifact_pattern,
            component_name=self.component_name.capitalize(),
        )
