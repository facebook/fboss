#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

"""
End-to-end tests for QoS Policy CLI commands.

This test covers:
1. QoS policy creation with map entries (config qos policy <name> map ...)

This test:
1. Cleans up any existing test QoS policy
2. Creates a new QoS policy with various map entries:
   - trafficClassToQueueId (tc-to-queue)
   - pfcPriorityToQueueId (pfc-pri-to-queue)
   - trafficClassToPgId (tc-to-pg)
   - pfcPriorityToPgId (pfc-pri-to-pg)
3. Commits the configuration and verifies it was applied
4. Cleans up the test config
"""

import json
import sys
from typing import Any, Dict, Optional

from cli_test_lib import (
    SYSTEM_CONFIG_PATH,
    cleanup_config,
    commit_config,
    run_cli,
)

# Test policy name
TEST_POLICY_NAME = "cli_e2e_test_qos_policy"

# Expected QoS map configuration (based on l3_scaleup.conf sample config)
# All maps use identity mapping: 0->0, 1->1, ..., 7->7
EXPECTED_TC_TO_QUEUE = {str(i): i for i in range(8)}
EXPECTED_PFC_PRI_TO_QUEUE = {str(i): i for i in range(8)}
EXPECTED_TC_TO_PG = {str(i): i for i in range(8)}
EXPECTED_PFC_PRI_TO_PG = {str(i): i for i in range(8)}


def configure_qos_policy_maps(policy_name: str) -> None:
    """Configure QoS policy with all map entries."""
    base_cmd = ["config", "qos", "policy", policy_name, "map"]

    # Configure trafficClassToQueueId (tc-to-queue)
    print("  Configuring trafficClassToQueueId (tc-to-queue)...")
    for tc, queue in EXPECTED_TC_TO_QUEUE.items():
        run_cli(base_cmd + ["tc-to-queue", tc, str(queue)])

    # Configure pfcPriorityToQueueId (pfc-pri-to-queue)
    print("  Configuring pfcPriorityToQueueId (pfc-pri-to-queue)...")
    for pfc_pri, queue in EXPECTED_PFC_PRI_TO_QUEUE.items():
        run_cli(base_cmd + ["pfc-pri-to-queue", pfc_pri, str(queue)])

    # Configure trafficClassToPgId (tc-to-pg)
    print("  Configuring trafficClassToPgId (tc-to-pg)...")
    for tc, pg in EXPECTED_TC_TO_PG.items():
        run_cli(base_cmd + ["tc-to-pg", tc, str(pg)])

    # Configure pfcPriorityToPgId (pfc-pri-to-pg)
    print("  Configuring pfcPriorityToPgId (pfc-pri-to-pg)...")
    for pfc_pri, pg in EXPECTED_PFC_PRI_TO_PG.items():
        run_cli(base_cmd + ["pfc-pri-to-pg", pfc_pri, str(pg)])


def cleanup_test_config() -> None:
    """Remove test QoS policy from config."""

    def modify_config(sw_config: Dict[str, Any]) -> None:
        # Remove test QoS policy from qosPolicies list
        qos_policies = sw_config.get("qosPolicies", [])
        sw_config["qosPolicies"] = [
            p for p in qos_policies if p.get("name") != TEST_POLICY_NAME
        ]

    cleanup_config(modify_config, "QoS policy test configs")


def verify_map(
    actual: Optional[Dict[str, int]], expected: Dict[str, int], map_name: str
) -> bool:
    """Verify a QoS map matches expected values."""
    if actual is None:
        print(f"  ERROR: {map_name} is None")
        return False

    if actual != expected:
        print(f"  ERROR: {map_name} mismatch")
        print(f"  Expected: {expected}")
        print(f"  Actual:   {actual}")
        return False

    print(f"  {map_name} verified")
    return True


def main() -> int:
    print("=" * 70)
    print("CLI E2E Test: QoS Policy Configuration")
    print("=" * 70)

    # Step 0: Cleanup any existing test config
    print("\n[Step 0] Cleaning up any existing test config...")
    cleanup_test_config()
    print("  Cleanup complete")

    # Step 1: Configure QoS policy with map entries
    print(f"\n[Step 1] Configuring QoS policy '{TEST_POLICY_NAME}'...")
    configure_qos_policy_maps(TEST_POLICY_NAME)
    print("  QoS policy configured")

    # Step 2: Commit the configuration
    print("\n[Step 2] Committing configuration...")
    commit_config()
    print("  Configuration committed successfully")

    # Step 3: Verify configuration by reading /etc/coop/agent.conf
    print("\n[Step 3] Verifying configuration...")
    with open(SYSTEM_CONFIG_PATH, "r") as f:
        config = json.load(f)

    sw_config = config.get("sw", {})

    # Find our test policy in qosPolicies list
    qos_policies = sw_config.get("qosPolicies", [])
    test_policy = None
    for policy in qos_policies:
        if policy.get("name") == TEST_POLICY_NAME:
            test_policy = policy
            break

    if test_policy is None:
        print(f"  ERROR: QoS policy '{TEST_POLICY_NAME}' not found in config")
        return 1
    print(f"  QoS policy '{TEST_POLICY_NAME}' found")

    # Verify qosMap exists
    qos_map = test_policy.get("qosMap")
    if qos_map is None:
        print("  ERROR: qosMap not found in policy")
        return 1

    # Verify each map
    all_verified = True

    # trafficClassToQueueId
    actual_tc_to_queue = qos_map.get("trafficClassToQueueId")
    if not verify_map(
        actual_tc_to_queue, EXPECTED_TC_TO_QUEUE, "trafficClassToQueueId"
    ):
        all_verified = False

    # pfcPriorityToQueueId
    actual_pfc_to_queue = qos_map.get("pfcPriorityToQueueId")
    if not verify_map(
        actual_pfc_to_queue, EXPECTED_PFC_PRI_TO_QUEUE, "pfcPriorityToQueueId"
    ):
        all_verified = False

    # trafficClassToPgId
    actual_tc_to_pg = qos_map.get("trafficClassToPgId")
    if not verify_map(actual_tc_to_pg, EXPECTED_TC_TO_PG, "trafficClassToPgId"):
        all_verified = False

    # pfcPriorityToPgId
    actual_pfc_to_pg = qos_map.get("pfcPriorityToPgId")
    if not verify_map(actual_pfc_to_pg, EXPECTED_PFC_PRI_TO_PG, "pfcPriorityToPgId"):
        all_verified = False

    if not all_verified:
        print("\n" + "=" * 70)
        print("TEST FAILED")
        print("=" * 70)
        return 1

    print("\n" + "=" * 70)
    print("TEST PASSED")
    print("=" * 70)

    # Step 4: Cleanup test config
    print("\n[Step 4] Cleaning up test config...")
    cleanup_test_config()
    print("  Cleanup complete")

    return 0


if __name__ == "__main__":
    sys.exit(main())
