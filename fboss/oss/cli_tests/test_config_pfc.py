#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end tests for PFC (Priority Flow Control) CLI commands.

This test covers:
1. Priority group policy configuration (config qos priority-group-policy)

This test:
1. Cleans up any existing test config (portPgConfigs and bufferPoolConfigs)
2. Creates a buffer pool (required for priority group config)
3. Creates a new priority group policy with multiple group IDs
4. Commits the configuration and verifies it was applied
5. Cleans up the test config
"""

import json
import os
import shutil
import sys

from cli_test_lib import commit_config, run_cli


# Paths
SYSTEM_CONFIG_PATH = "/etc/coop/agent.conf"
SESSION_CONFIG_PATH = os.path.expanduser("~/.fboss2/agent.conf")

# Test names
TEST_BUFFER_POOL_NAME = "cli_e2e_test_buffer_pool"
TEST_POLICY_NAME = "cli_e2e_test_pg_policy"

# Buffer pool configuration
TEST_BUFFER_POOL_CONFIG = {
    "sharedBytes": 78773528,
    "headroomBytes": 4405376,
}

# scalingFactor enum values: ONE_HALF=8, TWO=9
SCALING_FACTOR_MAP = {"ONE_HALF": 8, "TWO": 9}

# Expected portPgConfigs after test (what we expect in the JSON file)
EXPECTED_PORT_PG_CONFIGS = {
    TEST_POLICY_NAME: [
        {
            "id": 2,
            "name": "pg2",
            "sramScalingFactor": SCALING_FACTOR_MAP["ONE_HALF"],
            "minLimitBytes": 14478,
            "headroomLimitBytes": 726440,
            "resumeOffsetBytes": 9652,
            "bufferPoolName": TEST_BUFFER_POOL_NAME,
        },
        {
            "id": 6,
            "name": "pg6",
            "sramScalingFactor": SCALING_FACTOR_MAP["ONE_HALF"],
            "minLimitBytes": 4826,
            "headroomLimitBytes": 726440,
            "resumeOffsetBytes": 9652,
            "bufferPoolName": TEST_BUFFER_POOL_NAME,
        },
        {
            "id": 0,
            "name": "pg0",
            "sramScalingFactor": SCALING_FACTOR_MAP["TWO"],
            "minLimitBytes": 4826,
            "headroomLimitBytes": 0,
            "resumeOffsetBytes": 9652,
            "bufferPoolName": TEST_BUFFER_POOL_NAME,
        },
        {
            "id": 7,
            "name": "pg7",
            "sramScalingFactor": SCALING_FACTOR_MAP["TWO"],
            "minLimitBytes": 4826,
            "headroomLimitBytes": 0,
            "resumeOffsetBytes": 9652,
            "bufferPoolName": TEST_BUFFER_POOL_NAME,
        },
    ]
}

# CLI input format (uses string scaling factor names)
CLI_PG_CONFIGS = [
    {
        "id": 2,
        "scalingFactor": "ONE_HALF",
        "minLimitBytes": 14478,
        "headroomLimitBytes": 726440,
        "resumeOffsetBytes": 9652,
    },
    {
        "id": 6,
        "scalingFactor": "ONE_HALF",
        "minLimitBytes": 4826,
        "headroomLimitBytes": 726440,
        "resumeOffsetBytes": 9652,
    },
    {
        "id": 0,
        "scalingFactor": "TWO",
        "minLimitBytes": 4826,
        "headroomLimitBytes": 0,
        "resumeOffsetBytes": 9652,
    },
    {
        "id": 7,
        "scalingFactor": "TWO",
        "minLimitBytes": 4826,
        "headroomLimitBytes": 0,
        "resumeOffsetBytes": 9652,
    },
]


def configure_buffer_pool(pool_name: str, config: dict) -> None:
    """Configure a buffer pool with shared and headroom bytes."""
    base_cmd = ["config", "qos", "buffer-pool", pool_name]
    run_cli(base_cmd + ["shared-bytes", str(config["sharedBytes"])])
    run_cli(base_cmd + ["headroom-bytes", str(config["headroomBytes"])])


def configure_priority_group(
    policy_name: str, group_id: int, config: dict, buffer_pool_name: str
) -> None:
    """Configure a single priority group with all its attributes (one at a time)."""
    base_cmd = [
        "config",
        "qos",
        "priority-group-policy",
        policy_name,
        "group-id",
        str(group_id),
    ]

    # Set each attribute
    run_cli(base_cmd + ["min-limit-bytes", str(config["minLimitBytes"])])
    run_cli(base_cmd + ["headroom-limit-bytes", str(config["headroomLimitBytes"])])
    run_cli(base_cmd + ["resume-offset-bytes", str(config["resumeOffsetBytes"])])
    run_cli(base_cmd + ["scaling-factor", config["scalingFactor"]])
    run_cli(base_cmd + ["buffer-pool-name", buffer_pool_name])


def configure_priority_group_multi_attr(
    policy_name: str, group_id: int, config: dict, buffer_pool_name: str
) -> None:
    """Configure a single priority group with all attributes in one command."""
    cmd = [
        "config",
        "qos",
        "priority-group-policy",
        policy_name,
        "group-id",
        str(group_id),
        "min-limit-bytes",
        str(config["minLimitBytes"]),
        "headroom-limit-bytes",
        str(config["headroomLimitBytes"]),
        "resume-offset-bytes",
        str(config["resumeOffsetBytes"]),
        "scaling-factor",
        config["scalingFactor"],
        "buffer-pool-name",
        buffer_pool_name,
    ]
    run_cli(cmd)


def cleanup_test_config() -> None:
    """Remove portPgConfigs and bufferPoolConfigs from the config."""
    session_dir = os.path.dirname(SESSION_CONFIG_PATH)
    metadata_path = os.path.join(session_dir, "cli_metadata.json")

    print("  Copying system config to session config...")
    os.makedirs(session_dir, exist_ok=True)
    shutil.copy(SYSTEM_CONFIG_PATH, SESSION_CONFIG_PATH)

    print("  Removing PFC-related configs...")
    with open(SESSION_CONFIG_PATH, "r") as f:
        config = json.load(f)

    sw_config = config.get("sw", {})

    # Remove global PFC configs
    sw_config.pop("portPgConfigs", None)
    sw_config.pop("bufferPoolConfigs", None)

    with open(SESSION_CONFIG_PATH, "w") as f:
        json.dump(config, f, indent=2)

    # Update metadata to require AGENT_RESTART since we're changing PFC config
    # Use symbolic names matching thrift PORTABLE format
    print("  Updating metadata for AGENT_RESTART...")
    metadata = {
        "action": {"WEDGE_AGENT": "AGENT_RESTART"},
        "commands": [],
        "base": "",
    }
    with open(metadata_path, "w") as f:
        json.dump(metadata, f, indent=2)

    print("  Committing cleanup...")
    commit_config()


def main() -> int:
    print("=" * 70)
    print("CLI E2E Test: PFC Configuration")
    print("=" * 70)

    # Step 0: Cleanup any existing test config
    print("\n[Step 0] Cleaning up any existing test config...")
    cleanup_test_config()
    print("  Cleanup complete")

    # Step 1: Configure buffer pool (required for priority group config)
    print(f"\n[Step 1] Configuring buffer pool '{TEST_BUFFER_POOL_NAME}'...")
    configure_buffer_pool(TEST_BUFFER_POOL_NAME, TEST_BUFFER_POOL_CONFIG)
    print("  Buffer pool configured")

    # Step 2: Configure priority groups (using single-attribute commands)
    print(f"\n[Step 2] Configuring priority group policy '{TEST_POLICY_NAME}'...")
    print("  Using single-attribute commands for group-ids 2 and 6...")
    for pg_config in CLI_PG_CONFIGS[:2]:  # First two: group-ids 2 and 6
        group_id = pg_config["id"]
        print(f"  Configuring group-id {group_id} (one attribute at a time)...")
        configure_priority_group(
            TEST_POLICY_NAME, group_id, pg_config, TEST_BUFFER_POOL_NAME
        )
    print("  Using multi-attribute commands for group-ids 0 and 7...")
    for pg_config in CLI_PG_CONFIGS[2:]:  # Last two: group-ids 0 and 7
        group_id = pg_config["id"]
        print(f"  Configuring group-id {group_id} (all attributes at once)...")
        configure_priority_group_multi_attr(
            TEST_POLICY_NAME, group_id, pg_config, TEST_BUFFER_POOL_NAME
        )
    print("  All priority groups configured")

    # Step 3: Commit the configuration
    print("\n[Step 3] Committing configuration...")
    commit_config()
    print("  Configuration committed successfully")

    # Step 4: Verify configuration by reading /etc/coop/agent.conf
    print("\n[Step 4] Verifying configuration...")
    with open(SYSTEM_CONFIG_PATH, "r") as f:
        config = json.load(f)

    sw_config = config.get("sw", {})

    # Verify buffer pool
    actual_buffer_pool = sw_config.get("bufferPoolConfigs", {}).get(
        TEST_BUFFER_POOL_NAME
    )
    if actual_buffer_pool != TEST_BUFFER_POOL_CONFIG:
        print("  ERROR: Buffer pool mismatch")
        print(f"  Expected: {TEST_BUFFER_POOL_CONFIG}")
        print(f"  Actual:   {actual_buffer_pool}")
        return 1
    print(f"  Buffer pool '{TEST_BUFFER_POOL_NAME}' verified")

    # Verify priority group policy using deep equal
    actual_pg_configs = sw_config.get("portPgConfigs", {})
    if TEST_POLICY_NAME not in actual_pg_configs:
        print(f"  ERROR: Priority group policy '{TEST_POLICY_NAME}' not found")
        return 1

    # Sort both lists by id for comparison
    expected_list = sorted(
        EXPECTED_PORT_PG_CONFIGS[TEST_POLICY_NAME], key=lambda x: x["id"]
    )
    actual_list = sorted(actual_pg_configs[TEST_POLICY_NAME], key=lambda x: x["id"])

    if actual_list != expected_list:
        print("  ERROR: Priority group configs mismatch")
        print(f"  Expected: {json.dumps(expected_list, indent=2)}")
        print(f"  Actual:   {json.dumps(actual_list, indent=2)}")
        return 1
    print(f"  Priority group policy '{TEST_POLICY_NAME}' verified")

    print("\n" + "=" * 70)
    print("TEST PASSED")
    print("=" * 70)

    # Step 5: Cleanup test config
    print("\n[Step 5] Cleaning up test config...")
    cleanup_test_config()
    print("  Cleanup complete")

    return 0


if __name__ == "__main__":
    sys.exit(main())
