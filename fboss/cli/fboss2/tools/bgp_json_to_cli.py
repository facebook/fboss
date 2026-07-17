#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-unsafe

"""
BGP++ JSON to fboss2 CLI converter (j2c tool)

Converts BGP++ thrift-compatible JSON configuration to fboss2 CLI commands.
This enables round-trip conversion: JSON -> CLI -> JSON (via fboss2).

Usage:
    python3 bgp_json_to_cli.py <input.json> [--output output.sh]

Example:
    python3 bgp_json_to_cli.py rsw_bgp_config.json > configure_bgp.sh
"""

import argparse
import json
import shlex
import sys
from typing import Any


def escape_shell_arg(arg: str) -> str:
    """Escape argument for shell usage."""
    return shlex.quote(str(arg))


def _generate_global_basic_commands(config: dict[str, Any]) -> list[str]:
    """Generate basic global BGP commands."""
    commands = []
    if "router_id" in config:
        commands.append(
            f"config protocol bgp global router-id {escape_shell_arg(config['router_id'])}"
        )
    if "local_as_4_byte" in config:
        commands.append(
            f"config protocol bgp global local-asn {escape_shell_arg(str(config['local_as_4_byte']))}"
        )
    if "hold_time" in config:
        commands.append(
            f"config protocol bgp global hold-time {escape_shell_arg(str(config['hold_time']))}"
        )
    if "local_confed_as_4_byte" in config:
        commands.append(
            f"config protocol bgp global confed-asn {escape_shell_arg(str(config['local_confed_as_4_byte']))}"
        )
    return commands


def _generate_network6_commands(networks: list[dict[str, Any]]) -> list[str]:
    """Generate network6 commands for global BGP config."""
    commands = []
    for network in networks:
        prefix = network.get("prefix", "")
        if not prefix:
            continue
        cmd_parts = [
            f"config protocol bgp global network6 add {escape_shell_arg(prefix)}"
        ]
        if "policy_name" in network:
            cmd_parts.append(f"policy {escape_shell_arg(network['policy_name'])}")
        if "install_to_fib" in network:
            cmd_parts.append(f"install-to-fib {str(network['install_to_fib']).lower()}")
        if "minimum_supporting_routes" in network:
            cmd_parts.append(
                f"min-routes {escape_shell_arg(str(network['minimum_supporting_routes']))}"
            )
        commands.append(" ".join(cmd_parts))
    return commands


def _generate_switch_limit_commands(switch_limit: dict[str, Any]) -> list[str]:
    """Generate switch_limit_config commands."""
    commands = []
    if "prefix_limit" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit {escape_shell_arg(str(switch_limit['prefix_limit']))}"
        )
    if "total_path_limit" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-total-path {escape_shell_arg(str(switch_limit['total_path_limit']))}"
        )
    if "max_golden_vips" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-max-golden-vips {escape_shell_arg(str(switch_limit['max_golden_vips']))}"
        )
    if "overload_protection_mode" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-overload-protection-mode {escape_shell_arg(str(switch_limit['overload_protection_mode']))}"
        )
    return commands


def generate_global_commands(config: dict[str, Any]) -> list[str]:
    """Generate CLI commands for global BGP settings."""
    commands = []
    commands.extend(_generate_global_basic_commands(config))
    commands.extend(_generate_network6_commands(config.get("networks6", [])))
    commands.extend(
        _generate_switch_limit_commands(config.get("switch_limit_config", {}))
    )
    return commands


def _generate_peer_group_basic_commands(
    peer_group: dict[str, Any], escaped_name: str
) -> list[str]:
    """Generate basic peer-group commands (remote-asn, description, policies)."""
    commands = []
    if "remote_as_4_byte" in peer_group:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} remote-asn {escape_shell_arg(str(peer_group['remote_as_4_byte']))}"
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
    peer_group: dict[str, Any], escaped_name: str
) -> list[str]:
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
    timers: dict[str, Any], escaped_name: str
) -> list[str]:
    """Generate timer commands for peer-group."""
    commands = []
    if timers.get("hold_time_seconds"):
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers hold-time {escape_shell_arg(str(timers['hold_time_seconds']))}"
        )
    if timers.get("keep_alive_seconds"):
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers keepalive {escape_shell_arg(str(timers['keep_alive_seconds']))}"
        )
    if "out_delay_seconds" in timers:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers out-delay {escape_shell_arg(str(timers['out_delay_seconds']))}"
        )
    if "withdraw_unprog_delay_seconds" in timers:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} timers withdraw-unprog-delay {escape_shell_arg(str(timers['withdraw_unprog_delay_seconds']))}"
        )
    return commands


def _generate_peer_group_prefilter_commands(
    pre_filter: dict[str, Any], escaped_name: str
) -> list[str]:
    """Generate pre_filter commands for peer-group."""
    commands = []
    if pre_filter.get("max_routes"):
        commands.append(
            f"config protocol bgp peer-group {escaped_name} max-routes {escape_shell_arg(str(pre_filter['max_routes']))}"
        )
    if "warning_limit" in pre_filter:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} warning-limit {escape_shell_arg(str(pre_filter['warning_limit']))}"
        )
    if "warning_only" in pre_filter:
        commands.append(
            f"config protocol bgp peer-group {escaped_name} warning-only {str(pre_filter['warning_only']).lower()}"
        )
    return commands


def generate_peer_group_commands(peer_group: dict[str, Any]) -> list[str]:
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
    if bps >= 1_000_000_000 and bps % 1_000_000_000 == 0:
        return f"{bps // 1_000_000_000}G"
    if bps >= 1_000_000 and bps % 1_000_000 == 0:
        return f"{bps // 1_000_000}M"
    if bps >= 1_000 and bps % 1_000 == 0:
        return f"{bps // 1_000}K"
    return str(bps)


# (json field, CLI attribute, escape as free-form shell arg) — fields whose
# value maps 1:1 onto a neighbor attribute token.
_NEIGHBOR_SCALAR_FIELDS = [
    ("remote_as_4_byte", "remote-asn", False),
    ("local_as_4_byte", "local-asn", False),
    ("peer_group_name", "peer-group", True),
    ("description", "description", True),
    ("peer_tag", "peer-tag", True),
    ("local_addr", "bind-addr address", True),
    ("ingress_policy_name", "ingress-policy", True),
    ("egress_policy_name", "egress-policy", True),
    ("next_hop4", "next-hop4", True),
    ("next_hop6", "next-hop6", True),
    ("peer_id", "peer-id", True),
    ("type", "type", True),
]


def _generate_neighbor_basic_commands(peer: dict[str, Any], prefix: str) -> list[str]:
    """Generate scalar neighbor commands (ASNs, names, policies, addresses)."""
    commands = []
    for field, cli_name, escape in _NEIGHBOR_SCALAR_FIELDS:
        if field in peer:
            value = escape_shell_arg(str(peer[field])) if escape else peer[field]
            commands.append(f"{prefix} {cli_name} {value}")
    if "is_passive" in peer:
        mode = "PASSIVE" if peer["is_passive"] else "ACTIVE"
        commands.append(f"{prefix} connect-mode {mode}")
    if "link_bandwidth_bps" in peer:
        bw = peer["link_bandwidth_bps"]
        bw_str = format_bandwidth(bw) if isinstance(bw, int) else str(bw)
        commands.append(f"{prefix} link-bandwidth {bw_str}")
    if "advertise_link_bandwidth" in peer:
        commands.append(f"{prefix} advertise-lbw {peer['advertise_link_bandwidth']}")
    if "receive_link_bandwidth" in peer:
        commands.append(f"{prefix} receive-lbw {peer['receive_link_bandwidth']}")
    return commands


def _generate_neighbor_bool_commands(peer: dict[str, Any], prefix: str) -> list[str]:
    """Generate boolean flag commands for a neighbor."""
    commands = []
    bool_fields = [
        ("is_rr_client", "rr-client"),
        ("is_confed_peer", "confed-peer"),
        ("is_redistribute_peer", "redistribute-peer"),
        ("enhanced_route_refresh", "enhanced-route-refresh"),
        ("disable_ipv4_afi", "afi disable-ipv4-afi"),
        ("disable_ipv6_afi", "afi disable-ipv6-afi"),
        ("v4_over_v6_nexthop", "afi ipv4-over-ipv6-nh"),
        ("enable_stateful_ha", "graceful-restart stateful-ha"),
        ("next_hop_self", "next-hop-self"),
    ]
    for field, cli_name in bool_fields:
        if field in peer:
            commands.append(f"{prefix} {cli_name} {str(peer[field]).lower()}")
    return commands


def _generate_neighbor_add_path_commands(
    peer: dict[str, Any], prefix: str
) -> list[str]:
    """Generate add-path commands from the AddPath enum (RECEIVE=1, SEND=2, BOTH=3)."""
    if "add_path" not in peer:
        return []
    raw = peer["add_path"]
    value = (
        {"RECEIVE": 1, "SEND": 2, "BOTH": 3}.get(raw, raw)
        if isinstance(raw, str)
        else raw
    )
    value = int(value)
    commands = []
    if value & 1:
        commands.append(f"{prefix} add-path receive true")
    if value & 2:
        commands.append(f"{prefix} add-path send true")
    return commands


def _generate_neighbor_timer_commands(timers: dict[str, Any], prefix: str) -> list[str]:
    """Generate timer and graceful-restart commands for a neighbor."""
    commands = []
    if "hold_time_seconds" in timers:
        commands.append(f"{prefix} timers hold-time {timers['hold_time_seconds']}")
    if "keep_alive_seconds" in timers:
        commands.append(f"{prefix} timers keepalive {timers['keep_alive_seconds']}")
    if "out_delay_seconds" in timers:
        commands.append(f"{prefix} timers out-delay {timers['out_delay_seconds']}")
    if "withdraw_unprog_delay_seconds" in timers:
        commands.append(
            f"{prefix} timers withdraw-unprog-delay {timers['withdraw_unprog_delay_seconds']}"
        )
    if "graceful_restart_seconds" in timers:
        commands.append(
            f"{prefix} graceful-restart restart-time {timers['graceful_restart_seconds']}"
        )
    return commands


def _generate_neighbor_route_limit_commands(
    peer: dict[str, Any], prefix: str
) -> list[str]:
    """Generate max-route commands from pre_filter/post_filter."""
    commands = []
    pre_filter = peer.get("pre_filter", {})
    if "max_routes" in pre_filter:
        commands.append(f"{prefix} max-route pre-filter {pre_filter['max_routes']}")
    if "warning_limit" in pre_filter:
        commands.append(
            f"{prefix} max-route pre-warning-threshold {pre_filter['warning_limit']}"
        )
    if "warning_only" in pre_filter:
        commands.append(
            f"{prefix} max-route pre-warning-only {str(pre_filter['warning_only']).lower()}"
        )
    post_filter = peer.get("post_filter", {})
    if "max_routes" in post_filter:
        commands.append(f"{prefix} max-route post-filter {post_filter['max_routes']}")
    if "warning_limit" in post_filter:
        commands.append(
            f"{prefix} max-route post-warning-threshold {post_filter['warning_limit']}"
        )
    if "warning_only" in post_filter:
        commands.append(
            f"{prefix} max-route post-warning-only {str(post_filter['warning_only']).lower()}"
        )
    return commands


def generate_peer_commands(peer: dict[str, Any]) -> list[str]:
    """Generate `config protocol bgp neighbor` CLI commands for a peer."""
    peer_addr = peer.get("peer_addr", "")
    if not peer_addr:
        return []

    prefix = f"config protocol bgp neighbor {escape_shell_arg(peer_addr)}"
    commands = []
    commands.extend(_generate_neighbor_basic_commands(peer, prefix))
    commands.extend(_generate_neighbor_bool_commands(peer, prefix))
    commands.extend(_generate_neighbor_add_path_commands(peer, prefix))
    commands.extend(
        _generate_neighbor_timer_commands(peer.get("bgp_peer_timers", {}), prefix)
    )
    commands.extend(_generate_neighbor_route_limit_commands(peer, prefix))
    return commands


def generate_exec_commands(commands: list[str], binary: str = "fboss2") -> list[str]:
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


def json_to_cli(config: dict[str, Any], binary: str = "fboss2") -> list[str]:
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
        with open(args.input) as f:
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
        commands = json_to_cli(config, binary=args.binary)
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
