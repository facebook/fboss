#!/usr/bin/env python3

# Copyright 2004-present Facebook. All Rights Reserved.

# pyre-unsafe

import argparse
import getpass
import json
import logging
import os
import sys
import time
import typing as t
from datetime import date
from typing import Any, Dict, Optional

from analytics.bamboo import Bamboo as bb
from libfb.py import employee
from libfb.py.asyncio.await_utils import asyncio
from libfb.py.mail import async_send_internal_email
from libfb.py.thrift_clients.oncall_thrift_client import OncallThriftClient


# Configure logging
logger: logging.Logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)

# Constants
DEFAULT_MAX_RECORDS = "100"  # Default number of records returned from scuba
DEFAULT_OUTPUT_FILE = "known_bad_tests_data.json"
ONCALL = "fboss_agent_push"
PASSED = 1
FAILED = 0


class UserAndEmailHandler:
    @staticmethod
    def get_oncall_info(oncall_name: str) -> Any:
        """
        Retrieve the user ID of the current on-call person for a given rotation by short name.

        Args:
            oncall_name (str): The short name of the on-call rotation.

        Returns:
            int: The user ID of the current on-call person.
        """
        with OncallThriftClient() as client:
            oncall = client.getCurrentOncallForRotationByShortName(oncall_name)
            logger.info(f"Current on-call for {oncall}: {oncall.uid}")
            return oncall

    @staticmethod
    def get_user_data(isUser: bool) -> t.Tuple[str, int, str]:
        """
        Get the current user's name and ID, falling back to on-call information if needed.

        Returns:
            tuple: A tuple containing:
                - str: The username (either the current user or the on-call name)
                - int: The user ID corresponding to the username
                - str: The email of the on-call person
        """
        if isUser:
            user_name = os.environ.get("SUDO_USER", getpass.getuser())
            user_id = employee.unixname_to_uid(user_name)
            return user_name, user_id, ""
        else:
            oncall = UserAndEmailHandler.get_oncall_info(ONCALL)
            user_name = ONCALL
            logger.info(f"User name: {user_name}, User ID: {oncall.uid}")
            return user_name, oncall.uid, oncall.person_email

    @staticmethod
    async def send_email(
        isUser: bool, html: str, images: Optional[Dict[str, bytes]]
    ) -> None:
        """
        Send an email with the given HTML content and images.

        Args:
            html (str): The HTML content of the email.
            images (Dict[str, bytes]): A dictionary of image names and their byte content to be included in the email.
        """
        subject = "Known bad tests report. Date: %s" % date.today().strftime("%Y-%m-%d")

        user_name, user_id, user_email = UserAndEmailHandler.get_user_data(isUser)

        if user_email:
            to_addrs = [user_email]
        else:
            to_addrs = [("%s@meta.com" % user_name)]

        logger.info(f"Sending email to {to_addrs}")
        await async_send_internal_email(
            sender="noreply@meta.com",
            to=to_addrs,
            subject=subject,
            body=html,
            inline_images=images,
            is_html=True,
        )

    @staticmethod
    def format_workplace_post(tests: Dict[str, int]) -> str:
        """Format the test data into a Workplace post."""
        html = "<h1>Known bad tests passing continuously for a week</h1>"
        html += "<p>Here is the list of known bad tests passing 7 times in a row:</p>"
        html += "<p>Please remove them from known bad. </p>"
        html += "<table>"
        html += "<tr><th>Test Name</th></tr>"
        for test_name, _status in tests.items():
            html += f"<tr><td>{test_name}</td></tr>"
        html += "</table>"
        return html


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


def main() -> Optional[int]:
    """Main function to parse command line arguments and run the script."""
    parser = argparse.ArgumentParser(
        description="Get data from a Scuba for sai_agent_known_bad_test"
    )

    parser.add_argument(
        "--query_scuba",
        action="store_true",
        help="Query Scuba for test results",
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        help="Print debug messages",
    )

    parser.add_argument(
        "--user",
        action="store_true",
        help="Send email to user instead of oncall",
    )

    args = parser.parse_args()

    if args.query_scuba:
        # Query Scuba for test result
        logger.info("Querying Scuba for test results...")
        sql_query = ScubaQueryBuilder.build_query_for_test_results()
        df = ScubaQueryBuilder.execute_query(sql_query)

        if df is None:
            logger.error("Error: Could not execute Scuba query")
            return 1
        logger.info(f"Scuba query returned {len(df)} rows")
        tests = {}

        for _, row in df.iterrows():
            if row["status"] == PASSED and row["count"] == 7:
                if row["test_name"] not in tests:
                    tests[row["test_name"]] = row["status"]

        logger.info(f"Found {len(tests)} known bad tests passing 7 times in a row")

        # Write test data to a separate JSON file
        if tests:
            with open(DEFAULT_OUTPUT_FILE, "w") as f:
                json.dump(tests, f, indent=2, default=str)
            logger.info(f"Results written to {DEFAULT_OUTPUT_FILE}")

            # send email to user or oncall
            logger.info("Sending email...")
            html = UserAndEmailHandler.format_workplace_post(tests)

            logger.info("html generated successfully for email body")
            images = {}
            return_code = asyncio.run(
                UserAndEmailHandler.send_email(args.user, html, images)
            )

            if return_code == 0:
                logger.error(
                    f"Error: Failed to send email to user, return_code: {return_code}"
                )
                return return_code

            logger.info(f"Email sent successfully return_code: {return_code}")

    return 0


# No code to edit in the selected snippet.

if __name__ == "__main__":
    sys.exit(main())
