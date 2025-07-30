#!/usr/bin/env python3

# Copyright 2004-present Facebook. All Rights Reserved.

import logging
import sys
import time
from typing import Any, Optional

from analytics.bamboo import Bamboo as bb


# Configure logging
logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)

# Constants
DEFAULT_MAX_RECORDS = "100"  # Default number of records returned from scuba
DEFAULT_CLUSTER = "gp"
DEFAULT_LOG_TYPE = "stderr"
DEFAULT_OUTPUT_FILE = "chronos_job_data.json"
ONCALL = "fboss_agent_push"
PASSED = 1
FAILED = 0


class ScubaQueryBuilder:
    """Class to build and execute Scuba queries for Chronos jobs."""

    @staticmethod
    def build_query_for_test_results(
        limit: str = DEFAULT_MAX_RECORDS,
    ) -> str:
        """Build a Scuba query for Chronos jobs."""
        current_time = int(time.time())
        consider_results_since = str(current_time - 60 * 60 * 24 * 7)  # Last 7 days

        sql_query = f"""
            SELECT
                SUM(1, `weight`) AS `hits`,
                COUNT(1) AS `samples`,
                `test_name`,
                `status`
            FROM `testinfra_db_results`
            WHERE
                {consider_results_since} <= `time`
                AND `time` <= {current_time}
                AND (CONTAINS(`sandcastle_alias`, ARRAY('sai_agent_known_bad_test')))
                AND ((`purpose`) IN ('stress-run'))
                AND (`status`) IN (1)
            GROUP BY
                `test_name`,
                `status`
            ORDER BY
                `hits` DESC
            LIMIT
                {limit}
            """

        return sql_query

    @staticmethod
    def execute_query(sql_query: str) -> Optional[Any]:
        """Execute a Scuba query and return the results."""
        try:
            df = bb.query_scuba_nullable(sql_query)
            return df
        except Exception as e:
            logger.error(f"Error querying Scuba: {e}")
            return None


def main():
    pass


# No code to edit in the selected snippet.

if __name__ == "__main__":
    sys.exit(main())
