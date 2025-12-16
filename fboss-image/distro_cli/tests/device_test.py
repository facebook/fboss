#!/usr/bin/env python3

# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""
Unit tests for device commands

NOTE: These are skeleton tests for stub implementations.
When device commands are fully implemented, these tests will be expanded
to verify actual functionality.

These tests verify that:
1. Device command group exists and has expected subcommands
2. Commands can be called without crashing (stub behavior)
3. Context passing works correctly
"""

import sys
import tempfile
import unittest
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

import argparse

from cmds.device import (
    getip_command,
    image_command,
    image_upstream_command,
    reprovision_command,
    setup_device_commands,
    ssh_command,
    update_command,
)


class TestDeviceCommands(unittest.TestCase):
    """Test device command group and subcommands (stubs)"""

    def setUp(self):
        """Set up test fixtures"""
        self.test_mac = "aa:bb:cc:dd:ee:ff"

        # Create a temporary manifest file for tests that need it
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            f.write('{"test": "manifest"}')
            self.manifest_path = Path(f.name)

        # Create a temporary image file for tests that need it
        with tempfile.NamedTemporaryFile(mode="w", suffix=".bin", delete=False) as f:
            f.write("fake image data")
            self.image_path = Path(f.name)

    def tearDown(self):
        """Clean up test fixtures"""
        self.manifest_path.unlink()
        self.image_path.unlink()

    def test_device_commands_exist(self):
        """Test that device commands exist"""
        self.assertTrue(callable(setup_device_commands))
        self.assertTrue(callable(image_upstream_command))
        self.assertTrue(callable(image_command))
        self.assertTrue(callable(reprovision_command))
        self.assertTrue(callable(update_command))
        self.assertTrue(callable(getip_command))
        self.assertTrue(callable(ssh_command))

    def test_image_upstream_stub(self):
        """Test image-upstream command (stub)"""
        args = argparse.Namespace(mac=self.test_mac, components=["kernel", "sai"])
        # Call command - just verify it doesn't crash
        image_upstream_command(args)

    def test_image_stub(self):
        """Test image command (stub)"""
        args = argparse.Namespace(mac=self.test_mac, image_path=str(self.image_path))
        # Call command - just verify it doesn't crash
        image_command(args)

    def test_reprovision_stub(self):
        """Test reprovision command (stub)"""
        args = argparse.Namespace(mac=self.test_mac)
        # Call command - just verify it doesn't crash
        reprovision_command(args)

    def test_update_stub(self):
        """Test update command (stub)"""
        args = argparse.Namespace(
            mac=self.test_mac,
            manifest=str(self.manifest_path),
            components=["kernel", "sai"],
        )
        # Call command - just verify it doesn't crash
        update_command(args)

    def test_getip_stub(self):
        """Test getip command (stub)"""
        args = argparse.Namespace(mac=self.test_mac)
        # Call command - just verify it doesn't crash
        getip_command(args)

    def test_ssh_stub(self):
        """Test ssh command (stub)"""
        args = argparse.Namespace(mac=self.test_mac)
        # Call command - just verify it doesn't crash
        ssh_command(args)


if __name__ == "__main__":
    unittest.main()
