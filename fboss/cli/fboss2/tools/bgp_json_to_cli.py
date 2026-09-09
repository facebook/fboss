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


def escape_shell_arg(arg: Any) -> str:
    """Quote a JSON-sourced value for the generated shell script.

    Applied to every value taken from the input JSON, numeric fields included:
    the script does not validate types, and shlex.quote leaves plain digits
    and identifiers unchanged, so valid input produces identical output.
    """
    return shlex.quote(str(arg))


def _shell_bool(value: Any) -> str:
    return escape_shell_arg(str(value).lower())


def _generate_global_basic_commands(config: dict[str, Any]) -> list[str]:
    """Generate basic global BGP commands."""
    commands = []
    if "router_id" in config:
        commands.append(
            f"config protocol bgp global router-id {escape_shell_arg(config['router_id'])}"
        )
    if "local_as_4_byte" in config:
        commands.append(
            f"config protocol bgp global local-asn {escape_shell_arg(config['local_as_4_byte'])}"
        )
    if "hold_time" in config:
        commands.append(
            f"config protocol bgp global hold-time {escape_shell_arg(config['hold_time'])}"
        )
    if "local_confed_as_4_byte" in config:
        commands.append(
            f"config protocol bgp global confed-asn {escape_shell_arg(config['local_confed_as_4_byte'])}"
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
            cmd_parts.append(f"install-to-fib {_shell_bool(network['install_to_fib'])}")
        if "minimum_supporting_routes" in network:
            cmd_parts.append(
                f"min-routes {escape_shell_arg(network['minimum_supporting_routes'])}"
            )
        commands.append(" ".join(cmd_parts))
    return commands


def _generate_switch_limit_commands(switch_limit: dict[str, Any]) -> list[str]:
    """Generate switch_limit_config commands."""
    commands = []
    if "prefix_limit" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit {escape_shell_arg(switch_limit['prefix_limit'])}"
        )
    if "total_path_limit" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-total-path {escape_shell_arg(switch_limit['total_path_limit'])}"
        )
    if "max_golden_vips" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-max-golden-vips {escape_shell_arg(switch_limit['max_golden_vips'])}"
        )
    if "overload_protection_mode" in switch_limit:
        commands.append(
            f"config protocol bgp global switch-limit-overload-protection-mode {escape_shell_arg(switch_limit['overload_protection_mode'])}"
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


def format_bandwidth(bps: int) -> str:
    """Format bandwidth in bps with the largest exact K/M/G suffix.

    Only K, M and G are emitted: those are the multipliers bgpd's
    link-bandwidth parser accepts, so a "T" spelling would not round-trip.
    """
    if bps >= 1_000_000_000 and bps % 1_000_000_000 == 0:
        return f"{bps // 1_000_000_000}G"
    if bps >= 1_000_000 and bps % 1_000_000 == 0:
        return f"{bps // 1_000_000}M"
    if bps >= 1_000 and bps % 1_000 == 0:
        return f"{bps // 1_000}K"
    return str(bps)


def _printable(text: Any) -> str:
    """A JSON-sourced name made safe for a `#` comment line: no control
    characters, so it cannot break out of the comment."""
    return "".join(ch if ch.isprintable() else "?" for ch in str(text))


# ---------------------------------------------------------------------------
# Fields BgpPeer and PeerGroup share. The neighbor and peer-group dispatchers
# spell these attributes identically, so one table drives both generators.
# ---------------------------------------------------------------------------

# (json field, CLI attribute) — boolean flags.
_BOOL_FIELDS = [
    ("is_rr_client", "rr-client"),
    ("is_confed_peer", "confed-peer"),
    ("is_redistribute_peer", "redistribute-peer"),
    ("enhanced_route_refresh", "enhanced-route-refresh"),
    ("route_refresh", "route-refresh"),
    ("remove_private_as", "remove-private-as"),
    ("enforce_first_as", "enforce-first-as"),
    ("disable_ipv4_afi", "afi disable-ipv4-afi"),
    ("disable_ipv6_afi", "afi disable-ipv6-afi"),
    ("v4_over_v6_nexthop", "afi ipv4-over-ipv6-nh"),
    ("enable_stateful_ha", "graceful-restart stateful-ha"),
    ("next_hop_self", "next-hop-self"),
]

# (json field, CLI attribute) — integer-valued attributes.
_INT_FIELDS = [("ttl_security_hops", "ttl-security-hops")]

_TIMER_FIELDS = [
    ("hold_time_seconds", "timers hold-time"),
    ("keep_alive_seconds", "timers keepalive"),
    ("out_delay_seconds", "timers out-delay"),
    ("withdraw_unprog_delay_seconds", "timers withdraw-unprog-delay"),
    ("graceful_restart_seconds", "graceful-restart restart-time"),
]

# RouteLimit leaves, as (json field, attribute suffix, is_bool); the prefix is
# `max-route pre-` or `max-route post-`.
_ROUTE_LIMIT_FIELDS = [
    ("max_routes", "filter", False),
    ("warning_limit", "warning-threshold", False),
    ("warning_only", "warning-only", True),
]

# Every shared key the generators below convert. Anything else present on the
# JSON object is reported (see _unconverted_field_warnings) instead of being
# dropped silently.
_SHARED_HANDLED_FIELDS = (
    {
        "is_passive",
        "link_bandwidth_bps",
        "advertise_link_bandwidth",
        "receive_link_bandwidth",
        "add_path",
        "bgp_peer_timers",
        "pre_filter",
        "post_filter",
    }
    | {field for field, _ in _BOOL_FIELDS}
    | {field for field, _ in _INT_FIELDS}
)
_TIMER_HANDLED_FIELDS = {field for field, _ in _TIMER_FIELDS}
_ROUTE_LIMIT_HANDLED_FIELDS = {field for field, _, _ in _ROUTE_LIMIT_FIELDS}


def _generate_scalar_commands(
    obj: dict[str, Any], prefix: str, fields: list[tuple[str, str]]
) -> list[str]:
    """Generate `<prefix> <attribute> <value>` for each present scalar field."""
    commands = []
    for field, cli_name in fields:
        if field in obj:
            commands.append(f"{prefix} {cli_name} {escape_shell_arg(obj[field])}")
    return commands


def _generate_bool_commands(obj: dict[str, Any], prefix: str) -> list[str]:
    """Generate boolean flag commands for a neighbor or peer group."""
    commands = []
    for field, cli_name in _BOOL_FIELDS:
        if field in obj:
            commands.append(f"{prefix} {cli_name} {_shell_bool(obj[field])}")
    return commands


def _generate_link_bandwidth_commands(obj: dict[str, Any], prefix: str) -> list[str]:
    """Generate the link-bandwidth trio for a neighbor or peer group.

    advertise/receive enum values are passed through as found (name or
    integer): the dispatchers accept both spellings for a thrift enum.
    """
    commands = []
    if "link_bandwidth_bps" in obj:
        bw = obj["link_bandwidth_bps"]
        bw_str = format_bandwidth(bw) if isinstance(bw, int) else str(bw)
        commands.append(f"{prefix} link-bandwidth {escape_shell_arg(bw_str)}")
    if "advertise_link_bandwidth" in obj:
        commands.append(
            f"{prefix} advertise-lbw {escape_shell_arg(obj['advertise_link_bandwidth'])}"
        )
    if "receive_link_bandwidth" in obj:
        commands.append(
            f"{prefix} receive-lbw {escape_shell_arg(obj['receive_link_bandwidth'])}"
        )
    return commands


def _generate_add_path_commands(obj: dict[str, Any], prefix: str) -> list[str]:
    """Generate add-path commands from the AddPath enum (RECEIVE=1, SEND=2, BOTH=3)."""
    if "add_path" not in obj:
        return []
    raw = obj["add_path"]
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


def _generate_timer_commands(timers: dict[str, Any], prefix: str) -> list[str]:
    """Generate timer and graceful-restart commands for a neighbor or peer group."""
    return _generate_scalar_commands(timers, prefix, _TIMER_FIELDS)


def _generate_route_limit_commands(obj: dict[str, Any], prefix: str) -> list[str]:
    """Generate max-route commands from pre_filter/post_filter."""
    commands = []
    for struct, side in (("pre_filter", "pre"), ("post_filter", "post")):
        limit = obj.get(struct, {})
        for field, suffix, is_bool in _ROUTE_LIMIT_FIELDS:
            if field in limit:
                value = (
                    _shell_bool(limit[field])
                    if is_bool
                    else escape_shell_arg(limit[field])
                )
                commands.append(f"{prefix} max-route {side}-{suffix} {value}")
    return commands


def _generate_session_commands(obj: dict[str, Any], prefix: str) -> list[str]:
    """Generate the commands for every field BgpPeer and PeerGroup share."""
    commands = []
    if "is_passive" in obj:
        commands.append(f"{prefix} passive {_shell_bool(obj['is_passive'])}")
    commands.extend(_generate_bool_commands(obj, prefix))
    commands.extend(_generate_scalar_commands(obj, prefix, _INT_FIELDS))
    commands.extend(_generate_link_bandwidth_commands(obj, prefix))
    commands.extend(_generate_add_path_commands(obj, prefix))
    commands.extend(_generate_timer_commands(obj.get("bgp_peer_timers", {}), prefix))
    commands.extend(_generate_route_limit_commands(obj, prefix))
    return commands


def _unconverted_field_warnings(
    obj: dict[str, Any], handled: set[str], subject: str
) -> list[str]:
    """One `# WARNING` comment line per JSON field no generator converts.

    The dispatchers only expose the fields bgpd reads at that level, so a
    field left here is either dead in the daemon or per-peer-only; either
    way the operator must see it was not carried over.
    """
    warnings = []

    def warn(field: str) -> None:
        warnings.append(
            f"# WARNING: {subject}: field {_printable(field)} has no CLI "
            "equivalent, not converted"
        )

    for field in obj:
        if field not in handled:
            warn(field)
    for field in obj.get("bgp_peer_timers", {}):
        if field not in _TIMER_HANDLED_FIELDS:
            warn(f"bgp_peer_timers.{field}")
    for struct in ("pre_filter", "post_filter"):
        for field in obj.get(struct, {}):
            if field not in _ROUTE_LIMIT_HANDLED_FIELDS:
                warn(f"{struct}.{field}")
    return warnings


# (json field, CLI attribute) — BgpPeer-only fields whose value maps 1:1 onto
# a neighbor attribute token. The deprecated i32 ASNs are converted to the
# 4-byte attribute: bgpd reads either representation and the CLI stores the
# 4-byte field.
_NEIGHBOR_SCALAR_FIELDS = [
    ("remote_as_4_byte", "remote-asn"),
    ("remote_as", "remote-asn"),
    ("local_as_4_byte", "local-asn"),
    ("local_as", "local-asn"),
    ("peer_group_name", "peer-group"),
    ("description", "description"),
    ("peer_tag", "peer-tag"),
    ("local_addr", "bind-addr address"),
    ("ingress_policy_name", "ingress-policy"),
    ("egress_policy_name", "egress-policy"),
    ("next_hop4", "next-hop4"),
    ("next_hop6", "next-hop6"),
    ("peer_id", "peer-id"),
    ("type", "type"),
]

_NEIGHBOR_HANDLED_FIELDS = (
    {"peer_addr"}
    | {field for field, _ in _NEIGHBOR_SCALAR_FIELDS}
    | _SHARED_HANDLED_FIELDS
)


def generate_peer_commands(peer: dict[str, Any]) -> list[str]:
    """Generate `config protocol bgp neighbor` CLI commands for a peer."""
    peer_addr = peer.get("peer_addr", "")
    if not peer_addr:
        return []

    prefix = f"config protocol bgp neighbor {escape_shell_arg(peer_addr)}"
    commands = _unconverted_field_warnings(
        peer, _NEIGHBOR_HANDLED_FIELDS, f"neighbor {_printable(peer_addr)}"
    )
    commands.extend(_generate_scalar_commands(peer, prefix, _NEIGHBOR_SCALAR_FIELDS))
    commands.extend(_generate_session_commands(peer, prefix))
    return commands


# (json field, CLI attribute) — PeerGroup-only fields whose value maps 1:1
# onto a peer-group attribute token.
_PEER_GROUP_SCALAR_FIELDS = [
    ("remote_as_4_byte", "remote-asn"),
    ("local_as_4_byte", "local-asn"),
    ("description", "description"),
    ("peer_tag", "peer-tag"),
    ("ingress_policy_name", "ingress-policy"),
    ("egress_policy_name", "egress-policy"),
]


def generate_peer_group_commands(peer_group: dict[str, Any]) -> list[str]:
    """Generate `config protocol bgp peer-group` CLI commands for a peer group.

    The peer-group and neighbor dispatchers share one attribute grammar for
    the fields both thrift structs carry, so the per-shape generators are
    shared; only the scalar field list differs.
    """
    name = peer_group.get("name", "")
    if not name:
        return []

    prefix = f"config protocol bgp peer-group {escape_shell_arg(name)}"
    commands = []
    commands.extend(
        _generate_scalar_commands(peer_group, prefix, _PEER_GROUP_SCALAR_FIELDS)
    )
    commands.extend(_generate_session_commands(peer_group, prefix))
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
        exec_commands.append(cmd if cmd.startswith("#") else f"{binary} {cmd}")

    exec_commands.extend(
        [
            "",
            "echo ''",
            "echo 'BGP configuration complete!'",
            "echo 'Config saved to: ~/.fboss2/bgp_config.json'",
        ]
    )
    return exec_commands


def generate_commands(config: dict[str, Any]) -> list[str]:
    """Convert BGP++ JSON config to raw CLI commands (plus `#` warning lines
    for fields that have no CLI equivalent)."""
    commands = []
    commands.extend(generate_global_commands(config))
    for peer_group in config.get("peer_groups", []):
        commands.extend(generate_peer_group_commands(peer_group))
    for peer in config.get("peers", []):
        commands.extend(generate_peer_commands(peer))
    return commands


def json_to_cli(config: dict[str, Any], binary: str = "fboss2") -> list[str]:
    """Convert BGP++ JSON config to an executable CLI script."""
    return generate_exec_commands(generate_commands(config), binary)


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

    commands = generate_commands(config)
    # Fields with no CLI equivalent are flagged in the script AND on stderr,
    # so a scripted conversion cannot lose them silently.
    for line in commands:
        if line.startswith("# WARNING"):
            print(line[2:], file=sys.stderr)
    if not args.raw:
        commands = generate_exec_commands(commands, args.binary)
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
