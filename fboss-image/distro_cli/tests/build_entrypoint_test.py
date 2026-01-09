# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Test build_entrypoint.py behavior."""

import subprocess
import tarfile
import unittest
from pathlib import Path

from distro_cli.lib.constants import FBOSS_BUILDER_IMAGE
from distro_cli.lib.docker.container import run_container
from distro_cli.lib.paths import get_abs_path
from distro_cli.tests.test_helpers import ensure_test_docker_image, enter_tempdir


class TestBuildEntrypoint(unittest.TestCase):
    """Test build_entrypoint.py as universal build entry point."""

    @classmethod
    def setUpClass(cls):
        """Ensure fboss_builder image exists before running tests."""
        ensure_test_docker_image()

    def test_entrypoint_without_dependencies(self):
        """Test build_entrypoint.py executes build command when no dependencies exist."""
        with enter_tempdir("entrypoint_no_deps_") as tmpdir_path:
            output_file = tmpdir_path / "build_output.txt"

            # Mount tools directory (contains build_entrypoint.py)
            # No /deps mount - simulates build without dependencies
            tools_dir = get_abs_path("fboss-image/distro_cli/tools")
            exit_code = run_container(
                image=FBOSS_BUILDER_IMAGE,
                command=[
                    "python3",
                    "/tools/build_entrypoint.py",
                    "sh",
                    "-c",
                    "echo 'build completed' > /output/build_output.txt",
                ],
                volumes={tools_dir: Path("/tools"), tmpdir_path: Path("/output")},
                ephemeral=True,
            )

            self.assertEqual(exit_code, 0, "Build should succeed without dependencies")
            self.assertTrue(output_file.exists(), "Build output should be created")
            self.assertEqual(output_file.read_text().strip(), "build completed")

    def test_entrypoint_with_empty_dependencies(self):
        """Test build_entrypoint.py handles empty /deps directory gracefully."""
        with enter_tempdir("entrypoint_empty_deps_") as tmpdir_path:
            output_file = tmpdir_path / "build_output.txt"
            deps_dir = tmpdir_path / "deps"
            deps_dir.mkdir(exist_ok=True)

            # Mount empty /deps directory
            tools_dir = get_abs_path("fboss-image/distro_cli/tools")
            exit_code = run_container(
                image=FBOSS_BUILDER_IMAGE,
                command=[
                    "python3",
                    "/tools/build_entrypoint.py",
                    "sh",
                    "-c",
                    "echo 'build with empty deps' > /output/build_output.txt",
                ],
                volumes={
                    tools_dir: Path("/tools"),
                    tmpdir_path: Path("/output"),
                    deps_dir: Path("/deps"),
                },
                ephemeral=True,
            )

            self.assertEqual(
                exit_code, 0, "Build should succeed with empty dependencies"
            )
            self.assertTrue(output_file.exists())
            self.assertEqual(output_file.read_text().strip(), "build with empty deps")

    def test_entrypoint_with_compressed_dependency(self):
        """Test build_entrypoint.py can extract zstd-compressed dependency tarballs."""
        with enter_tempdir("entrypoint_compressed_deps_") as tmpdir_path:
            # Create a dummy file to include in the tarball
            deps_content_dir = tmpdir_path / "deps_content"
            deps_content_dir.mkdir()
            dummy_file = deps_content_dir / "test.txt"
            dummy_file.write_text("compressed dependency content")

            # Create uncompressed tarball first
            uncompressed_tar = tmpdir_path / "dependency.tar"
            with tarfile.open(uncompressed_tar, "w") as tar:
                tar.add(dummy_file, arcname="test.txt")

            # Compress with zstd
            compressed_tar = tmpdir_path / "dependency.tar.zst"
            subprocess.run(
                ["zstd", str(uncompressed_tar), "-o", str(compressed_tar)],
                check=True,
                capture_output=True,
            )
            uncompressed_tar.unlink()  # Remove uncompressed version

            # Create deps directory and move compressed tarball there
            deps_dir = tmpdir_path / "deps"
            deps_dir.mkdir()
            final_dep = deps_dir / "dependency.tar.zst"
            compressed_tar.rename(final_dep)

            output_file = tmpdir_path / "build_output.txt"

            # Run build_entrypoint.py with compressed dependency
            tools_dir = get_abs_path("fboss-image/distro_cli/tools")
            exit_code = run_container(
                image=FBOSS_BUILDER_IMAGE,
                command=[
                    "python3",
                    "/tools/build_entrypoint.py",
                    "sh",
                    "-c",
                    "echo 'build with compressed deps' > /output/build_output.txt",
                ],
                volumes={
                    tools_dir: Path("/tools"),
                    tmpdir_path: Path("/output"),
                    deps_dir: Path("/deps"),
                },
                ephemeral=True,
            )

            self.assertEqual(
                exit_code, 0, "Build should succeed with compressed dependencies"
            )
            self.assertTrue(output_file.exists())
            self.assertEqual(
                output_file.read_text().strip(), "build with compressed deps"
            )

    def test_entrypoint_with_uncompressed_dependency(self):
        """Test build_entrypoint.py can extract uncompressed dependency tarballs."""
        with enter_tempdir("entrypoint_uncompressed_deps_") as tmpdir_path:
            # Create a dummy file to include in the tarball
            deps_content_dir = tmpdir_path / "deps_content"
            deps_content_dir.mkdir()
            dummy_file = deps_content_dir / "test.txt"
            dummy_file.write_text("uncompressed dependency content")

            # Create uncompressed tarball
            deps_dir = tmpdir_path / "deps"
            deps_dir.mkdir()
            uncompressed_tar = deps_dir / "dependency.tar"
            with tarfile.open(uncompressed_tar, "w") as tar:
                tar.add(dummy_file, arcname="test.txt")

            output_file = tmpdir_path / "build_output.txt"

            # Run build_entrypoint.py with uncompressed dependency
            tools_dir = get_abs_path("fboss-image/distro_cli/tools")
            exit_code = run_container(
                image=FBOSS_BUILDER_IMAGE,
                command=[
                    "python3",
                    "/tools/build_entrypoint.py",
                    "sh",
                    "-c",
                    "echo 'build with uncompressed deps' > /output/build_output.txt",
                ],
                volumes={
                    tools_dir: Path("/tools"),
                    tmpdir_path: Path("/output"),
                    deps_dir: Path("/deps"),
                },
                ephemeral=True,
            )

            self.assertEqual(
                exit_code, 0, "Build should succeed with uncompressed dependencies"
            )
            self.assertTrue(output_file.exists())
            self.assertEqual(
                output_file.read_text().strip(), "build with uncompressed deps"
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
