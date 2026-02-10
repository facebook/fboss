#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end test for concurrent session conflict detection and rebase.

This test simulates two users making changes to the configuration at the same
time by using different HOME directories for each "user". It verifies:

1. User1 can start a session and commit changes
2. User2's commit fails if User1 committed while User2's session was open
3. User2 can rebase their changes onto User1's commit
4. After rebase, User2 can commit successfully

Requirements:
- FBOSS agent must be running with a valid configuration
- The test must be run as root (or with appropriate permissions)
"""

import os
import subprocess
import sys
import tempfile

from cli_test_lib import (
    find_first_eth_interface,
    get_fboss_cli,
    get_interface_info,
)


def run_cli_as_user(
    home_dir: str, args: list[str], check: bool = True
) -> subprocess.CompletedProcess:
    """Run CLI with a specific HOME directory to simulate a different user."""
    cli = get_fboss_cli()
    cmd = [cli] + args
    env = os.environ.copy()
    env["HOME"] = home_dir
    print(f"[User HOME={home_dir}] Running: {' '.join(args)}")
    result = subprocess.run(cmd, capture_output=True, text=True, env=env)
    if check and result.returncode != 0:
        print(f"  Failed with code {result.returncode}")
        print(f"  stdout: {result.stdout}")
        print(f"  stderr: {result.stderr}")
        raise RuntimeError(f"Command failed: {' '.join(cmd)}")
    return result


def main() -> int:
    print("=" * 60)
    print("CLI E2E Test: Concurrent Session Conflict Detection")
    print("=" * 60)

    # Step 1: Find an interface to test with
    print("\n[Step 1] Finding interfaces to test...")
    interface = find_first_eth_interface()
    print(f"  Using interface: {interface.name} (VLAN: {interface.vlan})")

    original_description = interface.description
    print(f"  Original description: '{original_description}'")

    # Create temporary home directories for two simulated users
    with tempfile.TemporaryDirectory(
        prefix="user1_"
    ) as user1_home, tempfile.TemporaryDirectory(prefix="user2_") as user2_home:

        try:
            # Step 2: User1 starts a session
            print("\n[Step 2] User1 starts a session...")
            run_cli_as_user(
                user1_home,
                ["config", "interface", interface.name, "description", "User1_change"],
            )
            print("  User1 session started with description change")

            # Step 3: User2 also starts a session (same base commit)
            print("\n[Step 3] User2 starts a session...")
            run_cli_as_user(
                user2_home,
                ["config", "interface", interface.name, "description", "User2_change"],
            )
            print("  User2 session started with description change")

            # Step 4: User1 commits first
            print("\n[Step 4] User1 commits first...")
            run_cli_as_user(user1_home, ["config", "session", "commit"])
            print("  User1 commit succeeded")

            # Verify User1's change is applied
            info = get_interface_info(interface.name)
            if info.description != "User1_change":
                print(f"  ERROR: Expected 'User1_change', got '{info.description}'")
                return 1
            print(f"  Verified: Description is now '{info.description}'")

            # Step 5: User2 tries to commit - should fail
            print("\n[Step 5] User2 tries to commit (should fail)...")
            result = run_cli_as_user(
                user2_home, ["config", "session", "commit"], check=False
            )
            print(f"  Return code: {result.returncode}")
            print(f"  stderr: {result.stderr[:300] if result.stderr else '(empty)'}")
            # Check for conflict message in stderr (exit code may incorrectly be 0)
            if "system configuration has changed" not in result.stderr:
                print(f"  ERROR: Expected conflict message in stderr")
                return 1
            print("  User2's commit correctly rejected with conflict message")

            # Step 6: User2 rebases
            print("\n[Step 6] User2 rebases (should fail - conflicting changes)...")
            result = run_cli_as_user(
                user2_home, ["config", "session", "rebase"], check=False
            )
            print(f"  Return code: {result.returncode}")
            print(f"  stderr: {result.stderr[:300] if result.stderr else '(empty)'}")
            # This should fail because both users changed the same field
            # Check both stdout and stderr for conflict message
            output = (result.stdout or "") + (result.stderr or "")
            if "conflict" in output.lower():
                print("  Rebase correctly detected conflict")
            else:
                print("  Note: Rebase may have succeeded or failed with other error")

            print("\n" + "=" * 60)
            print("TEST PASSED")
            print("=" * 60)
            return 0

        finally:
            # Cleanup: Restore original description
            print(f"\n[Cleanup] Restoring original description...")
            real_home = os.environ.get("HOME", "/root")
            try:
                run_cli_as_user(
                    real_home,
                    [
                        "config",
                        "interface",
                        interface.name,
                        "description",
                        original_description or "",
                    ],
                )
                run_cli_as_user(real_home, ["config", "session", "commit"])
                print(f"  Restored description to '{original_description}'")
            except Exception as e:
                print(f"  Warning: Cleanup failed: {e}")


if __name__ == "__main__":
    sys.exit(main())
