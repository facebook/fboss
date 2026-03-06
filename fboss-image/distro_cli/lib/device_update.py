# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Device update logic for updating FBOSS services on devices."""

import logging
from pathlib import Path

from distro_cli.lib.exceptions import DistroInfraError
from distro_cli.lib.manifest import ImageManifest

logger = logging.getLogger(__name__)

# Component to systemd services mapping.
COMPONENT_SERVICES: dict[str, list[str]] = {
    "fboss-forwarding-stack": ["wedge_agent", "fsdb", "qsfp_service"],
    "fboss-platform-stack": [
        "platform_manager",
        "sensor_service",
        "fan_service",
        "data_corral_service",
    ],
}


class DeviceUpdateError(DistroInfraError):
    """Error during device update."""


class DeviceUpdater:
    """Handles updating FBOSS services on a device.

    Workflow:
    1. Validate component is supported for update
    2. Acquire artifacts (build OR download)
    3. Create update package (artifacts + service_update.sh) to scp to the device
    4. Get device IP
    5. SCP update package to device
    6. SSH: extract and run service_update.sh
    """

    def __init__(
        self,
        mac: str,
        manifest: ImageManifest,
        component: str,
        device_ip: str | None = None,
    ):
        """Initialize the DeviceUpdater.

        Args:
            mac: Device MAC address
            manifest: Parsed image manifest
            component: Component name to update
            device_ip: Optional device IP (if already known)
        """
        self.mac = mac
        self.manifest = manifest
        self.component = component
        self.device_ip = device_ip

    def _get_services(self) -> list[str]:
        """Get systemd services for the component."""
        return COMPONENT_SERVICES.get(self.component, [])

    def validate(self) -> None:
        """Validate the update request.

        Raises:
            DeviceUpdateError: If validation fails
        """
        if self.component not in COMPONENT_SERVICES:
            raise DeviceUpdateError(
                f"Component '{self.component}' is not updatable. "
                f"Updatable components: {', '.join(COMPONENT_SERVICES.keys())}"
            )

        if not self.manifest.has_component(self.component):
            raise DeviceUpdateError(
                f"Component '{self.component}' not found in manifest"
            )

        services = self._get_services()
        if not services:
            raise DeviceUpdateError(
                f"Component '{self.component}' has no services defined in COMPONENT_SERVICES"
            )

        component_data = self.manifest.get_component(self.component)
        has_download = "download" in component_data
        has_execute = "execute" in component_data

        if not has_download and not has_execute:
            raise DeviceUpdateError(
                f"Component '{self.component}' has neither 'download' nor 'execute'"
            )

    def _acquire_artifacts(self) -> Path:
        """Acquire component artifacts via build or download.

        Returns:
            Path to the component artifact (tarball)

        Raises:
            DeviceUpdateError: If artifact acquisition fails
        """
        raise NotImplementedError("Stub")

    def _create_update_package(self, artifact_path: Path) -> Path:
        """Create update package with artifacts and update_service.sh script.

        Args:
            artifact_path: Path to the component artifact tarball

        Returns:
            Path to the created update package

        Raises:
            DeviceUpdateError: If package creation fails
        """
        raise NotImplementedError("Stub")

    def _transfer_and_execute(self, package_path: Path, services: list[str]) -> None:
        """Transfer update package to device and execute update_service.sh.

        Args:
            package_path: Path to the update package tarball
            services: List of services to update

        Raises:
            DeviceUpdateError: If transfer or execution fails
        """
        raise NotImplementedError("Stub")

    def update(self) -> bool:
        """Execute the update workflow.

        Returns:
            True if update succeeded

        Raises:
            DeviceUpdateError: If update fails
        """
        raise NotImplementedError("Stub")
