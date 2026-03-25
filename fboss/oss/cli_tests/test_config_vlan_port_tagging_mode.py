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
    running_config,
)


def get_vlan_port_tagging_mode(vlan_id: int, port_name: str) -> Optional[str]:
    """
    Get the current tagging mode for a port on a VLAN.

    Returns:
        "tagged" if emitTags=true and emitPriorityTags=false,
        "untagged" if both are false,
        "priority-tagged" if emitPriorityTags=true and emitTags=false,
        None if the port is not a member of the VLAN.
    """
    data = running_config()

    # running_config() already unwraps the host-level structure
    sw_config = data.get("sw", {})
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
                emit_tags = vp.get("emitTags", False)
                emit_priority_tags = vp.get("emitPriorityTags", False)
                if emit_priority_tags:
                    return "priority-tagged"
                if emit_tags:
                    return "tagged"
                return "untagged"
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
    print("\n[Step 2] Getting current tagging mode...")
    original_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if original_mode is None:
        print("  WARNING: Could not determine current tagging mode, assuming untagged")
        original_mode = "untagged"
    print(f"  Current mode: {original_mode}")

    # Step 3: Set tagging mode to "tagged"
    print("\n[Step 3] Setting tagging mode to 'tagged'...")
    set_tagging_mode(vlan_id, port_name, "tagged")
    print("  Tagging mode set to 'tagged'")

    # Step 4: Verify the change
    print("\n[Step 4] Verifying tagging mode is 'tagged'...")
    current_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if current_mode != "tagged":
        print(f"  ERROR: Expected 'tagged' mode but got: {current_mode}")
        return 1
    print("  Verified: Tagging mode is 'tagged'")

    # Step 5: Set tagging mode to "priority-tagged"
    print("\n[Step 5] Setting tagging mode to 'priority-tagged'...")
    set_tagging_mode(vlan_id, port_name, "priority-tagged")
    print("  Tagging mode set to 'priority-tagged'")

    # Step 6: Verify the change
    print("\n[Step 6] Verifying tagging mode is 'priority-tagged'...")
    current_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if current_mode != "priority-tagged":
        print(f"  ERROR: Expected 'priority-tagged' mode but got: {current_mode}")
        return 1
    print("  Verified: Tagging mode is 'priority-tagged'")

    # Step 7: Set tagging mode to "untagged"
    print("\n[Step 7] Setting tagging mode to 'untagged'...")
    set_tagging_mode(vlan_id, port_name, "untagged")
    print("  Tagging mode set to 'untagged'")

    # Step 8: Verify the change
    print("\n[Step 8] Verifying tagging mode is 'untagged'...")
    current_mode = get_vlan_port_tagging_mode(vlan_id, port_name)
    if current_mode != "untagged":
        print(f"  ERROR: Expected 'untagged' mode but got: {current_mode}")
        return 1
    print("  Verified: Tagging mode is 'untagged'")

    # Restore original mode if it was different
    if original_mode != "untagged":
        print(f"\n[Cleanup] Restoring original tagging mode to '{original_mode}'...")
        set_tagging_mode(vlan_id, port_name, original_mode)
        print("  Original mode restored")

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
