#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for the 'fboss2-dev config vlan <vlan-id> static-mac add/delete' commands.

This test:
1. Picks an interface from the running system with a valid VLAN
2. Adds a static MAC entry for that VLAN
3. Verifies the MAC entry was added via 'fboss2-dev show mac details'
4. Deletes the static MAC entry
5. Verifies the MAC entry was deleted

Requirements:
- FBOSS agent must be running with a valid configuration
- The test must be run as root (or with appropriate permissions)
"""

import sys
from typing import Optional

from cli_test_lib import (
    commit_config,
    find_first_eth_interface,
    run_cli,
)

# Test MAC address - using a locally administered unicast MAC
# (second hex digit is 2, 6, A, or E for locally administered)
TEST_MAC_ADDRESS = "02:00:00:E2:E2:01"


def get_mac_entries() -> list[dict]:
    """Get all MAC entries from 'fboss2-dev show mac details'."""
    data = run_cli(["show", "mac", "details"])

    # The JSON has a host key (e.g., "127.0.0.1") containing the L2 entries
    entries: list[dict] = []
    for host_data in data.values():
        for entry in host_data.get("l2Entries", []):
            entries.append(entry)
    return entries


def find_mac_entry(mac_address: str, vlan_id: int) -> Optional[dict]:
    """Find a MAC entry by MAC address and VLAN ID.

    Since reloadConfig is synchronous, no retry logic is needed.
    """
    normalized_mac = mac_address.upper()
    entries = get_mac_entries()
    for entry in entries:
        entry_mac = entry.get("mac", "").upper()
        entry_vlan = entry.get("vlanID")
        if entry_mac == normalized_mac and entry_vlan == vlan_id:
            return entry
    return None


def add_static_mac(vlan_id: int, mac_address: str, port_name: str) -> None:
    """Add a static MAC entry and commit the change."""
    run_cli(
        ["config", "vlan", str(vlan_id), "static-mac", "add", mac_address, port_name]
    )
    commit_config()


def delete_static_mac(vlan_id: int, mac_address: str) -> None:
    """Delete a static MAC entry and commit the change."""
    run_cli(["config", "vlan", str(vlan_id), "static-mac", "delete", mac_address])
    commit_config()


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: config vlan <id> static-mac add/delete")
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

    # Check if the test MAC already exists (cleanup from a previous failed run)
    existing_entry = find_mac_entry(TEST_MAC_ADDRESS, vlan_id)
    if existing_entry is not None:
        print(f"\n[Cleanup] Removing existing test MAC entry...")
        try:
            delete_static_mac(vlan_id, TEST_MAC_ADDRESS)
            print(f"  Removed existing entry for {TEST_MAC_ADDRESS}")
        except Exception as e:
            print(f"  WARNING: Could not remove existing entry: {e}")

    # Step 2: Add a static MAC entry
    print(f"\n[Step 2] Adding static MAC entry...")
    print(f"  VLAN: {vlan_id}, MAC: {TEST_MAC_ADDRESS}, Port: {port_name}")
    add_static_mac(vlan_id, TEST_MAC_ADDRESS, port_name)
    print(f"  Static MAC entry added")

    # Step 3: Verify the MAC entry via 'show mac details'
    print("\n[Step 3] Verifying MAC entry via 'show mac details'...")
    entry = find_mac_entry(TEST_MAC_ADDRESS, vlan_id)
    if entry is None:
        print(f"  ERROR: MAC entry not found for {TEST_MAC_ADDRESS} on VLAN {vlan_id}")
        return 1
    print(
        f"  Verified: MAC entry found - MAC: {entry.get('mac')}, "
        f"VLAN: {entry.get('vlanID')}, Port: {entry.get('ifName')}"
    )

    # Step 4: Delete the static MAC entry
    print(f"\n[Step 4] Deleting static MAC entry...")
    delete_static_mac(vlan_id, TEST_MAC_ADDRESS)
    print(f"  Static MAC entry deleted")

    # Step 5: Verify the MAC entry was deleted
    print("\n[Step 5] Verifying MAC entry was deleted...")
    entry = find_mac_entry(TEST_MAC_ADDRESS, vlan_id)
    if entry is not None:
        print(
            f"  ERROR: MAC entry still exists for {TEST_MAC_ADDRESS} on VLAN {vlan_id}"
        )
        return 1
    print(f"  Verified: MAC entry no longer exists")

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
