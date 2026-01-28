#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for the 'fboss2-dev config interface <name> description <string>' command.

This test:
1. Picks an interface from the running system
2. Gets the current description
3. Sets a new description
4. Verifies the description was set correctly via 'fboss2-dev show interface'
5. Restores the original description

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


def get_interface_description(interface_name: str) -> str:
    """Get the current description for an interface."""
    info = get_interface_info(interface_name)
    return info.description


def set_interface_description(interface_name: str, description: str) -> None:
    """Set the description for an interface and commit the change."""
    run_cli(["config", "interface", interface_name, "description", description])
    commit_config()


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: config interface <name> description <string>")
    print("=" * 60)

    # Step 1: Get an interface to test with
    print("\n[Step 1] Finding an interface to test...")
    interface = find_first_eth_interface()
    print(f"  Using interface: {interface.name} (VLAN: {interface.vlan})")

    # Step 2: Get the current description
    print("\n[Step 2] Getting current description...")
    original_description = get_interface_description(interface.name)
    print(f"  Current description: '{original_description}'")

    # Step 3: Set a new description
    test_description = "CLI_E2E_TEST_DESCRIPTION"
    if original_description == test_description:
        test_description = "CLI_E2E_TEST_DESCRIPTION_ALT"
    print(f"\n[Step 3] Setting description to '{test_description}'...")
    set_interface_description(interface.name, test_description)
    print(f"  Description set to '{test_description}'")

    # Step 4: Verify description via 'show interface'
    print("\n[Step 4] Verifying description via 'show interface'...")
    actual_description = get_interface_description(interface.name)
    if actual_description != test_description:
        print(
            f"  ERROR: Expected description '{test_description}', got '{actual_description}'"
        )
        return 1
    print(f"  Verified: Description is '{actual_description}'")

    # Step 5: Restore original description
    print(f"\n[Step 5] Restoring original description ('{original_description}')...")
    set_interface_description(interface.name, original_description)
    print(f"  Restored description to '{original_description}'")

    # Verify restoration
    restored_description = get_interface_description(interface.name)
    if restored_description != original_description:
        print(
            f"  WARNING: Restoration may have failed. Current: '{restored_description}'"
        )

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
