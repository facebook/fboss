# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

import unittest
from datetime import datetime
from typing import Any, Dict

from ..parser import NodeRun, Release


class ReleaseGetStatusCountsTest(unittest.TestCase):
    """Tests for Release.get_status_counts method."""

    def test_get_status_counts_with_all_successful_nodes(self) -> None:
        """Test get_status_counts returns correct counts when all nodes are successful."""
        # Setup: Create a release with all successful nodes
        runs = {
            "node1": NodeRun(
                node_name="node1",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node2": NodeRun(
                node_name="node2",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_status_counts
        counts = release.get_status_counts()

        # Assert: Verify counts are correct
        self.assertEqual(counts["SUCCESSFUL"], 2)
        self.assertEqual(counts["FAILED"], 0)
        self.assertEqual(counts["CANCELLED"], 0)
        self.assertEqual(counts["ONGOING"], 0)
        self.assertEqual(counts["PAUSED"], 0)
        self.assertEqual(counts["NOT_RUN"], 0)

    def test_get_status_counts_with_mixed_statuses(self) -> None:
        """Test get_status_counts returns correct counts with mixed node statuses."""
        # Setup: Create a release with various node statuses
        runs = {
            "node1": NodeRun(
                node_name="node1",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node2": NodeRun(
                node_name="node2",
                status="FAILED",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node3": NodeRun(
                node_name="node3",
                status="CANCELLED",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node4": NodeRun(
                node_name="node4",
                status="ONGOING",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node5": NodeRun(
                node_name="node5",
                status=None,
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_status_counts
        counts = release.get_status_counts()

        # Assert: Verify each status is counted correctly
        self.assertEqual(counts["SUCCESSFUL"], 1)
        self.assertEqual(counts["FAILED"], 1)
        self.assertEqual(counts["CANCELLED"], 1)
        self.assertEqual(counts["ONGOING"], 1)
        self.assertEqual(counts["PAUSED"], 0)
        self.assertEqual(counts["NOT_RUN"], 1)

    def test_get_status_counts_with_empty_runs(self) -> None:
        """Test get_status_counts returns all zeros when release has no runs."""
        # Setup: Create a release with no runs
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs={},
        )

        # Execute: Call get_status_counts
        counts = release.get_status_counts()

        # Assert: Verify all counts are zero
        self.assertEqual(counts["SUCCESSFUL"], 0)
        self.assertEqual(counts["FAILED"], 0)
        self.assertEqual(counts["CANCELLED"], 0)
        self.assertEqual(counts["ONGOING"], 0)
        self.assertEqual(counts["PAUSED"], 0)
        self.assertEqual(counts["NOT_RUN"], 0)


class ReleaseGetFailedNodesTest(unittest.TestCase):
    """Tests for Release.get_failed_nodes method."""

    def test_get_failed_nodes_returns_only_failed_and_cancelled(self) -> None:
        """Test get_failed_nodes returns only nodes with FAILED or CANCELLED status."""
        # Setup: Create a release with failed and cancelled nodes
        failed_node = NodeRun(
            node_name="failed_node",
            status="FAILED",
            scheduled_start_time=None,
            actual_start_time=None,
            actual_end_time=None,
            run_details="",
            run_data="",
        )
        cancelled_node = NodeRun(
            node_name="cancelled_node",
            status="CANCELLED",
            scheduled_start_time=None,
            actual_start_time=None,
            actual_end_time=None,
            run_details="",
            run_data="",
        )
        successful_node = NodeRun(
            node_name="successful_node",
            status="SUCCESSFUL",
            scheduled_start_time=None,
            actual_start_time=None,
            actual_end_time=None,
            run_details="",
            run_data="",
        )
        runs = {
            "failed_node": failed_node,
            "cancelled_node": cancelled_node,
            "successful_node": successful_node,
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_failed_nodes
        failed_nodes = release.get_failed_nodes()

        # Assert: Verify only failed and cancelled nodes are returned
        self.assertEqual(len(failed_nodes), 2)
        self.assertIn(failed_node, failed_nodes)
        self.assertIn(cancelled_node, failed_nodes)
        self.assertNotIn(successful_node, failed_nodes)

    def test_get_failed_nodes_excludes_not_run_nodes(self) -> None:
        """Test get_failed_nodes excludes nodes with None status (NOT_RUN)."""
        # Setup: Create a release with NOT_RUN nodes
        not_run_node = NodeRun(
            node_name="not_run_node",
            status=None,
            scheduled_start_time=None,
            actual_start_time=None,
            actual_end_time=None,
            run_details="",
            run_data="",
        )
        failed_node = NodeRun(
            node_name="failed_node",
            status="FAILED",
            scheduled_start_time=None,
            actual_start_time=None,
            actual_end_time=None,
            run_details="",
            run_data="",
        )
        runs = {"not_run_node": not_run_node, "failed_node": failed_node}
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_failed_nodes
        failed_nodes = release.get_failed_nodes()

        # Assert: Verify NOT_RUN nodes are excluded
        self.assertEqual(len(failed_nodes), 1)
        self.assertNotIn(not_run_node, failed_nodes)
        self.assertIn(failed_node, failed_nodes)

    def test_get_failed_nodes_returns_empty_list_when_all_successful(self) -> None:
        """Test get_failed_nodes returns empty list when all nodes are successful."""
        # Setup: Create a release with only successful nodes
        runs = {
            "node1": NodeRun(
                node_name="node1",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node2": NodeRun(
                node_name="node2",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_failed_nodes
        failed_nodes = release.get_failed_nodes()

        # Assert: Verify empty list is returned
        self.assertEqual(failed_nodes, [])


class ReleaseGetDistanceFromQualificationTest(unittest.TestCase):
    """Tests for Release.get_distance_from_qualification method."""

    def test_get_distance_from_qualification_with_all_successful(self) -> None:
        """Test get_distance_from_qualification returns 0 when all nodes are successful."""
        # Setup: Create a release with all successful nodes
        runs = {
            "node1": NodeRun(
                node_name="node1",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_distance_from_qualification
        distance = release.get_distance_from_qualification()

        # Assert: Verify distance is 0
        self.assertEqual(distance, 0)

    def test_get_distance_from_qualification_counts_failed_cancelled_not_run(
        self,
    ) -> None:
        """Test get_distance_from_qualification sums FAILED, CANCELLED, and NOT_RUN nodes."""
        # Setup: Create a release with failed, cancelled, and not run nodes
        runs = {
            "node1": NodeRun(
                node_name="node1",
                status="FAILED",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node2": NodeRun(
                node_name="node2",
                status="CANCELLED",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node3": NodeRun(
                node_name="node3",
                status=None,
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node4": NodeRun(
                node_name="node4",
                status="SUCCESSFUL",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_distance_from_qualification
        distance = release.get_distance_from_qualification()

        # Assert: Verify distance is 3 (1 failed + 1 cancelled + 1 not run)
        self.assertEqual(distance, 3)

    def test_get_distance_from_qualification_excludes_ongoing_and_paused(self) -> None:
        """Test get_distance_from_qualification excludes ONGOING and PAUSED nodes."""
        # Setup: Create a release with ongoing and paused nodes
        runs = {
            "node1": NodeRun(
                node_name="node1",
                status="ONGOING",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node2": NodeRun(
                node_name="node2",
                status="PAUSED",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
            "node3": NodeRun(
                node_name="node3",
                status="FAILED",
                scheduled_start_time=None,
                actual_start_time=None,
                actual_end_time=None,
                run_details="",
                run_data="",
            ),
        }
        release = Release(
            release_number=6655,
            release_instance_number=1,
            release_instance_id="R6655.1",
            runs=runs,
        )

        # Execute: Call get_distance_from_qualification
        distance = release.get_distance_from_qualification()

        # Assert: Verify distance is 1 (only the failed node counts)
        self.assertEqual(distance, 1)


class ParseTimestampTest(unittest.TestCase):
    """Tests for parse_timestamp function."""

    def test_parse_timestamp_with_valid_timestamp(self) -> None:
        """Test parse_timestamp converts valid timestamp dict to datetime."""
        # Setup: Create a valid timestamp dict
        from ..parser import parse_timestamp

        time_dict = {"secs": 1700000000}

        # Execute: Call parse_timestamp
        result = parse_timestamp(time_dict)

        # Assert: Verify datetime object is returned with correct timestamp
        self.assertIsInstance(result, datetime)
        self.assertEqual(result, datetime.fromtimestamp(1700000000))

    def test_parse_timestamp_with_none_input(self) -> None:
        """Test parse_timestamp returns None when given None input."""
        # Setup: None input
        from ..parser import parse_timestamp

        # Execute: Call parse_timestamp with None
        result = parse_timestamp(None)

        # Assert: Verify None is returned
        self.assertIsNone(result)

    def test_parse_timestamp_with_missing_secs_key(self) -> None:
        """Test parse_timestamp returns None when 'secs' key is missing."""
        # Setup: Create timestamp dict without 'secs' key
        from ..parser import parse_timestamp

        time_dict = {"other_key": 12345}

        # Execute: Call parse_timestamp
        result = parse_timestamp(time_dict)

        # Assert: Verify None is returned
        self.assertIsNone(result)

    def test_parse_timestamp_with_zero_secs(self) -> None:
        """Test parse_timestamp returns None when secs is 0."""
        # Setup: Create timestamp dict with secs = 0
        from ..parser import parse_timestamp

        time_dict = {"secs": 0}

        # Execute: Call parse_timestamp
        result = parse_timestamp(time_dict)

        # Assert: Verify None is returned (0 represents no timestamp)
        self.assertIsNone(result)

    def test_parse_timestamp_with_empty_dict(self) -> None:
        """Test parse_timestamp returns None when given empty dict."""
        # Setup: Empty dict
        from ..parser import parse_timestamp

        time_dict: Dict[str, Any] = {}

        # Execute: Call parse_timestamp
        result = parse_timestamp(time_dict)

        # Assert: Verify None is returned
        self.assertIsNone(result)

    def test_parse_timestamp_with_negative_timestamp(self) -> None:
        """Test parse_timestamp handles negative timestamps correctly."""
        # Setup: Create timestamp dict with negative secs (pre-epoch)
        from ..parser import parse_timestamp

        time_dict = {"secs": -100000}

        # Execute: Call parse_timestamp
        result = parse_timestamp(time_dict)

        # Assert: Verify datetime object is returned for pre-epoch time
        self.assertIsInstance(result, datetime)
        self.assertEqual(result, datetime.fromtimestamp(-100000))

    def test_parse_timestamp_with_future_timestamp(self) -> None:
        """Test parse_timestamp handles future timestamps correctly."""
        # Setup: Create timestamp dict with future secs
        from ..parser import parse_timestamp

        time_dict = {"secs": 2000000000}

        # Execute: Call parse_timestamp
        result = parse_timestamp(time_dict)

        # Assert: Verify datetime object is returned for future time
        self.assertIsInstance(result, datetime)
        self.assertEqual(result, datetime.fromtimestamp(2000000000))
