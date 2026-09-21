#!/usr/bin/env python3

# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""
Unit tests for build command

NOTE: These are skeleton tests for stub implementations.
When build command is fully implemented, these tests should be expanded.
"""

import argparse
import unittest
from pathlib import Path

from distro_cli.cmds.build import build_command, setup_build_command
from distro_cli.tests.test_helpers import (
    ensure_test_docker_image,
    enter_tempdir,
    override_artifact_store_dir,
)


class TestBuildCommand(unittest.TestCase):
    """Test build command"""

    def setUp(self):
        """Set up test fixtures"""
        self.test_dir = Path(__file__).parent / "data"
        self.manifest_path = self.test_dir / "dev_image.json"
        ensure_test_docker_image()

    def test_build_command_exists(self):
        """Test that build command exists"""
        self.assertTrue(callable(build_command))
        self.assertTrue(callable(setup_build_command))

    def test_build_components(self):
        """Test build command with specific components using stub manifest."""
        with (
            enter_tempdir("build_test_artifacts_") as temp_artifacts,
            override_artifact_store_dir(temp_artifacts),
        ):
            manifest_path = self.test_dir / "test-stub-component.json"

            args = argparse.Namespace(
                manifest=str(manifest_path),
                components=["kernel"],
                kiwi_ng_debug=False,
            )

            build_command(args)

            # Verify the artifact was created in the artifact store
            self.assertTrue(
                temp_artifacts.exists(),
                f"Artifacts directory not found: {temp_artifacts}",
            )

            # Find the artifact in the store (supports both .tar and .tar.zst)
            matching_files = list(temp_artifacts.glob("*/data/kernel-test.rpms.tar*"))
            self.assertTrue(
                len(matching_files) > 0,
                f"Expected artifact file not found in artifact store: {temp_artifacts}",
            )


if __name__ == "__main__":
    unittest.main()
