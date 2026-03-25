# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

from dataclasses import dataclass
from datetime import datetime
from typing import Any, Dict, List, Optional


@dataclass
class NodeRun:
    """Represents a single node run in a release."""

    node_name: str
    status: Optional[str]  # SUCCESSFUL, FAILED, ONGOING, PAUSED, or None
    scheduled_start_time: Optional[datetime]
    actual_start_time: Optional[datetime]
    actual_end_time: Optional[datetime]
    run_details: str
    run_data: str
    attempt: int = 0
    sandcastle_job_link: Optional[str] = None
    console_log_link: Optional[str] = None
    error_message: Optional[str] = None

    # Raw fields
    conveyor_fbid: Optional[int] = None
    node_instance_id: Optional[str] = None
    release_number: Optional[int] = None
    last_update_time: Optional[datetime] = None


@dataclass
class Release:
    """Represents a conveyor release instance."""

    release_number: int
    release_instance_number: int
    release_instance_id: str
    runs: Dict[str, NodeRun]
    creation_time: Optional[datetime] = None
    creator: Optional[str] = None

    def get_status_counts(self) -> Dict[str, int]:
        """Get counts of nodes by status."""
        counts = {
            "SUCCESSFUL": 0,
            "FAILED": 0,
            "CANCELLED": 0,
            "ONGOING": 0,
            "PAUSED": 0,
            "NOT_RUN": 0,
        }

        for node in self.runs.values():
            if node.status == "SUCCESSFUL":
                counts["SUCCESSFUL"] += 1
            elif node.status == "FAILED":
                counts["FAILED"] += 1
            elif node.status == "CANCELLED":
                counts["CANCELLED"] += 1
            elif node.status == "ONGOING":
                counts["ONGOING"] += 1
            elif node.status == "PAUSED":
                counts["PAUSED"] += 1
            else:  # None or null
                counts["NOT_RUN"] += 1

        return counts

    def get_failed_nodes(self) -> List[NodeRun]:
        """Get all nodes that didn't succeed (FAILED, CANCELLED, or error)."""
        failed = []
        for node in self.runs.values():
            # Only include nodes with actual failure/error statuses
            # Exclude None (NOT_RUN), SUCCESSFUL, ONGOING, PAUSED
            if node.status and node.status not in ["SUCCESSFUL", "ONGOING", "PAUSED"]:
                failed.append(node)
        return failed

    def get_distance_from_qualification(self) -> int:
        """Calculate how many failures away from qualification."""
        counts = self.get_status_counts()
        return counts["FAILED"] + counts["CANCELLED"] + counts["NOT_RUN"]


def parse_timestamp(time_dict: Optional[Dict[str, Any]]) -> Optional[datetime]:
    """Parse timestamp dict to datetime object."""
    if not time_dict or "secs" not in time_dict:
        return None

    secs = time_dict["secs"]
    if secs == 0:
        return None

    return datetime.fromtimestamp(secs)
