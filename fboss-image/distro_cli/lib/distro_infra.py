# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Distro Infrastructure helper functions."""

import json
import logging
import re
import subprocess
from pathlib import Path

from distro_cli.lib.docker import container
from distro_cli.lib.exceptions import DistroInfraError

logger = logging.getLogger("fboss-image")

# This should match DISTRO_CONTAINER_NAME in distro_infra/distro_infra.sh
DISTRO_INFRA_CONTAINER = "fboss-distro-infra"

GETIP_SCRIPT_CONTAINER_PATH = "/distro_infra/getip.sh"


def normalize_mac_address(mac: str) -> tuple[str, str]:
    """Normalize MAC address to both dash and colon formats.

    Args:
        mac: MAC address in any format

    Returns:
        Tuple of (dash_format, colon_format)
        e.g., ("aa-bb-cc-dd-ee-ff", "aa:bb:cc:dd:ee:ff")

    Raises:
        DistroInfraError: If MAC address is invalid
    """
    # Remove all separators and convert to lowercase
    mac_clean = re.sub(r"[:\-]", "", mac.lower())

    # Validate MAC address format (12 hex characters)
    if not re.match(r"^[0-9a-f]{12}$", mac_clean):
        raise DistroInfraError(
            f"Invalid MAC address: {mac}. Expected 12 hex characters with optional colons or dashes."
        )

    # Convert to dash and colon formats
    dash_mac = "-".join([mac_clean[i : i + 2] for i in range(0, 12, 2)])
    colon_mac = ":".join([mac_clean[i : i + 2] for i in range(0, 12, 2)])

    return dash_mac, colon_mac


def get_interface_name() -> str:
    """Get the network interface name of the distro-infra container from the persistent directory.

    Returns:
        Network interface name

    Raises:
        DistroInfraError: If interface_name.txt not found or empty
    """
    persistent_dir = find_persistent_dir()
    interface_file = persistent_dir / "interface_name.txt"

    if not interface_file.exists():
        raise DistroInfraError(
            f"Interface name file not found: {interface_file}. "
            "The distro-infra container may not have started properly."
        )

    interface = interface_file.read_text().strip()
    if not interface:
        raise DistroInfraError(f"Interface name file is empty: {interface_file}")

    return interface


def find_persistent_dir() -> Path:
    """Find the persistent directory mounted in the distro_infra container.

    Returns:
        Path to the persistent directory on the host

    Raises:
        DistroInfraError: If container is not running or persistent dir not found
    """
    # Check if container is running
    if not container.container_is_running(DISTRO_INFRA_CONTAINER):
        raise DistroInfraError(
            f"Container '{DISTRO_INFRA_CONTAINER}' is not running. "
            "Please start it first with distro_infra.sh"
        )

    try:
        result = subprocess.run(
            ["docker", "inspect", DISTRO_INFRA_CONTAINER],
            capture_output=True,
            text=True,
            check=True,
        )
        inspect_data = json.loads(result.stdout)

        if not inspect_data:
            raise DistroInfraError(
                f"Container {DISTRO_INFRA_CONTAINER} is not running. "
                "Please start it first with distro_infra.sh"
            )

        # Find the volume mount for /distro_infra/persistent
        mounts = inspect_data[0].get("Mounts", [])
        for mount in mounts:
            if mount.get("Destination") == "/distro_infra/persistent":
                return Path(mount["Source"])

        raise DistroInfraError(
            f"Could not find persistent directory mount in container {DISTRO_INFRA_CONTAINER}"
        )

    except subprocess.CalledProcessError as e:
        raise DistroInfraError(
            f"Container {DISTRO_INFRA_CONTAINER} is not running. "
            "Please start it first with distro_infra.sh"
        ) from e
    except (json.JSONDecodeError, KeyError, IndexError) as e:
        raise DistroInfraError(f"Failed to parse container inspect data: {e}") from e
