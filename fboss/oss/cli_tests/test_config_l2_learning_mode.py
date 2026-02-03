#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for the 'fboss2-dev config l2 learning-mode <mode>' command.

This test:
1. Gets the current L2 learning mode from the running configuration
2. Sets the L2 learning mode to "software"
3. Commits and verifies the change via the running configuration
4. Sets the L2 learning mode to "hardware"
5. Commits and verifies the change
6. Restores the original mode if different

Requirements:
- FBOSS agent must be running with a valid configuration
- Must be run on the DUT with appropriate permissions to commit config
"""

import sys
from typing import Optional

from cli_test_lib import (
    commit_config,
    run_cli,
    running_config,
    wait_for_agent_ready,
)

# L2LearningMode enum values from switch_config.thrift
L2_LEARNING_MODE_HARDWARE = 0
L2_LEARNING_MODE_SOFTWARE = 1
L2_LEARNING_MODE_DISABLED = 2

MODE_INT_TO_STR = {
    L2_LEARNING_MODE_HARDWARE: "hardware",
    L2_LEARNING_MODE_SOFTWARE: "software",
    L2_LEARNING_MODE_DISABLED: "disabled",
}

MODE_STR_TO_INT = {v: k for k, v in MODE_INT_TO_STR.items()}


def get_l2_learning_mode() -> Optional[int]:
    """
    Get the current L2 learning mode from the running configuration.

    Returns:
        The L2 learning mode as an integer (0=HARDWARE, 1=SOFTWARE, 2=DISABLED),
        or None if not found.
    """
    config = running_config()
    sw_config = config.get("sw", {})
    switch_settings = sw_config.get("switchSettings", {})
    return switch_settings.get("l2LearningMode", L2_LEARNING_MODE_HARDWARE)


def set_l2_learning_mode(mode: str) -> None:
    """Set the L2 learning mode and commit the change.

    Note: L2 learning mode changes require an agent restart (coldboot).
    The commit will trigger the restart automatically.
    """
    run_cli(["config", "l2", "learning-mode", mode])
    commit_config()
    # Wait for agent to be ready after restart
    print("  Waiting for agent to be ready after restart...")
    if not wait_for_agent_ready(timeout_seconds=60):
        raise RuntimeError("Agent did not become ready after restart")


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: config l2 learning-mode <mode>")
    print("=" * 60)

    # Step 1: Get current L2 learning mode
    print("\n[Step 1] Getting current L2 learning mode...")
    original_mode = get_l2_learning_mode()
    if original_mode is None:
        print(
            "  WARNING: Could not determine current L2 learning mode, assuming hardware"
        )
        original_mode = L2_LEARNING_MODE_HARDWARE
    print(
        f"  Current mode: {MODE_INT_TO_STR.get(original_mode, f'unknown({original_mode})')}"
    )

    # Step 2: Set L2 learning mode to a different mode (to trigger a change)
    # If current is hardware, set to software; otherwise set to hardware
    first_test_mode = (
        "software" if original_mode == L2_LEARNING_MODE_HARDWARE else "hardware"
    )
    first_test_mode_int = MODE_STR_TO_INT[first_test_mode]

    print(f"\n[Step 2] Setting L2 learning mode to '{first_test_mode}'...")
    set_l2_learning_mode(first_test_mode)
    print(f"  L2 learning mode set to '{first_test_mode}'")

    # Step 3: Verify the change
    print(f"\n[Step 3] Verifying L2 learning mode is '{first_test_mode}'...")
    current_mode = get_l2_learning_mode()
    if current_mode != first_test_mode_int:
        print(
            f"  ERROR: Expected {first_test_mode} mode ({first_test_mode_int}) but got: {current_mode}"
        )
        return 1
    print(f"  Verified: L2 learning mode is '{first_test_mode}'")

    # Step 4: Set L2 learning mode back to the opposite mode
    second_test_mode = "hardware" if first_test_mode == "software" else "software"
    second_test_mode_int = MODE_STR_TO_INT[second_test_mode]

    print(f"\n[Step 4] Setting L2 learning mode to '{second_test_mode}'...")
    set_l2_learning_mode(second_test_mode)
    print(f"  L2 learning mode set to '{second_test_mode}'")

    # Step 5: Verify the change
    print(f"\n[Step 5] Verifying L2 learning mode is '{second_test_mode}'...")
    current_mode = get_l2_learning_mode()
    if current_mode != second_test_mode_int:
        print(
            f"  ERROR: Expected {second_test_mode} mode ({second_test_mode_int}) but got: {current_mode}"
        )
        return 1
    print(f"  Verified: L2 learning mode is '{second_test_mode}'")

    # Restore original mode if it was different from the final test mode
    if original_mode != second_test_mode_int:
        original_mode_str = MODE_INT_TO_STR.get(original_mode, "hardware")
        print(
            f"\n[Cleanup] Restoring original L2 learning mode to '{original_mode_str}'..."
        )
        set_l2_learning_mode(original_mode_str)
        print("  Original mode restored")

    print("\n" + "=" * 60)
    print("TEST PASSED")
    print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
