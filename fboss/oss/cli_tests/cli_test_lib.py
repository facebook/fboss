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


def run_cli(args: list[str], check: bool = True, quiet: bool = False) -> dict[str, Any]:
    """Run the fboss2-dev CLI with the given arguments.

    The --fmt json flag is automatically prepended to all commands.
    Returns the parsed JSON output as a dict.

    Args:
        args: CLI arguments to pass after 'fboss2-dev --fmt json'
        check: If True, raise RuntimeError on non-zero return code
        quiet: If True, suppress logging of command execution
    """
    cli = get_fboss_cli()
    cmd = [cli, "--fmt", "json"] + args
    if not quiet:
        print(f"[CLI] Running: {' '.join(args)}")
    start_time = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.time() - start_time
    if not quiet:
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


def wait_for_agent_ready(
    timeout_seconds: float = 60.0,
    poll_interval_ms: int = 500,
) -> None:
    """
    Wait for the FBOSS agent to be ready by polling 'config applied-info'.

    Args:
        timeout_seconds: Maximum time to wait in seconds (default: 60)
        poll_interval_ms: How often to poll in milliseconds (default: 500)

    Raises:
        RuntimeError: If the agent is not ready within the timeout
    """
    start_time = time.time()
    poll_interval_s = poll_interval_ms / 1000.0

    while True:
        elapsed = time.time() - start_time
        if elapsed >= timeout_seconds:
            raise RuntimeError(f"Agent not ready after {timeout_seconds}s")

        try:
            # Try to run 'config applied-info' - a simple command to check agent is ready
            data = run_cli(["config", "applied-info"], check=False, quiet=True)
            if data:
                print(f"[CLI] Agent is ready (waited {elapsed:.1f}s)")
                # Log the applied-info output for debugging
                print(f"[CLI] Applied info: {json.dumps(data)}")
                return
        except Exception:
            pass

        # Agent not ready yet, wait and retry
        time.sleep(poll_interval_s)


def commit_config() -> None:
    """Commit the current configuration session and wait for agent to be ready."""
    run_cli(["config", "session", "commit"])
    # After commit, the agent may restart (warmboot/coldboot).
    # Wait for it to be ready before returning.
    wait_for_agent_ready()
