#!/usr/bin/env python3

# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""
Unit tests for ImageBuilder class

NOTE: These are skeleton tests for stub implementations.
When builder is fully implemented, these tests should be expanded.
"""

import sys
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

# Add parent directory to path to import the modules
test_dir = Path(__file__).parent
cli_dir = test_dir.parent
sys.path.insert(0, str(cli_dir))

from lib.builder import ImageBuilder
from lib.manifest import ImageManifest


class TestImageBuilder(unittest.TestCase):
    """Test ImageBuilder class"""

    def setUp(self):
        """Use the test manifest"""
        self.test_dir = Path(__file__).parent
        self.manifest_path = self.test_dir / "dev_image.json"
        self.manifest = ImageManifest(self.manifest_path)
        self.builder = ImageBuilder(self.manifest)

    def test_builder_initialization(self):
        """Test that builder initializes correctly"""
        self.assertIsNotNone(self.builder)
        self.assertEqual(self.builder.manifest, self.manifest)

    @patch("lib.builder.subprocess.run")
    @patch("lib.builder.Path.exists")
    @patch("lib.builder.shutil.move")
    def test_build_all_stub(self, _mock_move, mock_exists, mock_run):
        """Test build_all method (stub)"""
        # Mock the build script and output ISO existence
        mock_exists.return_value = True
        mock_run.return_value = MagicMock(returncode=0)

        # Just verify it doesn't crash
        # When implemented, this should verify actual build behavior
        self.builder.build_all()

    def test_build_components_stub(self):
        """Test build_components method (stub)"""
        # Just verify it doesn't crash
        # When implemented, this should verify component-specific builds
        components = ["kernel", "sai"]
        self.builder.build_components(components)


if __name__ == "__main__":
    unittest.main()
