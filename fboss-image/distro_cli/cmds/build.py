# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Build command implementation."""

from pathlib import Path

from lib.builder import ImageBuilder
from lib.cli import validate_path
from lib.manifest import ImageManifest


def build_command(args):
    """Build FBOSS image or components"""
    manifest_path = Path(args.manifest)
    manifest_obj = ImageManifest(manifest_path)
    builder = ImageBuilder(manifest_obj)

    if args.components:
        builder.build_components(list(args.components))
    else:
        builder.build_all()


def setup_build_command(cli):
    """Setup the build command"""
    cli.add_command(
        "build",
        build_command,
        help_text="Build FBOSS image or components",
        arguments=[
            (
                "manifest",
                {
                    "type": lambda p: validate_path(p, must_exist=True),
                    "help": "Path to manifest JSON file",
                },
            ),
            (
                "components",
                {"nargs": "*", "help": "Specific components to build (default: all)"},
            ),
        ],
    )
