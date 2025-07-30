#!/usr/bin/env python3

# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import json
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
                SUM(1, `weight`) AS `count`,
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
                `count` DESC
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
    """Main function to parse command line arguments and run the script."""
    parser = argparse.ArgumentParser(
        description="Get data from a Scuba for sai_agent_known_bad_test"
    )

    parser.add_argument(
        "--scuba_query",
        action="store_true",
        help="Query Scuba for test results",
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        help="Print debug messages",
    )

    args = parser.parse_args()

    if args.scuba_query:
        # Query Scuba for test result
        logger.info("Querying Scuba for test results...")
        sql_query = ScubaQueryBuilder.build_query_for_test_results()
        df = ScubaQueryBuilder.execute_query(sql_query)

        if df is None:
            logger.error("Error: Could not execute Scuba query")
            return 1
        logger.info(f"Scuba query returned {len(df)} rows")
        tests = {}

        for index, row in df.iterrows():
            if row["status"] == PASSED and row["count"] == 7:
                if row["test_name"] not in tests:
                    tests[row["test_name"]] = row["status"]

        # Write test data to a separate JSON file
        if tests:
            with open(DEFAULT_OUTPUT_FILE, "w") as f:
                json.dump(tests, f, indent=2, default=str)
            logger.info(f"Results written to {DEFAULT_OUTPUT_FILE}")

        return 0


# No code to edit in the selected snippet.

if __name__ == "__main__":
    sys.exit(main())
