#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for the 'fboss2-dev config vlan <vlan-id> port <port> taggingMode' command.

This test:
1. Picks an interface from the running system with a valid VLAN
2. Gets the current tagging mode for the port on that VLAN
3. Sets the tagging mode to "tagged"
4. Commits and verifies the change via the running configuration
5. Sets the tagging mode back to "untagged"
6. Commits and verifies the change

Requirements:
- FBOSS agent must be running with a valid configuration
- Must be run on the DUT with appropriate permissions to commit config
"""

import sys
from typing import Optional

from cli_test_lib import (
    commit_config,
    find_first_eth_interface,
    run_cli,
)


def get_vlan_port_tagging_mode(vlan_id: int, port_name: str) -> Optional[bool]:
    """
    Get the current tagging mode (emitTags) for a port on a VLAN.

    Returns:
        True if tagged (emitTags=true), False if untagged (emitTags=false),
        None if the port is not a member of the VLAN.
    """
    import json as json_module

    data = run_cli(["show", "running-config"])

    # The JSON has a host key (e.g., "localhost") containing a JSON string
    for host_data_str in data.values():
        # The value is a JSON string that needs to be parsed
        if isinstance(host_data_str, str):
            host_data = json_module.loads(host_data_str)
        else:
            host_data = host_data_str

        sw_config = host_data.get("sw", {})
        vlan_ports = sw_config.get("vlanPorts", [])
        port_list = sw_config.get("ports", [])

        # Build a map of logicalID -> port name
        logical_to_name = {}
        for port in port_list:
            logical_to_name[port.get("logicalID")] = port.get("name")

        for vp in vlan_ports:
            if vp.get("vlanID") == vlan_id:
                logical_port = vp.get("logicalPort")
                if logical_to_name.get(logical_port) == port_name:
                    return vp.get("emitTags", False)
    return None


def set_tagging_mode(vlan_id: int, port_name: str, mode: str) -> None:
    """Set the tagging mode for a port on a VLAN and commit the change."""
    run_cli(["config", "vlan", str(vlan_id), "port", port_name, "taggingMode", mode])
    commit_config()


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: config vlan <id> port <port> taggingMode")
    print("=" * 60)

    # Step 1: Get an interface to test with
    print("\n[Step 1] Finding an interface to test...")
    interface = find_first_eth_interface()
    if interface.vlan is None:
        print("  ERROR: Interface has no VLAN assigned")
        return 1
    print(f"  Using interface: {interface.name} (VLAN: {interface.vlan})")

    vlan_id = interface.vlan
    port_name = interface.name

    # Step 2: Get current tagging mode
    print(f"\n[Step 2] Getting current tagging mode...")
    original_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if original_mode is None:
        print(f"  WARNING: Could not determine current tagging mode, assuming untagged")
        original_mode = False
    print(f"  Current mode: {'tagged' if original_mode else 'untagged'}")

    # Step 3: Set tagging mode to "tagged"
    print(f"\n[Step 3] Setting tagging mode to 'tagged'...")
    set_tagging_mode(vlan_id, port_name, "tagged")
    print(f"  Tagging mode set to 'tagged'")

    # Step 4: Verify the change
    print("\n[Step 4] Verifying tagging mode is 'tagged'...")
    current_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if current_mode is not True:
        print(f"  ERROR: Expected tagged mode but got: {current_mode}")
        return 1
    print(f"  Verified: Tagging mode is 'tagged'")

    # Step 5: Set tagging mode to "untagged"
    print(f"\n[Step 5] Setting tagging mode to 'untagged'...")
    set_tagging_mode(vlan_id, port_name, "untagged")
    print(f"  Tagging mode set to 'untagged'")

    # Step 6: Verify the change
    print("\n[Step 6] Verifying tagging mode is 'untagged'...")
    current_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if current_mode is not False:
        print(f"  ERROR: Expected untagged mode but got: {current_mode}")
        return 1
    print(f"  Verified: Tagging mode is 'untagged'")

    # Restore original mode if it was different
    if original_mode:
        print(f"\n[Cleanup] Restoring original tagging mode to 'tagged'...")
        set_tagging_mode(vlan_id, port_name, "tagged")
        print(f"  Original mode restored")

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
