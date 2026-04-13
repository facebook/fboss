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
import sys
from pathlib import Path

from distro_cli.lib.cli import validate_path
from distro_cli.lib.device_update import DeviceUpdateError, DeviceUpdater
from distro_cli.lib.distro_infra import (
    deploy_image_to_device,
    DISTRO_INFRA_CONTAINER,
    get_interface_name,
    GETIP_SCRIPT_CONTAINER_PATH,
)
from distro_cli.lib.docker import container
from distro_cli.lib.exceptions import DistroInfraError
from distro_cli.lib.manifest import ImageManifest

logger = logging.getLogger(__name__)


def print_to_console(message: str) -> None:
    """Print message to console"""
    print(message)  # noqa: T201


def image_upstream_command(args):
    """Download full image from upstream repository and set it to be loaded onto device"""
    logger.info(f"Setting upstream image for device {args.mac}")
    logger.info("Device image-upstream command (stub)")


def image_command(args):
    """Set device image from file and configure PXE boot"""
    logger.info(f"Setting image for device {args.mac}: {args.image_path}")

    try:
        deploy_image_to_device(args.mac, args.image_path)
        logger.info(
            f"Successfully configured device {args.mac} with image {args.image_path}"
        )
        logger.info("Device is ready for PXE boot")

    except DistroInfraError as e:
        logger.error(f"Failed to configure device: {e}")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        sys.exit(1)


def reprovision_command(args):
    """Reprovision device"""
    ip_address = get_device_ip(args.mac)

    if not ip_address:
        logger.error("No IP address found for device")
        return

    # devpart -> /dev/nvme0n1p3
    # dev     -> /dev/nvme0n3
    # part    -> 3
    cmd = r"""
    if [ ! -d /opt/fboss ]; then echo "Not an FBOSS device. Aborting"; exit 1; fi; \
    rm -rf /boot/efi/EFI/*;
    root_devpart=$(mount | awk '/\/ type/ { print $1 }');
    root_dev=$(mount | awk -F 'p' '/\/ type/ { print $1 }');
    root_part=$(mount | awk -F '[[:space:]p]' '/\/ type/ { print $2 }');
    dd if=/dev/zero of=${root_devpart} bs=1M count=50;
    (sleep 1; echo yes; sleep 1; echo ignore) | parted ---pretend-input-tty ${root_dev} rm ${root_part};
    reboot --force
    """
    os.execvp(
        "ssh",
        [
            "ssh",
            "-o",
            "StrictHostKeyChecking=no",
            "-o",
            "UserKnownHostsFile=/dev/null",
            f"root@{ip_address}",
            cmd,
        ],
    )


def update_command(args):
    """Update specific components on device"""
    logger.info(f"Updating device {args.mac}")
    logger.info(f"Manifest: {args.manifest}")
    logger.info(f"Components: {' '.join(args.components)}")

    manifest = ImageManifest(Path(args.manifest))

    # Get device IP once for all components
    device_ip = get_device_ip(args.mac)
    if not device_ip:
        logger.error("Cannot update: device IP not found")
        sys.exit(1)

    for component in args.components:
        try:
            updater = DeviceUpdater(
                mac=args.mac,
                manifest=manifest,
                component=component,
                device_ip=device_ip,
            )
            updater.update()
            logger.info(f"Successfully updated {component}")
        except DeviceUpdateError as e:
            logger.error(f"Failed to update {component}: {e}")
            sys.exit(1)


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
