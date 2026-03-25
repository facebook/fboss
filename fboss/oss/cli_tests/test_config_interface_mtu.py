#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for the 'fboss2-dev config interface <name> mtu <N>' command.

This test:
1. Picks an interface from the running system
2. Gets the current MTU value
3. Sets a new MTU value
4. Verifies the MTU was set correctly via 'fboss2-dev show interface'
5. Verifies the MTU on the kernel interface via 'ip link'
6. Restores the original MTU

Requirements:
- FBOSS agent must be running with a valid configuration
- The test must be run as root (or with appropriate permissions)
"""

import json
import sys

from cli_test_lib import (
    commit_config,
    find_first_eth_interface,
    get_interface_info,
    run_cli,
    run_cmd,
)


def get_interface_mtu(interface_name: str) -> int:
    """Get the current MTU for an interface."""
    info = get_interface_info(interface_name)
    if info.mtu is None:
        raise RuntimeError(f"Could not find MTU for interface {interface_name}")
    return info.mtu


def get_kernel_interface_mtu(vlan_id: int) -> int:
    """Get the MTU of the kernel interface (fboss<vlan_id>) using 'ip -json link'."""
    kernel_intf = f"fboss{vlan_id}"
    result = run_cmd(["ip", "-json", "link", "show", kernel_intf], check=False)

    if result.returncode != 0:
        print(f"Warning: Kernel interface {kernel_intf} not found")
        return -1

    data = json.loads(result.stdout)
    if not data:
        raise RuntimeError(f"No data returned for kernel interface {kernel_intf}")

    mtu = data[0]["mtu"]
    assert mtu, "MTU can't be zero"
    return mtu


def set_interface_mtu(interface_name: str, mtu: int) -> None:
    """Set the MTU for an interface and commit the change."""
    run_cli(["config", "interface", interface_name, "mtu", str(mtu)])
    commit_config()


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: config interface <name> mtu <N>")
    print("=" * 60)

    # Step 1: Get an interface to test with
    print("\n[Step 1] Finding an interface to test...")
    interface = find_first_eth_interface()
    print(f"  Using interface: {interface.name} (VLAN: {interface.vlan})")

    # Step 2: Get the current MTU
    print("\n[Step 2] Getting current MTU...")
    original_mtu = get_interface_mtu(interface.name)
    print(f"  Current MTU: {original_mtu}")

    # Step 3: Set a new MTU (toggle between 1500 and 9000)
    new_mtu = 9000 if original_mtu != 9000 else 1500
    print(f"\n[Step 3] Setting MTU to {new_mtu}...")
    set_interface_mtu(interface.name, new_mtu)
    print(f"  MTU set to {new_mtu}")

    # Step 4: Verify MTU via 'show interface'
    print("\n[Step 4] Verifying MTU via 'show interface'...")
    actual_mtu = get_interface_mtu(interface.name)
    if actual_mtu != new_mtu:
        print(f"  ERROR: Expected MTU {new_mtu}, got {actual_mtu}")
        return 1
    print(f"  Verified: MTU is {actual_mtu}")

    # Step 5: Verify kernel interface MTU
    print("\n[Step 5] Verifying kernel interface MTU...")
    assert interface.vlan is not None  # Guaranteed by find_first_eth_interface
    kernel_mtu = get_kernel_interface_mtu(interface.vlan)
    if kernel_mtu > 0:
        if kernel_mtu != new_mtu:
            print(f"  ERROR: Kernel MTU is {kernel_mtu}, expected {new_mtu}")
            return 1
        print(
            f"  Verified: Kernel interface fboss{interface.vlan} has MTU {kernel_mtu}"
        )
    else:
        print(f"  Skipped: Kernel interface fboss{interface.vlan} not found")

    # Step 6: Restore original MTU
    print(f"\n[Step 6] Restoring original MTU ({original_mtu})...")
    set_interface_mtu(interface.name, original_mtu)
    print(f"  Restored MTU to {original_mtu}")

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
