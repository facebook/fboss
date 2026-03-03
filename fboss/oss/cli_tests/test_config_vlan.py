#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for the 'fboss2-dev config interface <name> switchport access vlan <N>' command.

This test:
1. Picks an interface from the running system
2. Gets the current access VLAN
3. Sets a new access VLAN
4. Verifies the VLAN was set correctly via 'fboss2-dev show interface'
5. Restores the original VLAN

Requirements:
- FBOSS agent must be running with a valid configuration
- The test must be run as root (or with appropriate permissions)
"""

import sys

from cli_test_lib import (
    commit_config,
    find_first_eth_interface,
    get_interface_info,
    run_cli,
)


def get_interface_vlan(interface_name: str) -> int:
    """Get the current access VLAN for an interface."""
    info = get_interface_info(interface_name)
    if info.vlan is None:
        raise RuntimeError(f"Could not find VLAN for interface {interface_name}")
    return info.vlan


def set_interface_vlan(interface_name: str, vlan_id: int) -> None:
    """Set the access VLAN for an interface and commit the change."""
    run_cli(
        [
            "config",
            "interface",
            interface_name,
            "switchport",
            "access",
            "vlan",
            str(vlan_id),
        ]
    )
    commit_config()


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: config interface <name> switchport access vlan <N>")
    print("=" * 60)

    # Step 1: Get an interface to test with
    print("\n[Step 1] Finding an interface to test...")
    interface = find_first_eth_interface()
    print(f"  Using interface: {interface.name} (VLAN: {interface.vlan})")

    # Step 2: Get the current VLAN
    print("\n[Step 2] Getting current access VLAN...")
    original_vlan = get_interface_vlan(interface.name)
    print(f"  Current access VLAN: {original_vlan}")

    # Step 3: Set a new VLAN (toggle between current and current+1)
    new_vlan = original_vlan + 1
    print(f"\n[Step 3] Setting access VLAN to {new_vlan}...")
    set_interface_vlan(interface.name, new_vlan)
    print(f"  Access VLAN set to {new_vlan}")

    # Step 4: Verify VLAN via 'show interface'
    print("\n[Step 4] Verifying access VLAN via 'show interface'...")
    actual_vlan = get_interface_vlan(interface.name)
    if actual_vlan != new_vlan:
        print(f"  ERROR: Expected VLAN {new_vlan}, got {actual_vlan}")
        return 1
    print(f"  Verified: Access VLAN is {actual_vlan}")

    # Step 5: Restore original VLAN
    print(f"\n[Step 5] Restoring original access VLAN ({original_vlan})...")
    set_interface_vlan(interface.name, original_vlan)
    print(f"  Restored access VLAN to {original_vlan}")

    # Verify restoration
    restored_vlan = get_interface_vlan(interface.name)
    if restored_vlan != original_vlan:
        print(f"  WARNING: Restoration may have failed. Current: {restored_vlan}")

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
