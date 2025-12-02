#!/usr/bin/env python3

# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""
Unit tests for ImageManifest class
"""

import json
import sys
import tempfile
import unittest
from pathlib import Path

# Add parent directory to path to import the modules
test_dir = Path(__file__).parent
cli_dir = test_dir.parent
sys.path.insert(0, str(cli_dir))

from lib.manifest import ImageManifest


class TestImageManifest(unittest.TestCase):
    """Test ImageManifest class"""

    def setUp(self):
        """Use the test manifest from the tests directory"""
        self.test_dir = Path(__file__).parent
        self.manifest_path = self.test_dir / "dev_image.json"

        # Load the manifest data for validation
        with self.manifest_path.open() as f:
            self.manifest_data = json.load(f)

    def test_load_manifest(self):
        """Test that manifest loads correctly"""
        manifest = ImageManifest(self.manifest_path)
        self.assertEqual(manifest.data, self.manifest_data)

    def test_has_component(self):
        """Test that has_component correctly identifies present components"""
        manifest = ImageManifest(self.manifest_path)

        # All components should be present in the test manifest
        self.assertTrue(manifest.has_component("kernel"))
        self.assertTrue(manifest.has_component("other_dependencies"))
        self.assertTrue(manifest.has_component("fboss-platform-stack"))
        self.assertTrue(manifest.has_component("bsps"))
        self.assertTrue(manifest.has_component("sai"))
        self.assertTrue(manifest.has_component("fboss-forwarding-stack"))

        # Non-existent component should return False
        self.assertFalse(manifest.has_component("nonexistent"))

    def test_missing_components(self):
        """Test manifest with missing optional components"""
        minimal_manifest = {
            "distribution_formats": {"onie": "FBOSS-minimal.bin"},
            "kernel": {"download": "https://example.com/kernel.tar"},
        }

        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(minimal_manifest, f)
            minimal_path = Path(f.name)

        try:
            manifest = ImageManifest(minimal_path)
            # Should have kernel but not other optional components
            self.assertTrue(manifest.has_component("kernel"))
            self.assertFalse(manifest.has_component("other_dependencies"))
            self.assertFalse(manifest.has_component("fboss-platform-stack"))
            self.assertFalse(manifest.has_component("bsps"))
            self.assertFalse(manifest.has_component("sai"))
            self.assertFalse(manifest.has_component("fboss-forwarding-stack"))
        finally:
            minimal_path.unlink()

    def test_resolve_path_relative(self):
        """Test path resolution for relative paths"""
        manifest = ImageManifest(self.manifest_path)
        resolved = manifest.resolve_path("vendor_bsp/build.make")

        # Check that the path ends with the expected relative path
        # (works in both normal and Bazel sandbox environments)
        self.assertTrue(str(resolved).endswith("tests/vendor_bsp/build.make"))
        self.assertTrue(resolved.is_absolute())

    def test_resolve_path_url(self):
        """Test that URLs are not resolved as paths"""
        manifest = ImageManifest(self.manifest_path)
        url = "https://artifact.github.com/test.rpm"
        resolved = manifest.resolve_path(url)

        self.assertEqual(resolved, url)


if __name__ == "__main__":
    unittest.main()
