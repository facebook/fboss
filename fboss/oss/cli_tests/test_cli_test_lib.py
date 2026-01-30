#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
Unit tests for cli_test_lib.py parsing logic.

These tests use static CLI output samples to verify the Interface parsing
without requiring a live switch connection.
"""

import unittest
from unittest.mock import patch

from cli_test_lib import get_all_interfaces, get_interface_info


# Sample parsed JSON data from 'fboss2 --fmt json show interface'
SAMPLE_JSON_DATA = {
    "127.0.0.1": {
        "interfaces": [
            {
                "name": "eth1/1/1",
                "description": "this",
                "status": "down",
                "speed": "800G",
                "vlan": 2001,
                "mtu": 1500,
                "prefixes": [
                    {"ip": "10.0.0.0", "prefixLength": 24},
                    {"ip": "2400::", "prefixLength": 64},
                    {"ip": "fe80::b4db:91ff:fe95:ff07", "prefixLength": 64},
                ],
            },
            {
                "name": "eth1/2/1",
                "description": "Another test description",
                "status": "up",
                "speed": "200G",
                "vlan": 2003,
                "mtu": 9216,
                "prefixes": [
                    {"ip": "11.0.0.0", "prefixLength": 24},
                    {"ip": "2401::", "prefixLength": 64},
                ],
            },
            {
                "name": "eth1/3/1",
                "description": "",
                "status": "down",
                "speed": "400G",
                "vlan": 2005,
                "mtu": 9000,
                "prefixes": [],
            },
        ]
    }
}


class TestGetAllInterfaces(unittest.TestCase):
    """Tests for get_all_interfaces() with mocked CLI."""

    @patch("cli_test_lib.run_cli")
    def test_get_all_interfaces(self, mock_run_cli):
        """Test parsing all interfaces."""
        mock_run_cli.return_value = SAMPLE_JSON_DATA

        interfaces = get_all_interfaces()

        self.assertEqual(len(interfaces), 3)
        self.assertIn("eth1/1/1", interfaces)
        self.assertIn("eth1/2/1", interfaces)
        self.assertIn("eth1/3/1", interfaces)

    @patch("cli_test_lib.run_cli")
    def test_parse_interface_fields(self, mock_run_cli):
        """Test that interface fields are correctly parsed."""
        mock_run_cli.return_value = SAMPLE_JSON_DATA

        interfaces = get_all_interfaces()

        intf = interfaces["eth1/1/1"]
        self.assertEqual(intf.name, "eth1/1/1")
        self.assertEqual(intf.status, "down")
        self.assertEqual(intf.speed, "800G")
        self.assertEqual(intf.vlan, 2001)
        self.assertEqual(intf.mtu, 1500)
        self.assertEqual(
            intf.addresses,
            ["10.0.0.0/24", "2400::/64", "fe80::b4db:91ff:fe95:ff07/64"],
        )
        self.assertEqual(intf.description, "this")

    @patch("cli_test_lib.run_cli")
    def test_parse_interface_no_addresses(self, mock_run_cli):
        """Test parsing an interface with no addresses."""
        mock_run_cli.return_value = SAMPLE_JSON_DATA

        interfaces = get_all_interfaces()

        intf = interfaces["eth1/3/1"]
        self.assertEqual(intf.name, "eth1/3/1")
        self.assertEqual(intf.addresses, [])

    @patch("cli_test_lib.run_cli")
    def test_empty_data(self, mock_run_cli):
        """Test that empty data returns empty dict."""
        mock_run_cli.return_value = {}

        interfaces = get_all_interfaces()
        self.assertEqual(interfaces, {})

    @patch("cli_test_lib.run_cli")
    def test_empty_interfaces(self, mock_run_cli):
        """Test that empty interfaces list returns empty dict."""
        mock_run_cli.return_value = {"127.0.0.1": {"interfaces": []}}

        interfaces = get_all_interfaces()
        self.assertEqual(interfaces, {})


class TestGetInterfaceInfo(unittest.TestCase):
    """Tests for get_interface_info() with mocked CLI."""

    @patch("cli_test_lib.run_cli")
    def test_get_interface_info(self, mock_run_cli):
        """Test getting a single interface."""
        mock_run_cli.return_value = SAMPLE_JSON_DATA

        intf = get_interface_info("eth1/1/1")

        self.assertEqual(intf.name, "eth1/1/1")
        self.assertEqual(intf.mtu, 1500)

    @patch("cli_test_lib.run_cli")
    def test_get_interface_info_not_found(self, mock_run_cli):
        """Test error when interface not found."""
        mock_run_cli.return_value = SAMPLE_JSON_DATA

        with self.assertRaises(RuntimeError):
            get_interface_info("nonexistent")


if __name__ == "__main__":
    unittest.main()
