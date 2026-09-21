# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""CLI utilities for argparse-based command line tools."""

import argparse
from pathlib import Path


def validate_path(path_str, must_exist=False):
    """Validate and convert path string to Path object."""
    path = Path(path_str)
    if must_exist and not path.exists():
        raise argparse.ArgumentTypeError(f"Path does not exist: {path}")
    return path


class CommandGroup:
    """Helper for building command groups with subcommands."""

    def __init__(self, subparsers, name, help_text, arguments=None):
        self.parser = subparsers.add_parser(name, help=help_text)
        if arguments:
            for arg_name, arg_attrs in arguments:
                self.parser.add_argument(arg_name, **arg_attrs)
        self.subparsers = self.parser.add_subparsers(
            dest=f"{name}_command", required=True
        )

    def add_command(self, name, func, help_text=None, arguments=None):
        """Add a subcommand to this group."""
        parser = self.subparsers.add_parser(name, help=help_text or func.__doc__)
        if arguments:
            for arg_name, arg_attrs in arguments:
                parser.add_argument(arg_name, **arg_attrs)
        parser.set_defaults(func=func)
        return parser


class CLI:
    """Main CLI application builder."""

    def __init__(self, description, verbose_flag=True):
        self.parser = argparse.ArgumentParser(
            description=description,
            formatter_class=argparse.RawDescriptionHelpFormatter,
        )

        if verbose_flag:
            self.parser.add_argument(
                "-v",
                "--verbose",
                action="store_true",
                help="Enable verbose (DEBUG) logging",
            )

        self.subparsers = self.parser.add_subparsers(
            dest="command", required=True, help="Command to execute"
        )

    def add_command(self, name, func, help_text=None, arguments=None):
        """Add a simple command."""
        parser = self.subparsers.add_parser(name, help=help_text or func.__doc__)
        if arguments:
            for arg_name, arg_attrs in arguments:
                parser.add_argument(arg_name, **arg_attrs)
        parser.set_defaults(func=func)
        return parser

    def add_command_group(self, name, help_text, arguments=None):
        """Add a command group (parent command with subcommands)."""
        return CommandGroup(self.subparsers, name, help_text, arguments)

    def run(self, setup_logging_func=None):
        """Parse arguments and execute the command."""
        args = self.parser.parse_args()

        if setup_logging_func and hasattr(args, "verbose"):
            setup_logging_func(verbose=args.verbose)

        return args.func(args)
