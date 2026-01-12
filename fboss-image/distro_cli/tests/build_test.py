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
import sys
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

# Add parent directory to path to import the modules
test_dir = Path(__file__).parent
cli_dir = test_dir.parent
sys.path.insert(0, str(cli_dir))

from cmds.build import build_command, setup_build_command


class TestBuildCommand(unittest.TestCase):
    """Test build command"""

    def setUp(self):
        """Set up test fixtures"""
        self.test_dir = Path(__file__).parent
        self.manifest_path = self.test_dir / "dev_image.json"

    def test_build_command_exists(self):
        """Test that build command exists"""
        self.assertTrue(callable(build_command))
        self.assertTrue(callable(setup_build_command))

    @patch("lib.builder.subprocess.run")
    @patch("lib.builder.Path.exists")
    @patch("lib.builder.shutil.move")
    def test_build_all_stub(self, _mock_move, mock_exists, mock_run):
        """Test build command with no components (build all)"""
        # Mock the build script and output ISO existence
        mock_exists.return_value = True
        mock_run.return_value = MagicMock(returncode=0)

        # Create mock args object
        args = argparse.Namespace(manifest=str(self.manifest_path), components=[])
        # Call build command - just verify it doesn't crash
        # When implemented, should verify full image build
        build_command(args)

    def test_build_specific_components_stub(self):
        """Test build command with specific components"""
        # Create mock args object
        args = argparse.Namespace(
            manifest=str(self.manifest_path), components=["kernel", "sai"]
        )
        # Call build command with components - just verify it doesn't crash
        # When implemented, should verify component-specific builds
        build_command(args)


if __name__ == "__main__":
    unittest.main()
