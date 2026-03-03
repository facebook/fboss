#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
Tests for BGP++ JSON to fboss2 CLI converter (j2c tool).

Tests cover:
- escape_shell_arg function for shell escaping
- format_bandwidth function for bandwidth formatting
- generate_global_commands for global BGP settings
- generate_peer_group_commands for peer group configuration
- generate_peer_commands for peer configuration
- json_to_cli full integration
"""

import unittest

from fboss.cli.fboss2.tools.bgp_json_to_cli import (
    escape_shell_arg,
    format_bandwidth,
    generate_exec_commands,
    generate_global_commands,
    generate_peer_commands,
    generate_peer_group_commands,
    generate_stub_commands,
    json_to_cli,
)


class EscapeShellArgTest(unittest.TestCase):
    """Tests for escape_shell_arg function."""

    def test_simple_string_not_escaped(self) -> None:
        """Simple strings without special chars should not be quoted."""
        result = escape_shell_arg("simple")
        self.assertEqual(result, "simple")

    def test_string_with_space_is_quoted(self) -> None:
        """Strings with spaces should be wrapped in double quotes."""
        result = escape_shell_arg("hello world")
        self.assertEqual(result, '"hello world"')

    def test_string_with_special_chars_is_quoted(self) -> None:
        """Strings with shell special characters should be quoted."""
        result = escape_shell_arg("value$var")
        self.assertEqual(result, '"value$var"')

    def test_string_with_backslash_is_quoted(self) -> None:
        """Strings with backslash should be quoted."""
        result = escape_shell_arg("path\\to\\file")
        self.assertEqual(result, '"path\\to\\file"')

    def test_empty_string(self) -> None:
        """Empty string should not be quoted."""
        result = escape_shell_arg("")
        self.assertEqual(result, "")

    def test_ipv6_address_not_escaped(self) -> None:
        """IPv6 addresses should not be quoted."""
        result = escape_shell_arg("2401:db00:501c::/64")
        self.assertEqual(result, "2401:db00:501c::/64")


class FormatBandwidthTest(unittest.TestCase):
    """Tests for format_bandwidth function."""

    def test_terabits(self) -> None:
        """Bandwidth in terabits should format as 'T'."""
        result = format_bandwidth(1_000_000_000_000)
        self.assertEqual(result, "1T")

    def test_gigabits(self) -> None:
        """Bandwidth in gigabits should format as 'G'."""
        result = format_bandwidth(10_000_000_000)
        self.assertEqual(result, "10G")

    def test_megabits(self) -> None:
        """Bandwidth in megabits should format as 'M'."""
        result = format_bandwidth(100_000_000)
        self.assertEqual(result, "100M")

    def test_kilobits(self) -> None:
        """Bandwidth in kilobits should format as 'K'."""
        result = format_bandwidth(1_000)
        self.assertEqual(result, "1K")

    def test_raw_bits(self) -> None:
        """Non-standard values should return as raw string."""
        result = format_bandwidth(12345)
        self.assertEqual(result, "12345")

    def test_100g(self) -> None:
        """100G should format correctly."""
        result = format_bandwidth(100_000_000_000)
        self.assertEqual(result, "100G")


class GenerateGlobalCommandsTest(unittest.TestCase):
    """Tests for generate_global_commands function."""

    def test_router_id(self) -> None:
        """Router ID should generate correct command."""
        config = {"router_id": "10.0.0.1"}
        commands = generate_global_commands(config)
        self.assertEqual(commands, ["config protocol bgp global router-id 10.0.0.1"])

    def test_local_as(self) -> None:
        """Local AS should generate correct command."""
        config = {"local_as_4_byte": 65000}
        commands = generate_global_commands(config)
        self.assertEqual(commands, ["config protocol bgp global local-asn 65000"])

    def test_hold_time(self) -> None:
        """Hold time should generate correct command."""
        config = {"hold_time": 90}
        commands = generate_global_commands(config)
        self.assertEqual(commands, ["config protocol bgp global hold-time 90"])

    def test_confed_asn(self) -> None:
        """Confederation AS should generate correct command."""
        config = {"local_confed_as_4_byte": 65001}
        commands = generate_global_commands(config)
        self.assertEqual(commands, ["config protocol bgp global confed-asn 65001"])

    def test_cluster_id(self) -> None:
        """Cluster ID should generate correct command."""
        config = {"cluster_id": "192.168.1.1"}
        commands = generate_global_commands(config)
        self.assertEqual(
            commands, ["config protocol bgp global cluster-id 192.168.1.1"]
        )

    def test_networks6_with_all_fields(self) -> None:
        """networks6 should generate command with all fields."""
        config = {
            "networks6": [
                {
                    "prefix": "2001:db8::/32",
                    "policy_name": "TEST_POLICY",
                    "install_to_fib": False,
                    "minimum_supporting_routes": 2,
                }
            ]
        }
        commands = generate_global_commands(config)
        expected = [
            "config protocol bgp global network6 add 2001:db8::/32 policy TEST_POLICY install-to-fib false min-routes 2"
        ]
        self.assertEqual(commands, expected)

    def test_networks6_with_install_to_fib_true(self) -> None:
        """networks6 with install_to_fib=true should preserve value."""
        config = {
            "networks6": [
                {
                    "prefix": "2001:db8::/32",
                    "install_to_fib": True,
                }
            ]
        }
        commands = generate_global_commands(config)
        self.assertIn("install-to-fib true", commands[0])

    def test_networks6_empty_prefix_skipped(self) -> None:
        """networks6 with empty prefix should be skipped."""
        config = {"networks6": [{"prefix": "", "policy_name": "ORPHAN"}]}
        commands = generate_global_commands(config)
        self.assertEqual(commands, [])

    def test_switch_limit_prefix_limit(self) -> None:
        """switch_limit_config.prefix_limit should generate correct command."""
        config = {"switch_limit_config": {"prefix_limit": 10000}}
        commands = generate_global_commands(config)
        self.assertEqual(commands, ["config protocol bgp global switch-limit 10000"])

    def test_switch_limit_total_path_limit(self) -> None:
        """switch_limit_config.total_path_limit should generate correct command."""
        config = {"switch_limit_config": {"total_path_limit": 250000}}
        commands = generate_global_commands(config)
        self.assertEqual(
            commands, ["config protocol bgp global switch-limit-total-path 250000"]
        )

    def test_switch_limit_max_golden_vips(self) -> None:
        """switch_limit_config.max_golden_vips should generate correct command."""
        config = {"switch_limit_config": {"max_golden_vips": 2860}}
        commands = generate_global_commands(config)
        self.assertEqual(
            commands, ["config protocol bgp global switch-limit-max-golden-vips 2860"]
        )

    def test_switch_limit_overload_protection_mode(self) -> None:
        """switch_limit_config.overload_protection_mode should generate correct command."""
        config = {"switch_limit_config": {"overload_protection_mode": 2}}
        commands = generate_global_commands(config)
        self.assertEqual(
            commands,
            ["config protocol bgp global switch-limit-overload-protection-mode 2"],
        )

    def test_all_switch_limit_fields(self) -> None:
        """All switch_limit_config fields should generate correct commands."""
        config = {
            "switch_limit_config": {
                "prefix_limit": 10000,
                "total_path_limit": 250000,
                "max_golden_vips": 2860,
                "overload_protection_mode": 2,
            }
        }
        commands = generate_global_commands(config)
        self.assertEqual(len(commands), 4)
        self.assertIn("config protocol bgp global switch-limit 10000", commands)
        self.assertIn(
            "config protocol bgp global switch-limit-total-path 250000", commands
        )
        self.assertIn(
            "config protocol bgp global switch-limit-max-golden-vips 2860", commands
        )
        self.assertIn(
            "config protocol bgp global switch-limit-overload-protection-mode 2",
            commands,
        )

    def test_empty_config(self) -> None:
        """Empty config should return empty list."""
        config = {}
        commands = generate_global_commands(config)
        self.assertEqual(commands, [])


class GeneratePeerGroupCommandsTest(unittest.TestCase):
    """Tests for generate_peer_group_commands function."""

    def test_empty_name_returns_empty(self) -> None:
        """Peer group without name should return empty list."""
        peer_group = {"remote_as_4_byte": 65000}
        commands = generate_peer_group_commands(peer_group)
        self.assertEqual(commands, [])

    def test_remote_asn(self) -> None:
        """remote_as_4_byte should generate correct command."""
        peer_group = {"name": "TEST-GROUP", "remote_as_4_byte": 65000}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            "config protocol bgp peer-group TEST-GROUP remote-asn 65000", commands
        )

    def test_description_with_spaces(self) -> None:
        """Description with spaces should be properly quoted."""
        peer_group = {"name": "TEST", "description": "Test peer group"}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            'config protocol bgp peer-group TEST description "Test peer group"',
            commands,
        )

    def test_boolean_flag_true(self) -> None:
        """Boolean flag with true value should generate 'true'."""
        peer_group = {"name": "TEST", "next_hop_self": True}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            "config protocol bgp peer-group TEST next-hop-self true", commands
        )

    def test_boolean_flag_false(self) -> None:
        """Boolean flag with false value should generate 'false' (not omitted)."""
        peer_group = {"name": "TEST", "disable_ipv4_afi": False}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            "config protocol bgp peer-group TEST disable-ipv4-afi false", commands
        )

    def test_is_rr_client(self) -> None:
        """is_rr_client should generate rr-client command."""
        peer_group = {"name": "TEST", "is_rr_client": True}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn("config protocol bgp peer-group TEST rr-client true", commands)

    def test_is_confed_peer(self) -> None:
        """is_confed_peer should generate confed-peer command."""
        peer_group = {"name": "TEST", "is_confed_peer": True}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn("config protocol bgp peer-group TEST confed-peer true", commands)

    def test_policies(self) -> None:
        """Policy names should generate correct commands."""
        peer_group = {
            "name": "TEST",
            "ingress_policy_name": "IN_POLICY",
            "egress_policy_name": "OUT_POLICY",
        }
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            "config protocol bgp peer-group TEST ingress-policy IN_POLICY", commands
        )
        self.assertIn(
            "config protocol bgp peer-group TEST egress-policy OUT_POLICY", commands
        )

    def test_v4_over_v6_nexthop(self) -> None:
        """v4_over_v6_nexthop should generate v4-over-v6-nh command."""
        peer_group = {"name": "TEST", "v4_over_v6_nexthop": True}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            "config protocol bgp peer-group TEST v4-over-v6-nh true", commands
        )

    def test_timers(self) -> None:
        """Timer fields should generate correct commands."""
        peer_group = {
            "name": "TEST",
            "bgp_peer_timers": {
                "hold_time_seconds": 30,
                "keep_alive_seconds": 10,
                "out_delay_seconds": 5,
                "withdraw_unprog_delay_seconds": 0,
            },
        }
        commands = generate_peer_group_commands(peer_group)
        self.assertIn(
            "config protocol bgp peer-group TEST timers hold-time 30", commands
        )
        self.assertIn(
            "config protocol bgp peer-group TEST timers keepalive 10", commands
        )
        self.assertIn(
            "config protocol bgp peer-group TEST timers out-delay 5", commands
        )
        self.assertIn(
            "config protocol bgp peer-group TEST timers withdraw-unprog-delay 0",
            commands,
        )

    def test_pre_filter_max_routes(self) -> None:
        """pre_filter.max_routes should generate max-routes command."""
        peer_group = {"name": "TEST", "pre_filter": {"max_routes": 45000}}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn("config protocol bgp peer-group TEST max-routes 45000", commands)

    def test_pre_filter_warning_settings(self) -> None:
        """pre_filter warning settings should generate correct commands."""
        peer_group = {
            "name": "TEST",
            "pre_filter": {"warning_limit": 100, "warning_only": True},
        }
        commands = generate_peer_group_commands(peer_group)
        self.assertIn("config protocol bgp peer-group TEST warning-limit 100", commands)
        self.assertIn("config protocol bgp peer-group TEST warning-only true", commands)

    def test_peer_tag(self) -> None:
        """peer_tag should generate peer-tag command."""
        peer_group = {"name": "TEST", "peer_tag": "FSW"}
        commands = generate_peer_group_commands(peer_group)
        self.assertIn("config protocol bgp peer-group TEST peer-tag FSW", commands)


class GeneratePeerCommandsTest(unittest.TestCase):
    """Tests for generate_peer_commands function."""

    def test_empty_peer_addr_returns_empty(self) -> None:
        """Peer without peer_addr should return empty list."""
        peer = {"remote_as_4_byte": 65000}
        commands = generate_peer_commands(peer)
        self.assertEqual(commands, [])

    def test_remote_asn(self) -> None:
        """remote_as_4_byte should generate correct command."""
        peer = {"peer_addr": "2001:db8::1", "remote_as_4_byte": 65000}
        commands = generate_peer_commands(peer)
        self.assertIn("config protocol bgp peer 2001:db8::1 remote-asn 65000", commands)

    def test_peer_group_name(self) -> None:
        """peer_group_name should generate peer-group command."""
        peer = {"peer_addr": "2001:db8::1", "peer_group_name": "MY-GROUP"}
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 peer-group MY-GROUP", commands
        )

    def test_local_addr(self) -> None:
        """local_addr should generate local-addr command."""
        peer = {"peer_addr": "2001:db8::1", "local_addr": "2001:db8::a"}
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 local-addr 2001:db8::a", commands
        )

    def test_is_passive(self) -> None:
        """is_passive should generate passive command."""
        peer = {"peer_addr": "2001:db8::1", "is_passive": True}
        commands = generate_peer_commands(peer)
        self.assertIn("config protocol bgp peer 2001:db8::1 passive true", commands)

    def test_disable_ipv4_afi_false_preserved(self) -> None:
        """disable_ipv4_afi: false should NOT be omitted."""
        peer = {"peer_addr": "2001:db8::1", "disable_ipv4_afi": False}
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 disable-ipv4-afi false", commands
        )

    def test_disable_ipv4_afi_true(self) -> None:
        """disable_ipv4_afi: true should generate correct command."""
        peer = {"peer_addr": "2001:db8::1", "disable_ipv4_afi": True}
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 disable-ipv4-afi true", commands
        )

    def test_next_hop4_and_next_hop6(self) -> None:
        """next_hop4 and next_hop6 should generate correct commands."""
        peer = {
            "peer_addr": "2001:db8::1",
            "next_hop4": "0.0.0.0",
            "next_hop6": "2001:db8::a",
        }
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 next-hop4 0.0.0.0", commands
        )
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 next-hop6 2001:db8::a", commands
        )

    def test_link_bandwidth_string(self) -> None:
        """link_bandwidth_bps as string should be used directly."""
        peer = {"peer_addr": "2001:db8::1", "link_bandwidth_bps": "10G"}
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 link-bandwidth 10G", commands
        )

    def test_link_bandwidth_int(self) -> None:
        """link_bandwidth_bps as int should be formatted."""
        peer = {"peer_addr": "2001:db8::1", "link_bandwidth_bps": 10_000_000_000}
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 link-bandwidth 10G", commands
        )

    def test_advertise_link_bandwidth(self) -> None:
        """advertise_link_bandwidth should generate advertise-lbw command."""
        peer = {"peer_addr": "2001:db8::1", "advertise_link_bandwidth": 1}
        commands = generate_peer_commands(peer)
        self.assertIn("config protocol bgp peer 2001:db8::1 advertise-lbw 1", commands)

    def test_peer_id_and_type(self) -> None:
        """peer_id and type should generate correct commands."""
        peer = {
            "peer_addr": "2001:db8::1",
            "peer_id": "test-peer:v6:1",
            "type": "BGP_MONITOR",
        }
        commands = generate_peer_commands(peer)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 peer-id test-peer:v6:1", commands
        )
        self.assertIn("config protocol bgp peer 2001:db8::1 type BGP_MONITOR", commands)

    def test_timers(self) -> None:
        """Timer fields should generate correct commands."""
        peer = {
            "peer_addr": "2001:db8::1",
            "bgp_peer_timers": {
                "hold_time_seconds": 120,
                "keep_alive_seconds": 40,
                "out_delay_seconds": 0,
                "withdraw_unprog_delay_seconds": 0,
            },
        }
        commands = generate_peer_commands(peer)
        self.assertIn("config protocol bgp peer 2001:db8::1 hold-time 120", commands)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 timers keepalive 40", commands
        )

    def test_pre_filter(self) -> None:
        """pre_filter fields should generate correct commands."""
        peer = {
            "peer_addr": "2001:db8::1",
            "pre_filter": {
                "max_routes": 45000,
                "warning_limit": 0,
                "warning_only": False,
            },
        }
        commands = generate_peer_commands(peer)
        self.assertIn("config protocol bgp peer 2001:db8::1 max-routes 45000", commands)
        self.assertIn("config protocol bgp peer 2001:db8::1 warning-limit 0", commands)
        self.assertIn(
            "config protocol bgp peer 2001:db8::1 warning-only false", commands
        )


class GenerateScriptCommandsTest(unittest.TestCase):
    """Tests for generate_exec_commands and generate_stub_commands."""

    def test_exec_commands_contains_header(self) -> None:
        """Exec commands should contain shebang and header."""
        commands = ["test command"]
        result = generate_exec_commands(commands, "fboss2")
        self.assertIn("#!/bin/bash", result)
        # Check for set -e with any comment
        self.assertTrue(any("set -e" in cmd for cmd in result))

    def test_exec_commands_prepends_binary(self) -> None:
        """Exec commands should prepend binary to each command."""
        commands = ["config protocol bgp global router-id 10.0.0.1"]
        result = generate_exec_commands(commands, "/path/to/fboss2")
        self.assertIn(
            "/path/to/fboss2 config protocol bgp global router-id 10.0.0.1", result
        )

    def test_stub_commands_contains_stub_env(self) -> None:
        """Stub commands should set FBOSS_BGP_STUB_MODE."""
        commands = ["test command"]
        result = generate_stub_commands(commands, "fboss2")
        self.assertIn("export FBOSS_BGP_STUB_MODE=1", result)


class JsonToCliIntegrationTest(unittest.TestCase):
    """Integration tests for json_to_cli function."""

    def test_minimal_config(self) -> None:
        """Minimal config with just router_id should work."""
        config = {"router_id": "10.0.0.1"}
        commands = json_to_cli(config, stub=False, binary="fboss2")
        self.assertTrue(any("router-id 10.0.0.1" in cmd for cmd in commands))

    def test_full_rsw_config(self) -> None:
        """Full RSW-like config should generate all expected commands."""
        config = {
            "router_id": "0.17.1.1",
            "networks6": [
                {
                    "prefix": "2401:db00:501c::/64",
                    "policy_name": "ORIGINATE_RACK_PRIVATE_PREFIXES",
                    "install_to_fib": False,
                    "minimum_supporting_routes": 0,
                }
            ],
            "switch_limit_config": {
                "prefix_limit": 10000,
                "total_path_limit": 250000,
                "max_golden_vips": 2860,
                "overload_protection_mode": 2,
            },
            "peer_groups": [
                {
                    "name": "RSW-FSW-V6",
                    "next_hop_self": True,
                    "is_confed_peer": True,
                    "v4_over_v6_nexthop": True,
                    "bgp_peer_timers": {
                        "hold_time_seconds": 30,
                        "keep_alive_seconds": 10,
                    },
                },
                {
                    "name": "RSW-RTSW-V6",
                    "disable_ipv4_afi": True,
                },
            ],
            "peers": [
                {
                    "peer_addr": "2401:db00:501c::/64",
                    "remote_as_4_byte": 65000,
                    "disable_ipv4_afi": False,
                    "is_passive": True,
                },
                {
                    "peer_addr": "2401:db00:e50e:1000::",
                    "peer_group_name": "RSW-FSW-V6",
                    "remote_as_4_byte": 6001,
                },
            ],
        }

        commands = json_to_cli(config, stub=False, binary="fboss2")
        joined = "\n".join(commands)

        # Verify global commands
        self.assertIn("router-id 0.17.1.1", joined)
        self.assertIn("network6 add 2401:db00:501c::/64", joined)
        self.assertIn("install-to-fib false", joined)
        self.assertIn("switch-limit 10000", joined)
        self.assertIn("switch-limit-total-path 250000", joined)

        # Verify peer group commands
        self.assertIn("peer-group RSW-FSW-V6 next-hop-self true", joined)
        self.assertIn("peer-group RSW-FSW-V6 confed-peer true", joined)
        self.assertIn("peer-group RSW-RTSW-V6 disable-ipv4-afi true", joined)

        # Verify peer commands
        self.assertIn("peer 2401:db00:501c::/64 remote-asn 65000", joined)
        self.assertIn("peer 2401:db00:501c::/64 disable-ipv4-afi false", joined)
        self.assertIn("peer 2401:db00:e50e:1000:: peer-group RSW-FSW-V6", joined)

    def test_stub_mode(self) -> None:
        """Stub mode should set FBOSS_BGP_STUB_MODE environment variable."""
        config = {"router_id": "10.0.0.1"}
        commands = json_to_cli(config, stub=True, binary="fboss2")
        self.assertIn("export FBOSS_BGP_STUB_MODE=1", commands)

    def test_empty_config(self) -> None:
        """Empty config should produce script with no commands."""
        config = {}
        commands = json_to_cli(config, stub=False, binary="fboss2")
        # Should only have header and footer, no actual BGP commands
        self.assertTrue(any("#!/bin/bash" in cmd for cmd in commands))

    def test_custom_binary(self) -> None:
        """Custom binary path should be used in commands."""
        config = {"router_id": "10.0.0.1"}
        commands = json_to_cli(config, stub=False, binary="/custom/path/fboss2")
        self.assertTrue(any("/custom/path/fboss2" in cmd for cmd in commands))


if __name__ == "__main__":
    unittest.main()
