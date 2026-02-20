#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
BGP++ JSON to fboss2 CLI converter (j2c tool)

Converts BGP++ thrift-compatible JSON configuration to fboss2 CLI commands.
This enables round-trip conversion: JSON -> CLI -> JSON (via fboss2).

Usage:
    python3 bgp_json_to_cli.py <input.json> [--output output.sh] [--stub]

Example:
    python3 bgp_json_to_cli.py rsw_bgp_config.json --stub > configure_bgp.sh
"""

import argparse
import json
import sys
from typing import Any, Dict, List


def escape_shell_arg(arg: str) -> str:
    """Escape argument for shell usage."""
    if " " in arg or '"' in arg or "'" in arg or any(c in arg for c in "!$`\\"):
        return f'"{arg}"'
    return arg


def _generate_global_basic_commands(config: Dict[str, Any]) -> List[str]:
    """Generate basic global BGP commands."""
    commands = []
    if "router_id" in config:
        commands.append(f"config protocol bgp global router-id {config['router_id']}")
    if "local_as_4_byte" in config:
        commands.append(
            f"config protocol bgp global local-asn {config['local_as_4_byte']}"
        )
    if "hold_time" in config:
        commands.append(f"config protocol bgp global hold-time {config['hold_time']}")
    if "local_confed_as_4_byte" in config:
        commands.append(
            f"config protocol bgp global confed-asn {config['local_confed_as_4_byte']}"
        )
    if "cluster_id" in config:
        commands.append(f"config protocol bgp global cluster-id {config['cluster_id']}")
    return commands


def _generate_network6_commands(networks: List[Dict[str, Any]]) -> List[str]:
    """Generate network6 commands for global BGP config."""
    commands = []
    for network in networks:
        prefix = network.get("prefix", "")
        if not prefix:
            continue
        cmd_parts = [f"config protocol bgp global network6 add {prefix}"]
        if "policy_name" in network:
            cmd_parts.append(f"policy {escape_shell_arg(network['policy_name'])}")
        if "install_to_fib" in network:
            cmd_parts.append(f"install-to-fib {str(network['install_to_fib']).lower()}")
        if "minimum_supporting_routes" in network:
            cmd_parts.append(f"min-routes {network['minimum_supporting_routes']}")
        commands.append(" ".join(cmd_parts))
    return commands


def _generate_switch_limit_commands(switch_limit: Dict[str, Any]) -> List[str]:
    """Generate switch_limit_config commands."""
    commands = []
    if "prefix_limit" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit {switch_limit['prefix_limit']}"
        )
    if "total_path_limit" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-total-path {switch_limit['total_path_limit']}"
        )
    if "max_golden_vips" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-max-golden-vips {switch_limit['max_golden_vips']}"
        )
    if "overload_protection_mode" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-overload-protection-mode {switch_limit['overload_protection_mode']}"
        )
    return commands


def generate_global_commands(config: Dict[str, Any]) -> List[str]:
    """Generate CLI commands for global BGP settings."""
    commands = []
    commands.extend(_generate_global_basic_commands(config))
    commands.extend(_generate_network6_commands(config.get("networks6", [])))
    commands.extend(
        _generate_switch_limit_commands(config.get("switch_limit_config", {}))
    )
    return commands


def _generate_peer_group_basic_commands(
    peer_group: Dict[str, Any], escaped_name: str
) -> List[str]:
    """Generate basic peer-group commands (remote-asn, description, policies)."""
    commands = []
    if "remote_as_4_byte" in peer_group:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} remote-asn {peer_group['remote_as_4_byte']}"
        )
    if "description" in peer_group:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} description {escape_shell_arg(peer_group['description'])}"
        )
    if "ingress_policy_name" in peer_group:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} ingress-policy {escape_shell_arg(peer_group['ingress_policy_name'])}"
        )
    if "egress_policy_name" in peer_group:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} egress-policy {escape_shell_arg(peer_group['egress_policy_name'])}"
        )
    if "peer_tag" in peer_group:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} peer-tag {escape_shell_arg(peer_group['peer_tag'])}"
        )
    return commands


def _generate_peer_group_bool_commands(
    peer_group: Dict[str, Any], escaped_name: str
) -> List[str]:
    """Generate boolean flag commands for peer-group."""
    commands = []
    bool_fields = [
        ("is_rr_client", "rr-client"),
        ("next_hop_self", "next-hop-self"),
        ("is_confed_peer", "confed-peer"),
        ("v4_over_v6_nexthop", "v4-over-v6-nh"),
        ("disable_ipv4_afi", "disable-ipv4-afi"),
    ]
    for field, cli_name in bool_fields:
        if field in peer_group:
            commands.append(
                f"config protocol bgp peer-group {escaped_name} {cli_name} {str(peer_group[field]).lower()}"
            )
    return commands


def _generate_peer_group_timer_commands(
    timers: Dict[str, Any], escaped_name: str
) -> List[str]:
    """Generate timer commands for peer-group."""
    commands = []
    if timers.get("hold_time_seconds"):
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers hold-time {timers['hold_time_seconds']}"
        )
    if timers.get("keep_alive_seconds"):
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers keepalive {timers['keep_alive_seconds']}"
        )
    if "out_delay_seconds" in timers:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers out-delay {timers['out_delay_seconds']}"
        )
    if "withdraw_unprog_delay_seconds" in timers:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers withdraw-unprog-delay {timers['withdraw_unprog_delay_seconds']}"
        )
    return commands


def _generate_peer_group_prefilter_commands(
    pre_filter: Dict[str, Any], escaped_name: str
) -> List[str]:
    """Generate pre_filter commands for peer-group."""
    commands = []
    if pre_filter.get("max_routes"):
        commands.append(
            f"config protocol bgp peer-group {escaped_name} max-routes {pre_filter['max_routes']}"
        )
    if "warning_limit" in pre_filter:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} warning-limit {pre_filter['warning_limit']}"
        )
    if "warning_only" in pre_filter:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} warning-only {str(pre_filter['warning_only']).lower()}"
        )
    return commands


def generate_peer_group_commands(peer_group: Dict[str, Any]) -> List[str]:
    """Generate CLI commands for a peer group."""
    name = peer_group.get("name", "")
    if not name:
        return []

    escaped_name = escape_shell_arg(name)
    commands = []
    commands.extend(_generate_peer_group_basic_commands(peer_group, escaped_name))
    commands.extend(_generate_peer_group_bool_commands(peer_group, escaped_name))
    commands.extend(
        _generate_peer_group_timer_commands(
            peer_group.get("bgp_peer_timers", {}), escaped_name
        )
    )
    commands.extend(
        _generate_peer_group_prefilter_commands(
            peer_group.get("pre_filter", {}), escaped_name
        )
    )
    return commands


def format_bandwidth(bps: int) -> str:
    """Format bandwidth in bps to human-readable format (e.g., 10G)."""
    if bps >= 1_000_000_000_000 and bps % 1_000_000_000_000 == 0:
        return f"{bps // 1_000_000_000_000}T"
    elif bps >= 1_000_000_000 and bps % 1_000_000_000 == 0:
        return f"{bps // 1_000_000_000}G"
    elif bps >= 1_000_000 and bps % 1_000_000 == 0:
        return f"{bps // 1_000_000}M"
    elif bps >= 1_000 and bps % 1_000 == 0:
        return f"{bps // 1_000}K"
    return str(bps)


def _generate_peer_basic_commands(peer: Dict[str, Any], peer_addr: str) -> List[str]:
    """Generate basic peer commands (remote-asn, peer-group, description, etc.)."""
    commands = []
    if "remote_as_4_byte" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} remote-asn {peer['remote_as_4_byte']}"
        )
    if "peer_group_name" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} peer-group {escape_shell_arg(peer['peer_group_name'])}"
        )
    if "description" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} description {escape_shell_arg(peer['description'])}"
        )
    if "local_addr" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} local-addr {peer['local_addr']}"
        )
    if "ingress_policy_name" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} ingress-policy {escape_shell_arg(peer['ingress_policy_name'])}"
        )
    if "egress_policy_name" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} egress-policy {escape_shell_arg(peer['egress_policy_name'])}"
        )
    return commands


def _generate_peer_bool_commands(peer: Dict[str, Any], peer_addr: str) -> List[str]:
    """Generate boolean flag commands for peer."""
    commands = []
    bool_fields = [
        ("is_passive", "passive"),
        ("is_rr_client", "rr-client"),
        ("next_hop_self", "next-hop-self"),
        ("v4_over_v6_nexthop", "v4-over-v6-nh"),
        ("disable_ipv4_afi", "disable-ipv4-afi"),
        ("advertise_link_bandwidth", "advertise-lbw"),
    ]
    for field, cli_name in bool_fields:
        if field in peer:
            commands.append(
                f"config protocol bgp peer {peer_addr} {cli_name} {str(peer[field]).lower()}"
            )
    return commands


def _generate_peer_nexthop_commands(peer: Dict[str, Any], peer_addr: str) -> List[str]:
    """Generate next-hop and link-bandwidth commands for peer."""
    commands = []
    if "next_hop4" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} next-hop4 {peer['next_hop4']}"
        )
    if "next_hop6" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} next-hop6 {peer['next_hop6']}"
        )
    if "link_bandwidth_bps" in peer:
        bw = peer["link_bandwidth_bps"]
        bw_str = format_bandwidth(bw) if isinstance(bw, int) else str(bw)
        commands.append(f"config protocol bgp peer {peer_addr} link-bandwidth {bw_str}")
    return commands


def _generate_peer_timer_commands(timers: Dict[str, Any], peer_addr: str) -> List[str]:
    """Generate timer commands for peer."""
    commands = []
    if timers.get("hold_time_seconds"):
        commands.append(
            f"config protocol bgp peer {peer_addr} hold-time {timers['hold_time_seconds']}"
        )
    if timers.get("keep_alive_seconds"):
        commands.append(
            f"config protocol bgp peer {peer_addr} timers keepalive {timers['keep_alive_seconds']}"
        )
    if "out_delay_seconds" in timers:
        commands.append(
            f"config protocol bgp peer {peer_addr} timers out-delay {timers['out_delay_seconds']}"
        )
    if "withdraw_unprog_delay_seconds" in timers:
        commands.append(
            f"config protocol bgp peer {peer_addr} timers withdraw-unprog-delay {timers['withdraw_unprog_delay_seconds']}"
        )
    return commands


def _generate_peer_prefilter_commands(
    pre_filter: Dict[str, Any], peer_addr: str
) -> List[str]:
    """Generate pre_filter commands for peer."""
    commands = []
    if pre_filter.get("max_routes"):
        commands.append(
            f"config protocol bgp peer {peer_addr} max-routes {pre_filter['max_routes']}"
        )
    if "warning_limit" in pre_filter:
        commands.append(
            f"config protocol bgp peer {peer_addr} warning-limit {pre_filter['warning_limit']}"
        )
    if "warning_only" in pre_filter:
        commands.append(
            f"config protocol bgp peer {peer_addr} warning-only {str(pre_filter['warning_only']).lower()}"
        )
    return commands


def _generate_peer_id_commands(peer: Dict[str, Any], peer_addr: str) -> List[str]:
    """Generate peer identification commands."""
    commands = []
    if "peer_id" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} peer-id {escape_shell_arg(peer['peer_id'])}"
        )
    if "type" in peer:
        commands.append(
            f"config protocol bgp peer {peer_addr} type {escape_shell_arg(peer['type'])}"
        )
    return commands


def generate_peer_commands(peer: Dict[str, Any]) -> List[str]:
    """Generate CLI commands for a peer."""
    peer_addr = peer.get("peer_addr", "")
    if not peer_addr:
        return []

    commands = []
    commands.extend(_generate_peer_basic_commands(peer, peer_addr))
    commands.extend(_generate_peer_bool_commands(peer, peer_addr))
    commands.extend(_generate_peer_nexthop_commands(peer, peer_addr))
    commands.extend(
        _generate_peer_timer_commands(peer.get("bgp_peer_timers", {}), peer_addr)
    )
    commands.extend(
        _generate_peer_prefilter_commands(peer.get("pre_filter", {}), peer_addr)
    )
    commands.extend(_generate_peer_id_commands(peer, peer_addr))
    return commands


def generate_stub_commands(commands: List[str], binary: str = "fboss2") -> List[str]:
    """Wrap commands for stub mode execution."""
    stub_commands = [
        "#!/bin/bash",
        "# BGP configuration script generated by j2c",
        "# Run with FBOSS_BGP_STUB_MODE=1 for testing",
        "",
        "export FBOSS_BGP_STUB_MODE=1",
        "",
    ]
    for cmd in commands:
        stub_commands.append(f"{binary} {cmd}")
    return stub_commands


def generate_exec_commands(commands: List[str], binary: str = "fboss2") -> List[str]:
    """Generate executable commands with custom binary."""
    exec_commands = [
        "#!/bin/bash",
        "# BGP configuration script generated by j2c",
        "# This script runs in normal mode (writes to ~/.fboss2/bgp_config.json)",
        "",
        "set -e  # Exit on first error",
        "",
    ]
    for cmd in commands:
        exec_commands.append(f"{binary} {cmd}")

    exec_commands.extend(
        [
            "",
            "echo ''",
            "echo 'BGP configuration complete!'",
            "echo 'Config saved to: ~/.fboss2/bgp_config.json'",
        ]
    )
    return exec_commands


def json_to_cli(
    config: Dict[str, Any], stub: bool = False, binary: str = "fboss2"
) -> List[str]:
    """Convert BGP++ JSON config to CLI commands."""
    commands = []

    # Generate global commands
    commands.extend(generate_global_commands(config))

    # Generate peer-group commands
    for peer_group in config.get("peer_groups", []):
        commands.extend(generate_peer_group_commands(peer_group))

    # Generate peer commands
    for peer in config.get("peers", []):
        commands.extend(generate_peer_commands(peer))

    if stub:
        return generate_stub_commands(commands, binary)

    return generate_exec_commands(commands, binary)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert BGP++ JSON configuration to fboss2 CLI commands"
    )
    parser.add_argument("input", help="Input JSON file")
    parser.add_argument(
        "--output", "-o", help="Output file (default: stdout)", default=None
    )
    parser.add_argument(
        "--stub",
        action="store_true",
        help="Generate stub mode script with FBOSS_BGP_STUB_MODE=1",
    )
    parser.add_argument(
        "--binary",
        "-b",
        default="fboss2",
        help="Binary name or path (default: fboss2). Use full path for buck run output.",
    )
    parser.add_argument(
        "--raw",
        action="store_true",
        help="Output raw CLI commands only (no shell script wrapper)",
    )
    args = parser.parse_args()

    try:
        with open(args.input, "r") as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"Error: Input file '{args.input}' not found", file=sys.stderr)
        return 1
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in '{args.input}': {e}", file=sys.stderr)
        return 1

    if args.raw:
        # Generate raw CLI commands only (no shell wrapper)
        commands = []
        commands.extend(generate_global_commands(config))
        for peer_group in config.get("peer_groups", []):
            commands.extend(generate_peer_group_commands(peer_group))
        for peer in config.get("peers", []):
            commands.extend(generate_peer_commands(peer))
        output = "\n".join(commands)
    else:
        commands = json_to_cli(config, stub=args.stub, binary=args.binary)
        output = "\n".join(commands)

    if args.output:
        with open(args.output, "w") as f:
            f.write(output + "\n")
        print(f"Generated {len(commands)} commands to {args.output}", file=sys.stderr)
    else:
        print(output)

    return 0


if __name__ == "__main__":
    sys.exit(main())
