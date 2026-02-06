#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end tests for Port Queue Config CLI commands.

This test covers:
1. Queuing policy configuration (config qos queuing-policy)

This test:
1. Cleans up any existing test config (portQueueConfigs)
2. Creates a new queuing policy with multiple queue IDs
3. Configures various queue attributes (scheduling, weight, sharedBytes, etc.)
4. Commits the configuration and verifies it was applied
5. Cleans up the test config

Based on sample config from l3_scaleup.conf lines 5825-5942.
"""

import json
import sys
from typing import Any

from cli_test_lib import (
    SYSTEM_CONFIG_PATH,
    cleanup_config,
    commit_config,
    find_first_eth_interface,
    run_cli,
)

# Test names
TEST_POLICY_NAME = "cli_e2e_test_queue_policy"
TEST_BUFFER_POOL_NAME = "cli_e2e_test_buffer_pool"

# QueueScheduling enum values: WEIGHTED_ROUND_ROBIN=0, STRICT_PRIORITY=1, DEFICIT_ROUND_ROBIN=2
SCHEDULING_MAP = {"WRR": 0, "SP": 1, "DRR": 2}

# MMUScalingFactor enum values
SCALING_FACTOR_MAP = {"ONE_HALF": 8, "TWO": 9, "FOUR": 10}

# StreamType enum values: UNICAST=0, MULTICAST=1, ALL=2, FABRIC_TX=3
STREAM_TYPE_MAP = {"UNICAST": 0, "MULTICAST": 1, "ALL": 2, "FABRIC_TX": 3}

# QueueCongestionBehavior enum values: EARLY_DROP=0, ECN=1
CONGESTION_BEHAVIOR_MAP = {"EARLY_DROP": 0, "ECN": 1}

# Buffer pool configuration (needed for buffer-pool-name reference)
TEST_BUFFER_POOL_CONFIG = {
    "sharedBytes": 78773528,
    "headroomBytes": 4405376,
}

# CLI input format for queue configs (uses string values)
# Based on l3_scaleup.conf structure but using attributes we support
CLI_QUEUE_CONFIGS = [
    {
        "id": 2,
        "scheduling": "SP",
        "sharedBytes": 83618832,
        "weight": 10,
    },
    {
        "id": 6,
        "scheduling": "SP",
        "sharedBytes": 83618832,
        "scalingFactor": "TWO",
    },
    {
        "id": 7,
        "scheduling": "WRR",
        "weight": 20,
        "reservedBytes": 5000,
        "bufferPoolName": TEST_BUFFER_POOL_NAME,
    },
    {
        "id": 3,
        "scheduling": "WRR",
        "weight": 15,
        "streamType": "MULTICAST",
    },
    {
        "id": 4,
        "scheduling": "SP",
        "sharedBytes": 83618832,
        "aqm": {
            "behavior": "ECN",
            "detection": {
                "linear": {
                    "minimumLength": 120000,
                    "maximumLength": 120000,
                    "probability": 100,
                }
            },
        },
    },
]

# Expected portQueueConfigs after test (what we expect in the JSON file)
# Note: streamType defaults to 0 (UNICAST)
EXPECTED_PORT_QUEUE_CONFIGS = {
    TEST_POLICY_NAME: [
        {
            "id": 2,
            "streamType": 0,
            "scheduling": SCHEDULING_MAP["SP"],
            "sharedBytes": 83618832,
            "weight": 10,
        },
        {
            "id": 6,
            "streamType": 0,
            "scheduling": SCHEDULING_MAP["SP"],
            "sharedBytes": 83618832,
            "scalingFactor": SCALING_FACTOR_MAP["TWO"],
        },
        {
            "id": 7,
            "streamType": 0,
            "scheduling": SCHEDULING_MAP["WRR"],
            "weight": 20,
            "reservedBytes": 5000,
            "bufferPoolName": TEST_BUFFER_POOL_NAME,
        },
        {
            "id": 3,
            "streamType": STREAM_TYPE_MAP["MULTICAST"],
            "scheduling": SCHEDULING_MAP["WRR"],
            "weight": 15,
        },
        {
            "id": 4,
            "streamType": 0,
            "scheduling": SCHEDULING_MAP["SP"],
            "sharedBytes": 83618832,
            "aqms": [
                {
                    "behavior": CONGESTION_BEHAVIOR_MAP["ECN"],
                    "detection": {
                        "linear": {
                            "minimumLength": 120000,
                            "maximumLength": 120000,
                            "probability": 100,
                        }
                    },
                }
            ],
        },
    ]
}


def configure_buffer_pool(pool_name: str, config: dict) -> None:
    """Configure a buffer pool with shared and headroom bytes."""
    base_cmd = ["config", "qos", "buffer-pool", pool_name]
    run_cli(base_cmd + ["shared-bytes", str(config["sharedBytes"])])
    run_cli(base_cmd + ["headroom-bytes", str(config["headroomBytes"])])


def configure_queue(policy_name: str, queue_id: int, config: dict) -> None:
    """Configure a single queue with its attributes.

    Uses the new key-value pair syntax:
        config qos queuing-policy <name> queue-id <id> <attr> <value> [<attr> <value> ...]
    """
    # Build command with queue-id followed by key-value pairs
    cmd = [
        "config",
        "qos",
        "queuing-policy",
        policy_name,
        "queue-id",
        str(queue_id),
    ]

    # Add each attribute as key-value pairs
    if "scheduling" in config:
        cmd.extend(["scheduling", config["scheduling"]])
    if "weight" in config:
        cmd.extend(["weight", str(config["weight"])])
    if "sharedBytes" in config:
        cmd.extend(["shared-bytes", str(config["sharedBytes"])])
    if "reservedBytes" in config:
        cmd.extend(["reserved-bytes", str(config["reservedBytes"])])
    if "scalingFactor" in config:
        cmd.extend(["scaling-factor", config["scalingFactor"]])
    if "streamType" in config:
        cmd.extend(["stream-type", config["streamType"]])
    if "bufferPoolName" in config:
        cmd.extend(["buffer-pool-name", config["bufferPoolName"]])

    # Handle active-queue-management (AQM) configuration
    # AQM must come last as it consumes all remaining arguments
    if "aqm" in config:
        aqm = config["aqm"]
        cmd.append("active-queue-management")
        if "behavior" in aqm:
            cmd.extend(["congestion-behavior", aqm["behavior"]])
        if "detection" in aqm:
            detection = aqm["detection"]
            if "linear" in detection:
                linear = detection["linear"]
                cmd.extend(["detection", "linear"])
                if "minimumLength" in linear:
                    cmd.extend(["minimum-length", str(linear["minimumLength"])])
                if "maximumLength" in linear:
                    cmd.extend(["maximum-length", str(linear["maximumLength"])])
                if "probability" in linear:
                    cmd.extend(["probability", str(linear["probability"])])

    run_cli(cmd)


def cleanup_test_config(interface_name: str) -> None:
    """Remove port queue config test data.

    Args:
        interface_name: Interface name to remove portQueueConfigName from.
    """

    def modify_config(sw_config: dict[str, Any]) -> None:
        # Remove test configs
        port_queue_configs = sw_config.get("portQueueConfigs", {})
        port_queue_configs.pop(TEST_POLICY_NAME, None)
        buffer_pool_configs = sw_config.get("bufferPoolConfigs", {})
        buffer_pool_configs.pop(TEST_BUFFER_POOL_NAME, None)
        # Remove portQueueConfigName from test interface
        for port in sw_config.get("ports", []):
            if port.get("name") == interface_name:
                port.pop("portQueueConfigName", None)
                break

    cleanup_config(modify_config, "port queue configs")


def main() -> int:
    print("=" * 70)
    print("CLI E2E Test: Port Queue Configuration")
    print("=" * 70)

    # Find a test interface
    test_intf = find_first_eth_interface()
    print(f"Using test interface: {test_intf.name}")

    # Step 0: Cleanup any existing test config
    print("\n[Step 0] Cleaning up any existing test config...")
    cleanup_test_config(test_intf.name)
    print("  Cleanup complete")

    # Step 1: Configure buffer pool (needed for buffer-pool-name reference)
    print(f"\n[Step 1] Configuring buffer pool '{TEST_BUFFER_POOL_NAME}'...")
    configure_buffer_pool(TEST_BUFFER_POOL_NAME, TEST_BUFFER_POOL_CONFIG)
    print("  Buffer pool configured")

    # Step 2: Configure queues
    print(f"\n[Step 2] Configuring queuing policy '{TEST_POLICY_NAME}'...")
    for queue_config in CLI_QUEUE_CONFIGS:
        queue_id = queue_config["id"]
        print(f"  Configuring queue-id {queue_id}...")
        configure_queue(TEST_POLICY_NAME, queue_id, queue_config)
    print("  All queues configured")
    # Assign queuing policy to interface
    print(f"  Assigning queuing policy to interface '{test_intf.name}'...")
    run_cli(["config", "interface", test_intf.name, "queuing-policy", TEST_POLICY_NAME])
    print("  Queuing policy assigned")

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

    # Verify port queue configs
    actual_queue_configs = sw_config.get("portQueueConfigs", {})
    if TEST_POLICY_NAME not in actual_queue_configs:
        print(f"  ERROR: Queuing policy '{TEST_POLICY_NAME}' not found")
        return 1

    # Sort both lists by id for comparison
    expected_list = sorted(
        EXPECTED_PORT_QUEUE_CONFIGS[TEST_POLICY_NAME], key=lambda x: x["id"]
    )
    actual_list = sorted(actual_queue_configs[TEST_POLICY_NAME], key=lambda x: x["id"])

    if actual_list != expected_list:
        print("  ERROR: Port queue configs mismatch")
        print(f"  Expected: {json.dumps(expected_list, indent=2)}")
        print(f"  Actual:   {json.dumps(actual_list, indent=2)}")
        return 1
    print(f"  Queuing policy '{TEST_POLICY_NAME}' verified")

    # Verify interface has queuing policy assigned
    ports = sw_config.get("ports", [])
    test_port = None
    for port in ports:
        if port.get("name") == test_intf.name:
            test_port = port
            break
    if test_port is None:
        print(f"  ERROR: Interface '{test_intf.name}' not found in config")
        return 1
    actual_policy = test_port.get("portQueueConfigName")
    if actual_policy != TEST_POLICY_NAME:
        print(f"  ERROR: Interface '{test_intf.name}' portQueueConfigName mismatch")
        print(f"  Expected: {TEST_POLICY_NAME}")
        print(f"  Actual:   {actual_policy}")
        return 1
    print(f"  Interface '{test_intf.name}' queuing policy verified")

    print("\n" + "=" * 70)
    print("TEST PASSED")
    print("=" * 70)

    # Step 5: Cleanup test config
    print("\n[Step 5] Cleaning up test config...")
    cleanup_test_config(test_intf.name)
    print("  Cleanup complete")

    return 0


if __name__ == "__main__":
    sys.exit(main())
