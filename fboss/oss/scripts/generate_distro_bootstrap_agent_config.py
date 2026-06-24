#!/usr/bin/env python3
"""Generate a basic bootstrap FBOSS agent.conf JSON from platform description CSV files.

The agent config will have one L3 interface/port per front-panel port configure for maximum speed.

Usage:
    generate_agent_config.py --platform <name> [--platforms-dir <path>]
                             [--asic-config-source <path>] [--output <path>]

Example:
    generate_agent_config.py --platform minipack3 --output agent.conf
    generate_agent_config.py --platform wedge800bact
"""

import argparse
import csv
import json
import pathlib
import re
import sys

# Profile ID → speed_mbps, derived from switch_config.thrift PortProfileID enum.
PROFILE_SPEED: dict[int, int] = {
    0: 0,
    1: 10000,
    3: 25000,
    8: 100000,
    9: 200000,
    10: 400000,
    11: 10000,
    14: 25000,
    19: 50000,
    21: 50000,
    22: 100000,
    23: 100000,
    24: 200000,
    25: 200000,
    26: 400000,
    32: 400000,
    35: 400000,
    36: 53000,
    37: 53000,
    38: 400000,
    39: 800000,
    41: 106000,
    42: 106000,
    43: 400000,
    45: 400000,
    47: 100000,
    49: 100000,
    50: 800000,
    54: 800000,
    55: 800000,
    56: 800000,
}

OPTICAL_PROFILES = {1, 3, 8, 9, 10, 23, 25, 26, 37, 38, 39, 42, 47, 54, 55, 56}

CORE_TYPE_TO_ASIC: dict[str, int] = {
    "TH5_NIF": 13,
    "J3_NIF": 15,
}

DEFAULT_ASIC_TYPE = 13

# FBOSS derives the kernel routing table ID by subtracting a type-specific
# constant from the intfID (2000 for port-based interfaces).  We start intfIDs
# at INTF_ID_OFFSET + 1 so that routing table IDs begin at 1, avoiding
# kernel-reserved IDs (0, 253, 254, 255).
INTF_ID_OFFSET = 2000

# Map every supported --platform value to the FBOSS platform_mapping_v2
# directory name (codename).  Entries are only needed when the two differ;
# platforms whose name already matches (wedge800bact, ...) are
# resolved automatically by _resolve_codename().
PLATFORM_CODENAME: dict[str, str] = {
    # dmidecode / run-config name  ->  platform_mapping_v2 codename
    "minipack3": "montblanc",
}


def _resolve_codename(platform: str, platforms_dir: pathlib.Path | None = None) -> str:
    """Resolve a platform name to the FBOSS platform_mapping_v2 codename.

    1. Direct match — platform dir exists under platforms_dir.
    2. Explicit alias — PLATFORM_CODENAME lookup.
    3. Normalised match — lowercase, strip hyphens/underscores/spaces
       (mimics fboss_init.sh logic), then retry 1 and 2.
    """
    # 1. Direct directory match
    if platforms_dir and (platforms_dir / platform).is_dir():
        return platform

    # 2. Explicit alias
    if platform in PLATFORM_CODENAME:
        return PLATFORM_CODENAME[platform]

    # 3. Normalise and retry
    normalised = platform.lower().replace("-", "").replace("_", "").replace(" ", "")
    if normalised != platform:
        if platforms_dir and (platforms_dir / normalised).is_dir():
            return normalised
        if normalised in PLATFORM_CODENAME:
            return PLATFORM_CODENAME[normalised]

    # Fall through — caller will use the original name and get a
    # FileNotFoundError with a clear message.
    return platform


def _find_platforms_dir(script_path: pathlib.Path) -> pathlib.Path:
    """Walk up from the script to locate the platform_mapping_v2/platforms dir."""
    candidates = [
        script_path.parent.parent.parent / "lib" / "platform_mapping_v2" / "platforms",
        script_path.parent.parent.parent.parent
        / "fboss"
        / "lib"
        / "platform_mapping_v2"
        / "platforms",
    ]
    for c in candidates:
        if c.is_dir():
            return c
    raise FileNotFoundError(
        "Cannot auto-detect platforms directory. Use --platforms-dir."
    )


def _find_hw_test_config(
    codename: str, script_path: pathlib.Path
) -> pathlib.Path | None:
    """Locate the materialized HW test config for the resolved codename."""
    hw_test_dir = script_path.parent.parent / "hw_test_configs"
    if not hw_test_dir.is_dir():
        return None
    for suffix in ("agent.materialized_JSON", "materialized_JSON"):
        candidate = hw_test_dir / f"{codename}.{suffix}"
        if candidate.exists():
            return candidate
    return None


def _load_asic_config(
    source: pathlib.Path | None,
    platform: str,
    script_path: pathlib.Path,
) -> dict:
    """Load ASIC config from a materialized JSON or standalone YAML file.

    Auto-detects from hw_test_configs/ when *source* is None.
    Returns the dict for ``platform.chip.asicConfig.common``.
    """
    if source is None:
        source = _find_hw_test_config(platform, script_path)
    if source is None:
        print(
            f"WARNING: No ASIC config source found for '{platform}'. "
            "The generated config will have an empty platform section.",
            file=sys.stderr,
        )
        return {}

    if source.suffix in (".yml", ".yaml"):
        return {"yamlConfig": source.read_text()}

    with source.open() as fh:
        data = json.load(fh)
    try:
        return data["platform"]["chip"]["asicConfig"]["common"]
    except KeyError:
        print(
            f"WARNING: Could not extract platform.chip.asicConfig.common "
            f"from {source}.",
            file=sys.stderr,
        )
        return {}


def _read_csv(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def _detect_asic_type(static_rows: list[dict[str, str]]) -> int:
    """Infer asicType integer from core-type values in the static mapping."""
    for row in static_rows:
        for col in ("Z_CORE_TYPE", "A_CORE_TYPE"):
            core_type = row.get(col, "").strip()
            if core_type in CORE_TYPE_TO_ASIC:
                return CORE_TYPE_TO_ASIC[core_type]
    return DEFAULT_ASIC_TYPE


def _choose_profile(supported_profiles_str: str) -> tuple[int, int]:
    """Return (profile_id, speed_mbps) for the highest-speed optical profile.

    Falls back to the highest-speed profile overall when no optical profiles
    are available.
    """
    ids = [int(p) for p in supported_profiles_str.split("-") if p.strip()]
    if not ids:
        return 0, 0

    optical = [
        (pid, PROFILE_SPEED.get(pid, 0)) for pid in ids if pid in OPTICAL_PROFILES
    ]
    if optical:
        return max(optical, key=lambda x: x[1])

    all_speeds = [(pid, PROFILE_SPEED.get(pid, 0)) for pid in ids]
    return max(all_speeds, key=lambda x: x[1])


def _is_primary_port(port_name: str) -> bool:
    """Return True if the port name ends with /1 (primary port of an OSFP group)."""
    parts = port_name.rsplit("/", 1)
    return len(parts) == 2 and parts[1] == "1"


def _build_cpu_queues() -> list[dict]:
    return [
        {"id": 9, "streamType": 1, "scheduling": 1, "name": "cpuQueue-high"},
        {"id": 2, "streamType": 1, "scheduling": 1, "name": "cpuQueue-mid"},
        {
            "id": 1,
            "streamType": 1,
            "scheduling": 1,
            "name": "cpuQueue-default",
            "portQueueRate": {"pktsPerSec": {"minimum": 0, "maximum": 1000}},
        },
        {
            "id": 0,
            "streamType": 1,
            "scheduling": 1,
            "name": "cpuQueue-low",
            "portQueueRate": {"pktsPerSec": {"minimum": 0, "maximum": 500}},
        },
    ]


def _build_cpu_traffic_policy() -> dict:
    return {
        "rxReasonToQueueOrderedList": [
            {"rxReason": 8, "queueId": 9},
            {"rxReason": 1, "queueId": 9},
            {"rxReason": 10, "queueId": 9},
            {"rxReason": 11, "queueId": 9},
            {"rxReason": 12, "queueId": 9},
            {"rxReason": 13, "queueId": 9},
            {"rxReason": 9, "queueId": 2},
            {"rxReason": 7, "queueId": 2},
            {"rxReason": 2, "queueId": 2},
            {"rxReason": 17, "queueId": 2},
            {"rxReason": 6, "queueId": 0},
            {"rxReason": 14, "queueId": 0},
            {"rxReason": 0, "queueId": 1},
        ]
    }


def _build_load_balancers() -> list[dict]:
    """ECMP (L3+L4 hash) + AGGREGATE_PORT (L3-only hash), both CRC16."""
    return [
        {
            "id": 1,
            "fieldSelection": {
                "ipv4Fields": [1, 2],
                "ipv6Fields": [1, 2],
                "transportFields": [1, 2],
                "mplsFields": [],
                "udfGroups": [],
            },
            "algorithm": 1,
        },
        {
            "id": 2,
            "fieldSelection": {
                "ipv4Fields": [1, 2],
                "ipv6Fields": [1, 2],
                "transportFields": [],
                "mplsFields": [],
                "udfGroups": [],
            },
            "algorithm": 1,
        },
    ]


def _build_qos_policies() -> list[dict]:
    """8 DSCP blocks -> 8 traffic classes, 1:1 TC-to-queue."""
    dscp_maps = [
        {
            "internalTrafficClass": tc,
            "fromDscpToTrafficClass": list(range(tc * 8, tc * 8 + 8)),
        }
        for tc in range(8)
    ]
    return [
        {
            "name": "default",
            "qosMap": {
                "dscpMaps": dscp_maps,
                "expMaps": [],
                "trafficClassToQueueId": {str(i): i for i in range(8)},
            },
        }
    ]


def _port_name_sort_key(name: str) -> list[int | str]:
    """Natural sort key for port names like 'eth1/42/1'."""
    return [int(tok) if tok.isdigit() else tok for tok in re.split(r"(\d+)", name)]


def _select_primary_ports(ppm_rows: list[dict[str, str]]) -> list[dict[str, str]]:
    """Filter to primary INTERFACE ports (ending in /1), sorted by front-panel order."""
    selected = []
    for row in ppm_rows:
        port_name = row.get("Port_Name", "").strip()
        if (
            row.get("Port_Type", "0").strip() == "0"
            and _is_primary_port(port_name)
            and row.get("Supported_Port_Profiles", "").strip()
        ):
            selected.append(row)
    selected.sort(key=lambda r: _port_name_sort_key(r["Port_Name"].strip()))
    return selected


def generate_config(
    platform: str,
    platforms_dir: pathlib.Path,
    asic_config_source: pathlib.Path | None = None,
) -> dict:
    """Build and return the agent config dict.

    Uses port-based L3 interfaces (type=3).
    ASIC config is loaded from hw_test_configs/ materialized JSON by default.
    """
    codename = _resolve_codename(platform, platforms_dir)
    platform_dir = platforms_dir / codename
    if not platform_dir.is_dir():
        raise FileNotFoundError(f"Platform directory not found: {platform_dir}")

    ppm_path = platform_dir / f"{codename}_port_profile_mapping.csv"
    static_path = platform_dir / f"{codename}_static_mapping.csv"

    if not ppm_path.exists():
        raise FileNotFoundError(f"Port profile mapping not found: {ppm_path}")
    if not static_path.exists():
        raise FileNotFoundError(f"Static mapping not found: {static_path}")

    ppm_rows = _read_csv(ppm_path)
    static_rows = _read_csv(static_path)
    asic_type = _detect_asic_type(static_rows)
    selected_rows = _select_primary_ports(ppm_rows)

    # Build port and interface lists
    ports = []
    interfaces = []

    for idx, row in enumerate(selected_rows):
        global_port_id = int(row.get("Global_PortID", "0").strip())
        port_name = row.get("Port_Name", "").strip()
        scope_str = row.get("Scope", "0").strip()
        scope = int(scope_str) if scope_str.isdigit() else 0
        profile_id, speed = _choose_profile(
            row.get("Supported_Port_Profiles", "").strip()
        )

        ports.append(
            {
                "logicalID": global_port_id,
                "name": port_name,
                "state": 2,
                "speed": speed,
                "profileID": profile_id,
                "minFrameSize": 64,
                "maxFrameSize": 9412,
                "parserType": 1,
                "routable": True,
                "ingressVlan": 4094,
                "pause": {"rx": False, "tx": False},
                "sFlowIngressRate": 0,
                "sFlowEgressRate": 0,
                "loopbackMode": 0,
                "portType": 0,
                "drainState": 0,
                "scope": scope,
                "conditionalEntropyRehash": False,
            }
        )

        # intfIDs are sequential in front-panel order (2001, 2002, ...).
        # FBOSS derives routing table ID = intfID - 2000, so tables start at 1.
        intf_id = INTF_ID_OFFSET + idx + 1
        interfaces.append(
            {
                "intfID": intf_id,
                "routerID": 0,
                "portID": global_port_id,
                "ipAddresses": [],
                "mtu": 9412,
                "isVirtual": False,
                "isStateSyncDisabled": False,
                "type": 3,  # PORT
                "scope": 0,
            }
        )

    # Loopback interface (VLAN-based, virtual, no physical port)
    interfaces.insert(
        0,
        {
            "intfID": 10,
            "routerID": 0,
            "vlanID": 10,
            "ipAddresses": [],
            "mtu": 9412,
            "isVirtual": True,
            "isStateSyncDisabled": False,
            "type": 1,  # VLAN
            "scope": 0,
        },
    )

    vlans = [
        {
            "name": "fbossLoopback0",
            "id": 10,
            "recordStats": True,
            "routable": True,
            "ipAddresses": [],
        },
        {
            "name": "default",
            "id": 4094,
            "recordStats": True,
            "routable": False,
            "ipAddresses": [],
        },
    ]

    sw = {
        "version": 0,
        "ports": ports,
        "vlans": vlans,
        "vlanPorts": [],
        "defaultVlan": 4094,
        "interfaces": interfaces,
        "arpTimeoutSeconds": 60,
        "arpRefreshSeconds": 20,
        "arpAgerInterval": 5,
        "proactiveArp": False,
        "maxNeighborProbes": 300,
        "staleEntryInterval": 10,
        "staticRoutesWithNhops": [],
        "staticRoutesToNull": [],
        "staticRoutesToCPU": [],
        "acls": [],
        "aggregatePorts": [],
        "sFlowCollectors": [],
        "cpuQueues": _build_cpu_queues(),
        "cpuTrafficPolicy": _build_cpu_traffic_policy(),
        "loadBalancers": _build_load_balancers(),
        "mirrors": [],
        "trafficCounters": [],
        "qosPolicies": _build_qos_policies(),
        "defaultPortQueues": [],
        "staticMplsRoutesWithNhops": [],
        "staticMplsRoutesToNull": [],
        "staticMplsRoutesToCPU": [],
        "staticIp2MplsRoutes": [],
        "portQueueConfigs": {},
        "switchSettings": {
            "l2LearningMode": 0,
            "qcmEnable": False,
            "ptpTcEnable": False,
            "l2AgeTimerSeconds": 300,
            "maxRouteCounterIDs": 0,
            "blockNeighbors": [],
            "macAddrsToBlock": [],
            "switchType": 0,
            "exactMatchTableConfigs": [],
            "switchDrainState": 0,
            "switchIdToSwitchInfo": {
                "0": {
                    "switchType": 0,
                    "asicType": asic_type,
                    "switchIndex": 0,
                    "portIdRange": {"minimum": 0, "maximum": 2047},
                    "firmwareNameToFirmwareInfo": {},
                }
            },
            "vendorMacOuis": [],
            "metaMacOuis": [],
            "needL2EntryForNeighbor": True,
        },
        "dsfNodes": {},
        "defaultVoqConfig": [],
        "mirrorOnDropReports": [],
    }

    # Platform section — ASIC config from materialized HW test config
    script_path = pathlib.Path(__file__).resolve()
    asic_common = _load_asic_config(asic_config_source, codename, script_path)

    return {
        "defaultCommandLineArgs": {
            "check_wb_handles": "true",
            "cleanup_probed_kernel_data": "true",
            "counter_refresh_interval": "0",
            "disable_neighbor_updates": "false",
            "ecmp_width": "320",
            "enable_1to1_intf_route_table_mapping": "true",
            "enable_replayer": "true",
            "log_variable_name": "true",
            "multi_switch": "true",
            "publish_state_to_fsdb": "true",
            "publish_stats_to_fsdb": "true",
            "sai_configure_six_tap": "true",
            "use_full_dlb_scale": "true",
        },
        "sw": sw,
        "platform": {"chip": {"asicConfig": {"common": asic_common}}},
    }


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    script_path = pathlib.Path(__file__).resolve()
    try:
        default_platforms_dir = str(_find_platforms_dir(script_path))
    except FileNotFoundError:
        default_platforms_dir = None

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--platform",
        required=True,
        help="Platform name (e.g. montblanc, wedge800bact)",
    )
    parser.add_argument(
        "--platforms-dir",
        default=default_platforms_dir,
        help="Path to platform_mapping_v2/platforms (auto-detected by default)",
    )
    parser.add_argument(
        "--asic-config-source",
        default=None,
        help="Path to materialized JSON or YAML with ASIC config (auto-detected)",
    )
    parser.add_argument(
        "--output",
        default="-",
        help="Output file path (default: stdout)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(argv)

    if not args.platforms_dir:
        print(
            "ERROR: Could not auto-detect platforms directory. "
            "Pass --platforms-dir explicitly.",
            file=sys.stderr,
        )
        return 1

    platforms_dir = pathlib.Path(args.platforms_dir)
    if not platforms_dir.is_dir():
        print(f"ERROR: platforms directory not found: {platforms_dir}", file=sys.stderr)
        return 1

    asic_source = (
        pathlib.Path(args.asic_config_source) if args.asic_config_source else None
    )

    try:
        config = generate_config(
            platform=args.platform,
            platforms_dir=platforms_dir,
            asic_config_source=asic_source,
        )
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    output = json.dumps(config, indent=2, sort_keys=True)

    if args.output == "-":
        print(output)
    else:
        out_path = pathlib.Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(output + "\n")
        print(f"Wrote {out_path}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    sys.exit(main())
