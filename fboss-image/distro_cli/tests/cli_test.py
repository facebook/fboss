# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""
Tests for CLI utilities
"""

import argparse
import sys
import tempfile
import unittest
from pathlib import Path

# Add parent directory to path for imports
test_dir = Path(__file__).parent
cli_dir = test_dir.parent
sys.path.insert(0, str(cli_dir))

from lib.cli import CLI, CommandGroup, validate_path


class ValidatePathTest(unittest.TestCase):
    """Test validate_path function"""

    def test_validate_path_converts_to_path(self):
        """Test that validate_path converts string to Path"""
        result = validate_path("/tmp/test")
        self.assertIsInstance(result, Path)
        self.assertEqual(result, Path("/tmp/test"))

    def test_validate_path_existing_file(self):
        """Test validate_path with must_exist=True for existing file"""
        with tempfile.NamedTemporaryFile(delete=False) as f:
            temp_path = f.name

        try:
            result = validate_path(temp_path, must_exist=True)
            self.assertEqual(result, Path(temp_path))
        finally:
            Path(temp_path).unlink()

    def test_validate_path_nonexistent_file_raises(self):
        """Test validate_path with must_exist=True for nonexistent file"""
        with self.assertRaises(argparse.ArgumentTypeError) as ctx:
            validate_path("/nonexistent/path/file.txt", must_exist=True)
        self.assertIn("does not exist", str(ctx.exception))

    def test_validate_path_nonexistent_file_without_check(self):
        """Test validate_path without must_exist for nonexistent file"""
        result = validate_path("/nonexistent/path/file.txt", must_exist=False)
        self.assertEqual(result, Path("/nonexistent/path/file.txt"))


class CommandGroupTest(unittest.TestCase):
    """Test CommandGroup class"""

    def setUp(self):
        """Set up test fixtures"""
        parser = argparse.ArgumentParser()
        self.subparsers = parser.add_subparsers()

    def test_command_group_creation(self):
        """Test creating a command group"""
        group = CommandGroup(self.subparsers, "test", help_text="Test group")
        self.assertIsNotNone(group.parser)
        self.assertIsNotNone(group.subparsers)

    def test_command_group_with_arguments(self):
        """Test command group with arguments"""
        group = CommandGroup(
            self.subparsers,
            "device",
            help_text="Device commands",
            arguments=[("mac", {"help": "MAC address"})],
        )
        self.assertIsNotNone(group.parser)

    def test_add_command_to_group(self):
        """Test adding a command to a group"""

        def test_func(args):
            pass

        group = CommandGroup(self.subparsers, "test", help_text="Test group")
        parser = group.add_command("subtest", test_func, help_text="Sub command")
        self.assertIsNotNone(parser)

    def test_add_command_with_arguments(self):
        """Test adding a command with arguments"""

        def test_func(args):
            pass

        group = CommandGroup(self.subparsers, "test", help_text="Test group")
        parser = group.add_command(
            "subtest",
            test_func,
            help_text="Sub command",
            arguments=[("arg1", {"help": "Argument 1"})],
        )
        self.assertIsNotNone(parser)


class CLITest(unittest.TestCase):
    """Test CLI class"""

    def test_cli_creation(self):
        """Test creating a CLI instance"""
        cli = CLI(description="Test CLI")
        self.assertIsNotNone(cli.parser)
        self.assertIsNotNone(cli.subparsers)

    def test_cli_with_verbose_flag(self):
        """Test CLI with verbose flag enabled"""

        def test_command(args):
            pass

        cli = CLI(description="Test CLI", verbose_flag=True)
        cli.add_command("test", test_command)
        args = cli.parser.parse_args(["--verbose", "test"])
        self.assertTrue(args.verbose)

    def test_cli_without_verbose_flag(self):
        """Test CLI without verbose flag"""
        cli = CLI(description="Test CLI", verbose_flag=False)
        self.assertIsNotNone(cli.parser)

    def test_add_command(self):
        """Test adding a simple command"""

        def test_command(args):
            pass

        cli = CLI(description="Test CLI")
        parser = cli.add_command("test", test_command, help_text="Test command")
        self.assertIsNotNone(parser)

    def test_add_command_with_arguments(self):
        """Test adding a command with arguments"""

        def test_command(args):
            pass

        cli = CLI(description="Test CLI")
        parser = cli.add_command(
            "test",
            test_command,
            help_text="Test command",
            arguments=[("arg1", {"help": "Argument 1"})],
        )
        self.assertIsNotNone(parser)

    def test_add_command_group(self):
        """Test adding a command group"""
        cli = CLI(description="Test CLI")
        group = cli.add_command_group("device", help_text="Device commands")
        self.assertIsInstance(group, CommandGroup)


if __name__ == "__main__":
    unittest.main()
