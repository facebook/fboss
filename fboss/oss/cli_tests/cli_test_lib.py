#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
Common library for CLI end-to-end tests.

This module provides shared utilities for CLI tests including:
- Finding and running the fboss2-dev CLI binary
- Parsing interface information from CLI output
- Running shell commands with proper error handling
"""

import json
import os
import subprocess
import time
from dataclasses import dataclass
from typing import Any, Callable, Optional


@dataclass
class Interface:
    """Represents a network interface from the output of 'show interface'."""

    name: str
    status: str
    speed: str
    vlan: Optional[int]
    mtu: int
    addresses: list[str]  # IPv4 and IPv6 addresses assigned to the interface
    description: str

    @staticmethod
    def from_json(data: dict[str, Any]) -> "Interface":
        """
        Parse interface data from JSON into an Interface object.

        The JSON format from 'fboss2 --fmt json show interface' is:
        {
            "name": "eth1/1/1",
            "description": "...",
            "status": "down",
            "speed": "800G",
            "vlan": 2001,
            "mtu": 1500,
            "prefixes": [{"ip": "10.0.0.0", "prefixLength": 24}, ...],
            ...
        }
        """
        # Convert prefixes to "ip/prefixLength" format
        prefixes = data.get("prefixes", [])
        addresses = [f"{p['ip']}/{p['prefixLength']}" for p in prefixes]

        return Interface(
            name=data["name"],
            status=data["status"],
            speed=data["speed"],
            vlan=data.get("vlan"),
            mtu=data["mtu"],
            addresses=addresses,
            description=data.get("description", ""),
        )


# CLI binary path - can be overridden via FBOSS_CLI_PATH environment variable
_FBOSS_CLI = None


def get_fboss_cli() -> str:
    """
    Get the path to the FBOSS CLI binary.

    The path can be overridden by setting the FBOSS_CLI_PATH environment variable.
    Example: FBOSS_CLI_PATH=/tmp/fboss2-dev python3 test_config_interface_mtu.py
    """
    global _FBOSS_CLI
    if _FBOSS_CLI is not None:
        return _FBOSS_CLI

    # Check if path is specified via environment variable
    env_path = os.environ.get("FBOSS_CLI_PATH")
    if env_path:
        expanded = os.path.expanduser(env_path)
        if os.path.isfile(expanded) and os.access(expanded, os.X_OK):
            _FBOSS_CLI = expanded
            print(f"  Using CLI from FBOSS_CLI_PATH: {_FBOSS_CLI}")
            return _FBOSS_CLI
        else:
            raise RuntimeError(
                f"FBOSS_CLI_PATH is set to '{env_path}' but the file does not exist "
                "or is not executable"
            )

    # Default locations (only fboss2-dev has config commands)
    candidates = (
        "/opt/fboss/bin/fboss2-dev",
        "fboss2-dev",
    )

    for candidate in candidates:
        if os.path.isabs(candidate):
            if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
                _FBOSS_CLI = candidate
                return _FBOSS_CLI
        else:
            # Check if it's in PATH
            result = subprocess.run(
                ["which", candidate], capture_output=True, text=True
            )
            if result.returncode == 0:
                _FBOSS_CLI = result.stdout.strip()
                return _FBOSS_CLI

    raise RuntimeError(
        "Could not find fboss2-dev CLI binary. "
        "Set FBOSS_CLI_PATH environment variable to specify the path."
    )


def run_cmd(cmd: list[str], check: bool = True) -> subprocess.CompletedProcess:
    """Run a command and return the result."""
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"Command failed with return code {result.returncode}")
        print(f"stdout: {result.stdout}")
        print(f"stderr: {result.stderr}")
        raise RuntimeError(f"Command failed: {' '.join(cmd)}")
    return result


def run_cli(args: list[str], check: bool = True) -> dict[str, Any]:
    """Run the fboss2-dev CLI with the given arguments.

    The --fmt json flag is automatically prepended to all commands.
    Returns the parsed JSON output as a dict.
    """
    cli = get_fboss_cli()
    cmd = [cli, "--fmt", "json"] + args
    print(f"[CLI] Running: {' '.join(args)}")
    start_time = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.time() - start_time
    print(f"[CLI] Completed in {elapsed:.2f}s: {' '.join(args)}")
    if check and result.returncode != 0:
        print(f"Command failed with return code {result.returncode}")
        print(f"stdout: {result.stdout}")
        print(f"stderr: {result.stderr}")
        raise RuntimeError(f"Command failed: {' '.join(cmd)}")
    return json.loads(result.stdout) if result.stdout.strip() else {}


def _get_interfaces(interface_name: Optional[str] = None) -> dict[str, Interface]:
    """
    Get interface information from 'fboss2-dev show interface [name]'.

    Args:
        interface_name: Optional interface name. If None, gets all interfaces.

    Returns a dict mapping interface name to Interface object.
    """
    args = ["show", "interface"]
    if interface_name is not None:
        args.append(interface_name)

    data = run_cli(args)

    # The JSON has a host key (e.g., "127.0.0.1") containing the interfaces
    interfaces: dict[str, Interface] = {}
    for host_data in data.values():
        for intf_data in host_data.get("interfaces", []):
            intf = Interface.from_json(intf_data)
            assert intf.name not in interfaces, f"Duplicate interface name: {intf.name}"
            interfaces[intf.name] = intf

    return interfaces


def get_all_interfaces() -> dict[str, Interface]:
    """
    Get all interface information from 'fboss2-dev show interface'.
    Returns a dict mapping interface name to Interface object.
    """
    return _get_interfaces()


def get_interface_info(interface_name: str) -> Interface:
    """
    Get interface information from 'fboss2-dev show interface <name>'.
    Returns an Interface object.
    """
    interfaces = _get_interfaces(interface_name)

    if interface_name not in interfaces:
        raise RuntimeError(f"Could not find interface {interface_name} in output")

    return interfaces[interface_name]


def find_interfaces(predicate: Callable[[Interface], bool]) -> list[Interface]:
    """
    Find all interfaces matching the given predicate.

    Args:
        predicate: A callable that takes an Interface and returns True
                   if the interface should be included in the results.

    Returns:
        A list of Interface objects for all matching interfaces.

    This calls the CLI only once via get_all_interfaces().
    """
    all_interfaces = get_all_interfaces()
    return [intf for intf in all_interfaces.values() if predicate(intf)]


def find_first_eth_interface() -> Interface:
    """
    Find the first suitable ethernet interface.
    Returns an Interface object.

    Only returns ethernet interfaces (starting with 'eth') with a valid VLAN > 1.
    """

    def is_valid_eth_interface(intf: Interface) -> bool:
        return intf.name.startswith("eth") and intf.vlan is not None and intf.vlan > 1

    matches = find_interfaces(is_valid_eth_interface)

    if not matches:
        raise RuntimeError("No suitable ethernet interface found with VLAN > 1")

    return matches[0]


def commit_config() -> None:
    """Commit the current configuration session."""
    run_cli(["config", "session", "commit"])


# Paths for config files
SYSTEM_CONFIG_PATH = "/etc/coop/agent.conf"
SESSION_CONFIG_PATH = os.path.expanduser("~/.fboss2/agent.conf")


def cleanup_config(
    modify_config: Callable[[dict[str, Any]], None],
    description: str = "test configs",
) -> None:
    """
    Common cleanup helper that modifies the config and commits the changes.

    This function:
    1. Copies the system config to the session config
    2. Loads the config JSON
    3. Calls the modify_config callback to modify the sw config
    4. Writes the modified config back
    5. Updates metadata for AGENT_RESTART
    6. Commits the cleanup

    Args:
        modify_config: A callable that takes the sw config dict and modifies it
                       in place to remove test-specific configurations.
        description: A description of what is being cleaned up (for logging).
    """
    import shutil

    session_dir = os.path.dirname(SESSION_CONFIG_PATH)
    metadata_path = os.path.join(session_dir, "cli_metadata.json")

    print("  Copying system config to session config...")
    os.makedirs(session_dir, exist_ok=True)
    shutil.copy(SYSTEM_CONFIG_PATH, SESSION_CONFIG_PATH)

    print(f"  Removing {description}...")
    with open(SESSION_CONFIG_PATH, "r") as f:
        config = json.load(f)

    sw_config = config.get("sw", {})
    modify_config(sw_config)

    with open(SESSION_CONFIG_PATH, "w") as f:
        json.dump(config, f, indent=2)

    # Update metadata to require AGENT_RESTART
    print("  Updating metadata for AGENT_RESTART...")
    metadata = {
        "action": {"WEDGE_AGENT": "AGENT_RESTART"},
        "commands": [],
        "base": "",
    }
    with open(metadata_path, "w") as f:
        json.dump(metadata, f, indent=2)

    print("  Committing cleanup...")
    commit_config()


def running_config() -> dict[str, Any]:
    """
    Get the running configuration from the FBOSS agent.

    Returns the nested JSON payload, skipping the initial 'localhost' level.
    This allows direct access to the configuration without needing to iterate
    over host keys.

    Returns:
        The configuration dict containing 'sw', 'platform', etc.
    """
    data = run_cli(["show", "running-config"])

    # The JSON has a host key (e.g., "localhost") containing a JSON string
    for host_data_str in data.values():
        # The value is a JSON string that needs to be parsed
        if isinstance(host_data_str, str):
            return json.loads(host_data_str)
        return host_data_str

    return {}


def wait_for_agent_ready(max_wait_seconds: int = 60) -> bool:
    """
    Wait for the wedge_agent to be ready after a restart.

    The agent restart typically takes 40-50 seconds. We wait for an initial
    period and then poll until the agent responds with valid data.

    Args:
        max_wait_seconds: Maximum time to wait for the agent to be ready.

    Returns:
        True if the agent is ready, False if timeout.
    """
    # Initial delay - agent restart takes significant time
    # This avoids noisy polling during the restart
    initial_wait = 30
    print(f"  Sleeping {initial_wait}s for agent restart...")
    time.sleep(initial_wait)

    # Now poll until agent is ready or timeout
    start_time = time.time()
    remaining = max_wait_seconds - initial_wait
    while time.time() - start_time < remaining:
        try:
            # Try to get the running config - if it works, agent is ready
            data = run_cli(["show", "running-config"], check=False)
            # Make sure we got valid data (not empty due to connection issues)
            if data and any(data.values()):
                return True
        except (RuntimeError, json.JSONDecodeError):
            pass
        time.sleep(2)
    return False
