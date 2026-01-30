# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Image Builder - handles building FBOSS images from manifests."""

import logging
import shutil
import subprocess
import sys
from pathlib import Path
from typing import ClassVar

logger = logging.getLogger(__name__)


class ImageBuilder:
    """Handles building FBOSS images from manifests."""

    # Component list - build order based on dependencies
    # TODO: Convert to DAG for parallel builds and easier extensibility
    COMPONENTS: ClassVar[list[str]] = [
        "kernel",
        "other_dependencies",
        "fboss-platform-stack",
        "bsps",
        "sai",
        "fboss-forwarding-stack",
    ]

    def __init__(self, manifest):
        self.manifest = manifest
        self.workspace_root = manifest.manifest_dir

    def build_all(self):
        """Build all components and create distribution artifacts."""
        logger.info("Building FBOSS Image")

        # Build components in dependency order (if present in manifest)
        for component in self.COMPONENTS:
            if self.manifest.has_component(component):
                self._build_component(component)

        self._build_base_image()

    def build_components(self, component_names: list[str]):
        """Build specific components only."""
        logger.info(f"Building components: {', '.join(component_names)}")

        # Validate all requested components exist in manifest
        for component in component_names:
            if not self.manifest.has_component(component):
                logger.error(f"Component '{component}' not found in manifest")
                sys.exit(1)

        # Build requested components in dependency order
        for component in self.COMPONENTS:
            if component in component_names:
                self._build_component(component)

    def _mv_distro_file(
        self, image_builder_dir: Path, format_name: str, file_extension: str
    ):
        dist_formats = self.manifest.data.get("distribution_formats")
        if not dist_formats or format_name not in dist_formats:
            return

        output = (
            image_builder_dir
            / "output"
            / f"FBOSS-Distro-Image.x86_64-1.0.install.{file_extension}"
        )
        image = Path(dist_formats[format_name])

        if not output.exists():
            logger.error(f"Image build output not found: {output}")
            sys.exit(1)

        shutil.move(str(output), str(image))

    def _build_base_image(self):
        """Build the base OS image and create distribution artifacts."""
        logger.info("Starting base OS image build")

        # Validate distribution formats are specified
        dist_formats = self.manifest.data.get("distribution_formats")
        if not dist_formats:
            logger.error("No distribution formats specified in manifest")
            sys.exit(1)

        if not any(k in dist_formats for k in ["usb", "pxe"]):
            logger.error(
                "Neither usb nor pxe distribution format specified in manifest."
            )
            logger.error(
                f"Manifest distribution formats: {', '.join(dist_formats.keys())}"
            )
            sys.exit(1)

        # Locate the image builder directory
        script_dir = Path(__file__).parent.resolve()
        distro_cli_dir = script_dir.parent
        fboss_image_dir = distro_cli_dir.parent
        image_builder_dir = fboss_image_dir / "image_builder"
        build_script = image_builder_dir / "bin" / "build_image.sh"

        if not build_script.exists():
            logger.error(f"Build script not found: {build_script}")
            sys.exit(1)

        logger.info(f"Using image builder: {image_builder_dir}")
        logger.info(f"Running build script: {build_script}")

        # Run the build script
        cmd = [str(build_script)]
        if "pxe" in dist_formats or "usb" in dist_formats:
            cmd.append("--build-pxe-usb")
        if "onie" in dist_formats:
            cmd.append("--build-onie")
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            logger.error(f"Build script failed with exit code {e.returncode}")
            sys.exit(1)

        self._mv_distro_file(image_builder_dir, "usb", "iso")
        self._mv_distro_file(image_builder_dir, "pxe", "tar")
        self._mv_distro_file(image_builder_dir, "onie", "bin")

        logger.info("Finished base OS image build")

    def _build_component(self, component: str):
        """Build a specific component."""
        logger.info(f"Building: {component}")
        logger.info(f"Done building: {component} (stub)")
