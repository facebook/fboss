# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Device command implementation."""

import json
import logging
import os

from distro_cli.lib.cli import validate_path
from distro_cli.lib.distro_infra import (
    DISTRO_INFRA_CONTAINER,
    GETIP_SCRIPT_CONTAINER_PATH,
    get_interface_name,
)
from distro_cli.lib.docker import container
from distro_cli.lib.exceptions import DistroInfraError

logger = logging.getLogger("fboss-image")


def print_to_console(message: str) -> None:
    """Print message to console"""
    print(message)  # noqa: T201


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


def get_device_ip(mac: str) -> str | None:
    """Get device IP address by querying the distro-infra container.

    Args:
        mac: Device MAC address

    Returns:
        IP address string (IPv4 preferred, IPv6 fallback), or None if not found
    """
    if not container.container_is_running(DISTRO_INFRA_CONTAINER):
        logger.error(f"Container '{DISTRO_INFRA_CONTAINER}' is not running")
        logger.error("Please start the distro-infra container first")
        return None

    try:
        interface = get_interface_name()
    except DistroInfraError as e:
        logger.error(f"Failed to get interface name: {e}")
        return None

    cmd = [GETIP_SCRIPT_CONTAINER_PATH, mac, interface]

    # Execute in container
    exit_code, stdout, stderr = container.exec_in_container(DISTRO_INFRA_CONTAINER, cmd)

    if exit_code != 0:
        logger.error(f"getip.sh failed with exit code {exit_code}")
        if stderr:
            logger.error(f"stderr: {stderr}")
        if stdout:
            logger.error(f"stdout: {stdout}")
        return None

    try:
        result = json.loads(stdout)

        if "error_code" in result:
            logger.error(f"Error: {result.get('error', 'Unknown error')}")
            logger.error(f"Error code: {result['error_code']}")
            return None

        ipv4 = result.get("ipv4")
        ipv6 = result.get("ipv6")

        return ipv4 if ipv4 else ipv6

    except json.JSONDecodeError as e:
        logger.error(f"Failed to parse JSON output: {e}")
        logger.error(f"Output was: {stdout}")
        return None


def getip_command(args):
    """Get device IP address"""
    logger.info(f"Getting IP for device {args.mac}")

    ip_address = get_device_ip(args.mac)

    if ip_address:
        print_to_console(ip_address)
    else:
        logger.error("No IP address found in response")


def ssh_command(args):
    """SSH to device"""
    logger.info(f"SSH to device {args.mac}")

    ip_address = get_device_ip(args.mac)

    if not ip_address:
        logger.error("No IP address found for device")
        return

    logger.info(f"Connecting to {ip_address}")
    os.execvp(
        "ssh",
        [
            "ssh",
            "-o",
            "StrictHostKeyChecking=no",
            "-o",
            "UserKnownHostsFile=/dev/null",
            f"root@{ip_address}",
        ],
    )


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
        "getip",
        getip_command,
        help_text="Get device IP address",
    )

    device.add_command(
        "ssh",
        ssh_command,
        help_text="SSH to device",
    )
