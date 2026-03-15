#!/usr/bin/env python3

# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Unit tests for device commands."""

import argparse
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
from distro_cli.lib.distro_infra import DISTRO_INFRA_CONTAINER
from distro_cli.lib.docker import container


class TestDeviceCommands(unittest.TestCase):
    """Test device command group and subcommands"""

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
            command=["/distro_infra/run_distro_infra.sh", "--intf", "lo"],
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
        args = argparse.Namespace(mac=self.test_mac, components=["kernel", "sai"])
        # Call command - just verify it doesn't crash
        image_upstream_command(args)

    def test_image_stub(self):
        """Test image command (stub)"""
        args = argparse.Namespace(mac=self.test_mac, image_path=str(self.image_path))
        # Call command - just verify it doesn't crash
        image_command(args)

    def test_reprovision_stub(self):
        """Test reprovision command (stub)"""
        args = argparse.Namespace(mac=self.test_mac)
        # Call command - just verify it doesn't crash
        reprovision_command(args)

    def test_update_stub(self):
        """Test update command (stub)"""
        args = argparse.Namespace(
            mac=self.test_mac,
            manifest=str(self.manifest_path),
            components=["kernel", "sai"],
        )
        # Call command - just verify it doesn't crash
        update_command(args)

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


if __name__ == "__main__":
    unittest.main()
