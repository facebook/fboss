# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Device command implementation."""

import logging

from lib.cli import validate_path

logger = logging.getLogger("fboss-image")


def image_upstream_command(args):
    """Download full image from upstream repository and set it to be loaded onto device"""
    logger.info(f"Setting upstream image for device {args.mac}")
    logger.info("Device image-upstream command (stub)")


def image_command(args):
    """Set device image from file"""
    logger.info(f"Setting image for device {args.mac}: {args.image_path}")
    logger.info("Device image command (stub)")


def reprovision_command(args):
    """Reprovision device"""
    logger.info(f"Reprovisioning device {args.mac}")
    logger.info("Device reprovision command (stub)")


def update_command(args):
    """Update specific components on device"""
    logger.info(f"Updating device {args.mac}")
    logger.info(f"Manifest: {args.manifest}")
    logger.info(f"Components: {' '.join(args.components)}")
    logger.info("Device update command (stub)")


def getip_command(args):
    """Get device IP address"""
    logger.info(f"Getting IP for device {args.mac}")
    logger.info("Device getip command (stub)")


def ssh_command(args):
    """SSH to device"""
    logger.info(f"SSH to device {args.mac}")
    logger.info("Device ssh command (stub)")


def setup_device_commands(cli):
    """Setup the device commands"""
    device = cli.add_command_group(
        "device",
        help_text="Manage FBOSS devices",
        arguments=[("mac", {"help": "Device MAC address"})],
    )

    device.add_command(
        "image-upstream",
        image_upstream_command,
        help_text="Download and set upstream Distro Image to be loaded onto device",
        arguments=[],
    )

    device.add_command(
        "image",
        image_command,
        help_text="Set Distro Image file to be loaded onto device",
        arguments=[
            (
                "image_path",
                {
                    "type": lambda p: validate_path(p, must_exist=True),
                    "help": "Path to image file",
                },
            )
        ],
    )

    device.add_command(
        "reprovision", reprovision_command, help_text="Reprovision device", arguments=[]
    )

    device.add_command(
        "update",
        update_command,
        help_text="Update specific components on device",
        arguments=[
            (
                "manifest",
                {
                    "type": lambda p: validate_path(p, must_exist=True),
                    "help": "Path to manifest JSON file",
                },
            ),
            ("components", {"nargs": "+", "help": "Component names to update"}),
        ],
    )

    device.add_command(
        "getip", getip_command, help_text="Get device IP address", arguments=[]
    )

    device.add_command("ssh", ssh_command, help_text="SSH to device", arguments=[])
