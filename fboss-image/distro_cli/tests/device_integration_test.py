#!/usr/bin/env python3

"""Integration tests for device commands using the test topology.

Tests verify:
- FBOSS services run in per-service btrfs subvolumes
- Updates replace service scripts with new versions
- Service restart picks up the new version
"""

import subprocess
import unittest
from pathlib import Path

from distro_cli.lib.device_update import DeviceUpdater
from distro_cli.lib.manifest import ImageManifest
from distro_cli.tests.test_helpers import (
    get_writable_copy,
    override_artifact_store_dir,
    waitfor,
)

INTEGRATION_DATA_DIR = Path(__file__).parent / "test_topology" / "integration_data"
INTEGRATION_MANIFEST = INTEGRATION_DATA_DIR / "integration_manifest.json"

BASE_VERSION = "1.0.0"
UPDATE_VERSION = "2.0.0"

EXPECTED_SERVICES = [
    "wedge_agent",
    "fsdb",
    "qsfp_service",
    "platform_manager",
    "sensor_service",
    "fan_service",
    "data_corral_service",
]


class TestDeviceTopologyIntegration(unittest.TestCase):
    """Integration tests using the test topology"""

    PROXY_DEVICE = "proxy-device"

    @classmethod
    def setUpClass(cls):
        """Start the test topology (or reuse if already running)"""
        test_dir = Path(__file__).parent
        cls.topology_dir = test_dir / "test_topology"
        cls.start_script = cls.topology_dir / "start.sh"
        cls._started_topology = False

        if not cls.start_script.exists():
            raise unittest.SkipTest(f"Test topology not found at {cls.topology_dir}")

        result = subprocess.run(
            ["docker", "ps", "--format", "{{.Names}}"],
            capture_output=True,
            text=True,
            check=False,
        )
        running = result.stdout.strip().split("\n")
        topology_running = cls.PROXY_DEVICE in running

        if not topology_running:
            result = subprocess.run(
                [str(cls.start_script)],
                capture_output=True,
                text=True,
                timeout=120,
                check=False,
            )
            if result.returncode != 0:
                raise unittest.SkipTest(f"Failed to start topology: {result.stderr}")
            cls._started_topology = True

        result = subprocess.run(
            [
                "docker",
                "exec",
                cls.PROXY_DEVICE,
                "cat",
                "/sys/class/net/eth0/address",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            raise unittest.SkipTest("Failed to get device MAC")
        cls.device_mac = result.stdout.strip()

        # Get proxy-device IP directly from Docker instead of using distro_infra
        result = subprocess.run(
            [
                "docker",
                "inspect",
                "-f",
                "{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}",
                cls.PROXY_DEVICE,
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            raise unittest.SkipTest("Failed to get device IP")
        cls.device_ip = result.stdout.strip()

    @classmethod
    def tearDownClass(cls):
        """Stop the test topology only if we started it"""
        if cls._started_topology:
            subprocess.run(
                [str(cls.start_script), "stop"],
                capture_output=True,
                timeout=30,
                check=False,
            )

    def test_integration_topology_running(self):
        """Verify proxy-device container is running"""
        result = subprocess.run(
            ["docker", "ps", "--format", "{{.Names}}"],
            capture_output=True,
            text=True,
            check=False,
        )
        running = result.stdout.strip().split("\n")
        self.assertIn(self.PROXY_DEVICE, running)

    def test_integration_device_has_ip(self):
        """Verify device IP was resolved in setUpClass"""
        self.assertIsNotNone(self.device_ip)
        self.assertTrue(self.device_ip.startswith("172.17."))  # docker0 bridge subnet

    def test_integration_services_running(self):
        """Verify all FBOSS services are running"""
        for service in EXPECTED_SERVICES:
            result = subprocess.run(
                [
                    "docker",
                    "exec",
                    self.PROXY_DEVICE,
                    "systemctl",
                    "is-active",
                    service,
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(
                result.stdout.strip(), "active", f"Service {service} not active"
            )

    def test_integration_per_service_btrfs_subvolumes(self):
        """Verify each service runs in its own btrfs subvolume"""
        result = subprocess.run(
            ["docker", "exec", self.PROXY_DEVICE, "ls", "-d", "/mnt/btrfs/updates/"],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, "Updates directory should exist")

        # Initially, no per-service subvolumes should exist (created during updates)
        result = subprocess.run(
            ["docker", "exec", self.PROXY_DEVICE, "ls", "/mnt/btrfs/updates/"],
            capture_output=True,
            text=True,
            check=False,
        )
        # Directory should be empty or only contain distro-base
        if result.stdout.strip():
            subvolumes = result.stdout.strip().split("\n")
            # Only distro-base should exist, no per-service subvolumes yet
            for subvol in subvolumes:
                self.assertNotIn(
                    "-",
                    subvol,
                    f"No per-service subvolumes should exist yet, found {subvol}",
                )

    def test_integration_services_have_base_version(self):
        """Verify each service starts with the base version (1.0.0)"""
        for service in EXPECTED_SERVICES:
            version = self._get_service_version(service)
            self.assertEqual(
                version,
                BASE_VERSION,
                f"Service {service} should have version {BASE_VERSION}, got {version}",
            )

    def _update_component_and_verify(self, component: str, services: list[str]):
        """Update a component and wait for services to report new version."""
        # Get writable copy of integration data (test sandbox is read-only) to allow storing
        # intermediate build artifacts
        with get_writable_copy(
            INTEGRATION_DATA_DIR, "integration_"
        ) as temp_integration_data:
            temp_manifest = temp_integration_data / "integration_manifest.json"
            manifest = ImageManifest(temp_manifest)

            # Override artifact store to use writable temp directory (as the sandbox is read-only) to
            # store final artifacts
            artifact_dir = temp_integration_data.parent / "artifacts"
            artifact_dir.mkdir(exist_ok=True)
            with override_artifact_store_dir(artifact_dir):
                updater = DeviceUpdater(
                    mac=self.device_mac,
                    manifest=manifest,
                    component=component,
                    device_ip=self.device_ip,
                )
                updater.update()

        for svc in services:
            waitfor(
                lambda s=svc: self._get_service_version(s) != "",
                lambda s=svc: self.fail(f"Timed out waiting for {s} version file"),
            )

    def _get_service_version(self, service: str) -> str:
        """Read service version from the service's btrfs subvolume.

        Services run with RootDirectory set to their btrfs subvolume, so the version
        file is written to /var/run/{service}.version inside that subvolume, not the
        host's /var/run.
        """
        # Get the service's RootDirectory from systemd
        root_dir_result = subprocess.run(
            [
                "docker",
                "exec",
                self.PROXY_DEVICE,
                "systemctl",
                "show",
                service,
                "--property=RootDirectory",
            ],
            capture_output=True,
            text=True,
            check=False,
        )

        # Parse RootDirectory=<path> output
        root_dir = ""
        for line in root_dir_result.stdout.strip().split("\n"):
            if line.startswith("RootDirectory="):
                root_dir = line.split("=", 1)[1]
                break

        # If no RootDirectory, read from host's /var/run (base version case)
        version_path = (
            f"/var/run/{service}.version"
            if not root_dir
            else f"{root_dir}/var/run/{service}.version"
        )

        result = subprocess.run(
            [
                "docker",
                "exec",
                self.PROXY_DEVICE,
                "cat",
                version_path,
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        return result.stdout.strip()

    def _get_subvolume_count(self, service: str) -> int:
        result = subprocess.run(
            [
                "docker",
                "exec",
                self.PROXY_DEVICE,
                "bash",
                "-c",
                f"ls -d /mnt/btrfs/updates/{service}-* 2>/dev/null | wc -l",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        return int(result.stdout.strip()) if result.stdout.strip() else 0

    def _get_service_root_directory(self, service: str) -> str:
        result = subprocess.run(
            [
                "docker",
                "exec",
                self.PROXY_DEVICE,
                "bash",
                "-c",
                f"grep RootDirectory /etc/systemd/system/{service}.service.d/root-override.conf "
                f"2>/dev/null | cut -d= -f2",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        return result.stdout.strip()

    def _is_service_active(self, service: str) -> bool:
        result = subprocess.run(
            [
                "docker",
                "exec",
                self.PROXY_DEVICE,
                "systemctl",
                "is-active",
                service,
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        return result.stdout.strip() == "active"

    def test_integration_update_changes_version(self):
        """Test update creates new btrfs subvolume and cleans up old ones.

        Verifies:
        1. Initial state: services have base version (1.0.0), no subvolumes
        2. After first update: services have new version (2.0.0), 1 subvolume each
        3. After second update: services have newer version (2.0.0), still 1 subvolume (old cleaned up)
        """
        if not INTEGRATION_MANIFEST.exists():
            self.skipTest(f"Integration manifest not found: {INTEGRATION_MANIFEST}")

        test_cases = [
            (
                "fboss-platform-stack",
                [
                    "platform_manager",
                    "sensor_service",
                    "fan_service",
                    "data_corral_service",
                ],
            ),
            (
                "fboss-forwarding-stack",
                ["wedge_agent", "fsdb", "qsfp_service"],
            ),
        ]

        for component, services in test_cases:
            with self.subTest(component=component):
                # Initial state: services start with base version, no subvolumes
                for svc in services:
                    version = self._get_service_version(svc)
                    self.assertEqual(
                        version,
                        BASE_VERSION,
                        f"Service {svc} should start with {BASE_VERSION}, got {version}",
                    )
                    pre_count = self._get_subvolume_count(svc)
                    self.assertEqual(
                        pre_count,
                        0,
                        f"Service {svc} should have 0 subvolumes before first update",
                    )

                # First update
                self._update_component_and_verify(component, services)
                first_update_root_dirs = {}

                # After first update: verify subvolumes created and services use them
                for svc in services:
                    count = self._get_subvolume_count(svc)
                    self.assertEqual(
                        count,
                        1,
                        f"Service {svc} should have 1 subvolume after first update",
                    )

                    root_dir = self._get_service_root_directory(svc)
                    first_update_root_dirs[svc] = root_dir
                    self.assertTrue(
                        root_dir.startswith(f"/mnt/btrfs/updates/{svc}-"),
                        f"Service {svc} should have valid RootDirectory after first update (got {root_dir})",
                    )

                    self.assertTrue(
                        self._is_service_active(svc),
                        f"Service {svc} should be active after first update",
                    )

                    version = self._get_service_version(svc)
                    self.assertEqual(
                        version,
                        UPDATE_VERSION,
                        f"Service {svc} should have {UPDATE_VERSION} after first update, got {version}",
                    )

                # Second update
                self._update_component_and_verify(component, services)

                # After second update: verify old subvolume cleaned up, only 1 subvolume remains
                for svc in services:
                    count = self._get_subvolume_count(svc)
                    self.assertEqual(
                        count,
                        1,
                        f"Service {svc} should have 1 subvolume after second update (old cleaned up)",
                    )

                    root_dir = self._get_service_root_directory(svc)
                    self.assertTrue(
                        root_dir.startswith(f"/mnt/btrfs/updates/{svc}-"),
                        f"Service {svc} should have valid RootDirectory after second update (got {root_dir})",
                    )

                    # Verify RootDirectory changed (new subvolume created)
                    self.assertNotEqual(
                        root_dir,
                        first_update_root_dirs[svc],
                        f"Service {svc} RootDirectory should change after second update",
                    )

                    self.assertTrue(
                        self._is_service_active(svc),
                        f"Service {svc} should be active after second update",
                    )

                    version = self._get_service_version(svc)
                    self.assertEqual(
                        version,
                        UPDATE_VERSION,
                        f"Service {svc} should have {UPDATE_VERSION} after second update, got {version}",
                    )

if __name__ == "__main__":
    unittest.main()
