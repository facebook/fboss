#!/usr/bin/env python3

# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Unit tests for device commands."""

import argparse
import json
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from distro_cli.cmds.device import (
    get_device_ip,
    getip_command,
    image_command,
    image_upstream_command,
    reprovision_command,
    setup_device_commands,
    ssh_command,
    update_command,
)
from distro_cli.lib.cli import CLI
from distro_cli.lib.device_update import DeviceUpdateError, DeviceUpdater
from distro_cli.lib.distro_infra import DISTRO_INFRA_CONTAINER
from distro_cli.lib.docker import container
from distro_cli.lib.manifest import ImageManifest
from distro_cli.tests.test_helpers import waitfor


class TestDeviceCommands(unittest.TestCase):
    """Test device command group and subcommands"""

    IPXE_FILES = ("ipxev4.efi", "ipxev6.efi", "autoexec.ipxe")

    @classmethod
    def setUpClass(cls):
        """Set up test container before all tests"""
        try:
            result = subprocess.run(
                ["docker", "images", "-q", "fboss_distro_infra"],
                capture_output=True,
                text=True,
                check=True,
            )
            if not result.stdout.strip():
                raise unittest.SkipTest(
                    "fboss_distro_infra Docker image not found. "
                    "Please build it with: cd fboss-image/distro_infra && ./build.sh"
                )
        except (subprocess.CalledProcessError, FileNotFoundError):
            raise unittest.SkipTest("Docker not available or image not built")

        cwd = Path.cwd()
        cls.container_temp_dir = Path(
            tempfile.mkdtemp(prefix="distro_infra_test_", dir=cwd)
        )
        cls.container_persistent_dir = cls.container_temp_dir / "persistent"
        cls.container_persistent_dir.mkdir(parents=True, exist_ok=True)

        # Write interface name file (normally done by distro_infra.sh)
        interface_file = cls.container_persistent_dir / "interface_name.txt"
        interface_file.write_text("lo")

        # Clean up any existing container with the same name
        if container.container_is_running(DISTRO_INFRA_CONTAINER):
            container.stop_and_remove_container(DISTRO_INFRA_CONTAINER)

        # Start the fboss-distro-infra container in background
        volumes = {cls.container_persistent_dir: Path("/distro_infra/persistent")}

        exit_code = container.run_container(
            image="fboss_distro_infra",
            command=["/distro_infra/run_distro_infra.sh", "--intf", "lo", "--nodhcpv6"],
            volumes=volumes,
            ephemeral=False,
            detach=True,
            name=DISTRO_INFRA_CONTAINER,
            privileged=True,  # Required for network operations
        )

        if exit_code != 0:
            raise RuntimeError(f"Failed to start {DISTRO_INFRA_CONTAINER} container")

    @classmethod
    def tearDownClass(cls):
        """Clean up test container after all tests"""
        if container.container_is_running(DISTRO_INFRA_CONTAINER):
            container.stop_and_remove_container(DISTRO_INFRA_CONTAINER)

        shutil.rmtree(cls.container_temp_dir, ignore_errors=True)

    def setup_image_command_test(self):
        """Wait for container to create PXE boot infrastructure."""
        cache_dir = self.container_persistent_dir / "cache"

        waitfor(
            cache_dir.exists,
            lambda: self.fail("Timed out waiting for cache directory to be created"),
        )

        for filename in self.IPXE_FILES:
            cache_file = cache_dir / filename
            waitfor(
                cache_file.exists,
                lambda f=filename: self.fail(
                    f"Timed out waiting for {f} to be created"
                ),
            )

    def verify_image_command_common(self, mac):
        """Verify common PXE boot infrastructure created by image command"""
        dash_mac = mac.replace(":", "-")
        mac_dir = self.container_persistent_dir / dash_mac

        self.assertTrue(mac_dir.exists())
        self.assertTrue(mac_dir.is_dir())

        for ipxe_file in self.IPXE_FILES:
            ipxe_path = mac_dir / ipxe_file
            self.assertTrue(ipxe_path.exists())

        pxeboot_marker = mac_dir / "pxeboot_complete"
        self.assertTrue(pxeboot_marker.exists())

        ipxev6_serverip = mac_dir / "ipxev6.efi-serverip"
        if ipxev6_serverip.exists():
            content = ipxev6_serverip.read_text()
            self.assertIn("#!ipxe", content)
            self.assertIn("set server_ip", content)

        result = subprocess.run(
            [
                "docker",
                "exec",
                DISTRO_INFRA_CONTAINER,
                "cat",
                f"/distro_infra/dnsmasq_conf.d/{dash_mac}",
            ],
            capture_output=True,
            check=False,
            text=True,
        )
        self.assertEqual(result.returncode, 0)
        self.assertIn(mac, result.stdout)

        return mac_dir

    def setUp(self):
        """Set up test fixtures"""
        self.test_mac = "aa:bb:cc:dd:ee:ff"

        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            f.write('{"test": "manifest"}')
            self.manifest_path = Path(f.name)

        self.temp_dir = tempfile.mkdtemp()
        self.image_path = Path(self.temp_dir) / "test_image.tar"

        test_file = Path(self.temp_dir) / "test_file.txt"
        test_file.write_text("test content")

        with tarfile.open(self.image_path, "w") as tar:
            tar.add(test_file, arcname="test_file.txt")

    def tearDown(self):
        """Clean up test fixtures"""
        self.manifest_path.unlink()
        shutil.rmtree(self.temp_dir, ignore_errors=True)

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
        args = argparse.Namespace(
            mac=self.test_mac, components=["kernel", "npu_sai"]
        )
        # Call command - just verify it doesn't crash
        image_upstream_command(args)

    def test_image_command_with_tarball(self):
        """Test image command with tarball extraction"""
        self.setup_image_command_test()

        temp_dir = tempfile.mkdtemp()
        test_file = Path(temp_dir) / "test_file.txt"
        test_file.write_text("test content")
        tarball_path = Path(temp_dir) / "test_image.tar"
        with tarfile.open(tarball_path, "w") as tar:
            tar.add(test_file, arcname="test_file.txt")

        args = argparse.Namespace(mac=self.test_mac, image_path=str(tarball_path))
        image_command(args)

        mac_dir = self.verify_image_command_common(self.test_mac)

        extracted_file = mac_dir / "test_file.txt"
        self.assertTrue(extracted_file.exists())
        self.assertEqual(extracted_file.read_text(), "test content")

        shutil.rmtree(temp_dir, ignore_errors=True)

    def test_image_command_with_directory(self):
        """Test image command failure with directory (only tarballs supported)"""
        self.setup_image_command_test()

        temp_dir = tempfile.mkdtemp()
        dir_path = Path(temp_dir) / "test_image_dir"
        dir_path.mkdir()
        file1 = dir_path / "file1.txt"
        file2 = dir_path / "file2.txt"
        file1.write_text("content1")
        file2.write_text("content2")

        args = argparse.Namespace(mac=self.test_mac, image_path=str(dir_path))
        with self.assertRaises(SystemExit) as excinfo:
            image_command(args)
        self.assertEqual(excinfo.exception.code, 1)

        shutil.rmtree(temp_dir, ignore_errors=True)

    def test_image_command_with_single_file(self):
        """Test image command failure with non-tarball file"""
        self.setup_image_command_test()

        temp_dir = tempfile.mkdtemp()
        single_file_path = Path(temp_dir) / "single_file.bin"
        single_file_path.write_text("single file content")

        args = argparse.Namespace(mac=self.test_mac, image_path=str(single_file_path))
        with self.assertRaises(SystemExit) as excinfo:
            image_command(args)
        self.assertEqual(excinfo.exception.code, 1)

        shutil.rmtree(temp_dir, ignore_errors=True)

    def test_reprovision_stub(self):
        """Test reprovision command (stub)"""
        args = argparse.Namespace(mac=self.test_mac)
        # Call command - just verify it doesn't crash
        reprovision_command(args)

    @patch("distro_cli.cmds.device.container.exec_in_container")
    @patch("distro_cli.cmds.device.container.container_is_running")
    def test_get_device_ip_ipv4(self, mock_is_running, mock_exec):
        """Test get_device_ip returns IPv4 when available"""
        mock_is_running.return_value = True
        mock_exec.return_value = (
            0,
            '{"mac": "aa:bb:cc:dd:ee:ff", "ipv4": "192.168.1.100", "ipv6": "fe80::1"}',
            "",
        )

        ip = get_device_ip(self.test_mac)
        self.assertEqual(ip, "192.168.1.100")

    @patch("distro_cli.cmds.device.container.exec_in_container")
    @patch("distro_cli.cmds.device.container.container_is_running")
    def test_get_device_ip_ipv6_fallback(self, mock_is_running, mock_exec):
        """Test get_device_ip returns IPv6 when IPv4 not available"""
        mock_is_running.return_value = True
        mock_exec.return_value = (
            0,
            '{"mac": "aa:bb:cc:dd:ee:ff", "ipv6": "fe80::1"}',
            "",
        )

        ip = get_device_ip(self.test_mac)
        self.assertEqual(ip, "fe80::1")

    @patch("distro_cli.cmds.device.container.exec_in_container")
    @patch("distro_cli.cmds.device.container.container_is_running")
    @patch("distro_cli.cmds.device.os.execvp")
    def test_ssh_command_calls_execvp_correctly(
        self, mock_execvp, mock_is_running, mock_exec
    ):
        """Test ssh command calls os.execvp with correct arguments"""
        mock_is_running.return_value = True
        mock_exec.return_value = (
            0,
            '{"mac": "aa:bb:cc:dd:ee:ff", "ipv4": "192.168.1.100"}',
            "",
        )

        args = argparse.Namespace(mac=self.test_mac, interface=None)
        ssh_command(args)

        mock_execvp.assert_called_once_with(
            "ssh",
            [
                "ssh",
                "-o",
                "StrictHostKeyChecking=no",
                "-o",
                "UserKnownHostsFile=/dev/null",
                "root@192.168.1.100",
            ],
        )


class TestDeviceCLIIntegration(unittest.TestCase):
    """Test device command CLI integration for image command"""

    def setUp(self):
        """Set up CLI for testing"""
        self.cli = CLI(description="Test CLI")
        setup_device_commands(self.cli)

    def test_image_command_argument_parsing(self):
        """Test that image command arguments are parsed correctly"""
        with tempfile.NamedTemporaryFile(suffix=".tar", delete=False) as f:
            temp_image = f.name

        try:
            args = self.cli.parser.parse_args(
                ["device", "aa:bb:cc:dd:ee:ff", "image", temp_image]
            )
            self.assertEqual(args.mac, "aa:bb:cc:dd:ee:ff")
            self.assertEqual(str(args.image_path), temp_image)
            self.assertTrue(callable(args.func))
            self.assertEqual(args.func, image_command)
        finally:
            Path(temp_image).unlink()


class TestDeviceUpdater(unittest.TestCase):
    """Unit tests for DeviceUpdater class"""

    def setUp(self):
        """Set up test fixtures"""
        self.test_data_dir = Path(__file__).parent / "data"
        self.update_manifest_path = self.test_data_dir / "update_manifest.json"

    def test_validate_non_updatable_component(self):
        """Test that non-updatable components raise error"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="kernel",
        )
        with self.assertRaises(DeviceUpdateError) as ctx:
            updater.validate()
        self.assertIn("is not updatable", str(ctx.exception))
        self.assertIn("Updatable components:", str(ctx.exception))

    def test_validate_component_not_in_manifest(self):
        """Test that component in COMPONENT_SERVICES but missing from manifest raises error"""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(
                {
                    "distribution_formats": {"onie": "test.bin"},
                    "kernel": {"download": "https://example.com/kernel.tar"},
                },
                f,
            )
            temp_manifest_path = Path(f.name)
        self.addCleanup(temp_manifest_path.unlink)

        manifest = ImageManifest(temp_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-forwarding-stack",
        )
        with self.assertRaises(DeviceUpdateError) as ctx:
            updater.validate()
        self.assertIn("not found in manifest", str(ctx.exception))

    def test_validate_component_with_no_services_allowed_for_other_dependencies(self):
        """Test that component with empty services list is allowed for other_dependencies"""
        # Create a manifest with other_dependencies
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(
                {
                    "distribution_formats": {"onie": "test.bin"},
                    "kernel": {"download": "https://example.com/kernel.tar"},
                    "other_dependencies": [
                        {"download": "https://example.com/nano.rpm"}
                    ],
                },
                f,
            )
            temp_manifest_path = Path(f.name)
        self.addCleanup(temp_manifest_path.unlink)

        manifest = ImageManifest(temp_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="other_dependencies",
        )
        updater.validate()

    def test_validate_component_with_no_services_raises_error(self):
        """Test that component with empty services list raises error (except other_dependencies)"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-platform-stack",
        )
        # Patch COMPONENT_SERVICES to have empty list for platform-stack
        with patch(
            "distro_cli.lib.device_update.COMPONENT_SERVICES",
            {"fboss-platform-stack": [], "fboss-forwarding-stack": ["wedge_agent"]},
        ):
            with self.assertRaises(DeviceUpdateError) as ctx:
                updater.validate()
            self.assertIn("has no services defined", str(ctx.exception))

    def test_validate_component_without_download_or_execute(self):
        """Test that component without download or execute raises error"""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(
                {
                    "distribution_formats": {"onie": "test.bin"},
                    "kernel": {"download": "https://example.com/kernel.tar"},
                    "fboss-forwarding-stack": {},
                },
                f,
            )
            temp_manifest_path = Path(f.name)
        self.addCleanup(temp_manifest_path.unlink)

        manifest = ImageManifest(temp_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-forwarding-stack",
        )
        with self.assertRaises(DeviceUpdateError) as ctx:
            updater.validate()
        self.assertIn("neither 'download' nor 'execute'", str(ctx.exception))

    def test_validate_success_forwarding_stack(self):
        """Test successful validation for fboss-forwarding-stack"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-forwarding-stack",
        )
        # Should not raise
        updater.validate()

    def test_validate_success_platform_stack(self):
        """Test successful validation for fboss-platform-stack"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-platform-stack",
        )
        # Should not raise
        updater.validate()

    def test_get_services_from_component_services(self):
        """Test that services are correctly read from COMPONENT_SERVICES dict"""
        manifest = ImageManifest(self.update_manifest_path)

        updater1 = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-forwarding-stack",
        )
        self.assertEqual(
            updater1._get_services(),
            ["wedge_agent", "fsdb", "qsfp_service"],
        )

        updater2 = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-platform-stack",
        )
        self.assertEqual(
            updater2._get_services(),
            [
                "platform_manager",
                "sensor_service",
                "fan_service",
                "data_corral_service",
            ],
        )

    def test_get_services_for_other_dependencies(self):
        """Test that other_dependencies returns empty service list"""
        # Create a manifest with other_dependencies
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(
                {
                    "distribution_formats": {"onie": "test.bin"},
                    "kernel": {"download": "https://example.com/kernel.tar"},
                    "other_dependencies": [
                        {"download": "https://example.com/nano.rpm"}
                    ],
                },
                f,
            )
            temp_manifest_path = Path(f.name)
        self.addCleanup(temp_manifest_path.unlink)

        manifest = ImageManifest(temp_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="other_dependencies",
        )
        self.assertEqual(updater._get_services(), [])

    def test_update_requires_device_ip(self):
        """Test that update() raises DeviceUpdateError when device_ip is not set"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-forwarding-stack",
            device_ip=None,
        )
        with self.assertRaises(DeviceUpdateError) as ctx:
            updater.update()
        self.assertIn("Device IP not set", str(ctx.exception))

    def test_acquire_artifacts_and_services(self):
        """Test services are available for platform-stack component"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-platform-stack",
        )

        # Validate first
        updater.validate()

        # Verify services come from COMPONENT_SERVICES dict
        services = updater._get_services()
        self.assertEqual(
            services,
            [
                "platform_manager",
                "sensor_service",
                "fan_service",
                "data_corral_service",
            ],
        )

    def test_acquire_artifacts_forwarding_stack(self):
        """Test services are available for forwarding-stack component"""
        manifest = ImageManifest(self.update_manifest_path)
        updater = DeviceUpdater(
            mac="aa:bb:cc:dd:ee:ff",
            manifest=manifest,
            component="fboss-forwarding-stack",
        )

        # Validate first
        updater.validate()

        # Verify services come from COMPONENT_SERVICES dict
        services = updater._get_services()
        self.assertEqual(
            services,
            ["wedge_agent", "fsdb", "qsfp_service"],
        )


if __name__ == "__main__":
    unittest.main()
