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

    Note: L2 learning mode changes require an agent restart, so we need to
    wait for the agent to be ready after the commit.
    """
    run_cli(["config", "l2", "learning-mode", mode])
    commit_config()
    # Wait for agent to be ready after restart
    print("  Waiting for agent to be ready after restart...")
    if not wait_for_agent_ready(max_wait_seconds=60):
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

    # Step 2: Set L2 learning mode to "software"
    print("\n[Step 2] Setting L2 learning mode to 'software'...")
    set_l2_learning_mode("software")
    print("  L2 learning mode set to 'software'")

    # Step 3: Verify the change
    print("\n[Step 3] Verifying L2 learning mode is 'software'...")
    current_mode = get_l2_learning_mode()
    if current_mode != L2_LEARNING_MODE_SOFTWARE:
        print(f"  ERROR: Expected software mode (1) but got: {current_mode}")
        return 1
    print("  Verified: L2 learning mode is 'software'")

    # Step 4: Set L2 learning mode to "hardware"
    print("\n[Step 4] Setting L2 learning mode to 'hardware'...")
    set_l2_learning_mode("hardware")
    print("  L2 learning mode set to 'hardware'")

    # Step 5: Verify the change
    print("\n[Step 5] Verifying L2 learning mode is 'hardware'...")
    current_mode = get_l2_learning_mode()
    if current_mode != L2_LEARNING_MODE_HARDWARE:
        print(f"  ERROR: Expected hardware mode (0) but got: {current_mode}")
        return 1
    print("  Verified: L2 learning mode is 'hardware'")

    # Restore original mode if it was different
    if original_mode != L2_LEARNING_MODE_HARDWARE:
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
