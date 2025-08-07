#!/usr/bin/env python3

# Copyright 2004-present Facebook. All Rights Reserved.

# pyre-unsafe

import argparse
import getpass
import json
import logging
import os
import re
import sys
import time
import typing as t
from datetime import date
from typing import Any, Dict, List, Optional

import libfb.py.asyncio.tasks as task_api

from analytics.bamboo import Bamboo as bb
from libfb.py import employee
from libfb.py.asyncio.await_utils import asyncio
from libfb.py.asyncio.sandcastle import AsyncSandcastleClient
from libfb.py.asyncio.tasks import Task, TaskPriority
from libfb.py.mail import async_send_internal_email
from libfb.py.thrift_clients.oncall_thrift_client import OncallThriftClient
from schedulers.chronos.py.scripts.chronos.ji_log_actions import get_chronos_ji_logs


# Configure logging
logger: logging.Logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)

# Constants
DEFAULT_MAX_RECORDS = "200"  # Default number of records returned from scuba
DEFAULT_OUTPUT_FILE = "known_bad_tests_data.json"
DEFAULT_CLUSTER = "gp"
ONCALL = "fboss_agent_push"
PASSED = 1
FAILED = 0
USER_UNIX_ID = "prasoon"
KNOWN_BAD_TEST = "known_bad_tests"
FBOSS_KNOWN_BAD_TESTS = f"fboss_{KNOWN_BAD_TEST}"
DEFAULT_LOG_TYPE = "stderr"


class UserAndEmailHandler:
    def get_oncall_info(self, oncall_name: str) -> Any:
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

    def _get_user_data(self, isUser: bool) -> t.Tuple[str, int, str]:
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
            oncall = UserAndEmailHandler().get_oncall_info(ONCALL)
            user_name = ONCALL
            logger.info(f"User name: {user_name}, User ID: {oncall.uid}")
            return user_name, oncall.uid, oncall.person_email

    async def send_email(
        self, isUser: bool, html: str, images: Optional[Dict[str, bytes]]
    ) -> None:
        """
        Send an email with the given HTML content and images.

        Args:
            html (str): The HTML content of the email.
            images (Dict[str, bytes]): A dictionary of image names and their byte content to be included in the email.
        """
        subject = "Known bad tests report. Date: %s" % date.today().strftime("%Y-%m-%d")

        user_name, _user_id, user_email = self._get_user_data(isUser)

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

    async def create_task(self, isUser: bool, tests: Dict[str, int]) -> Task:
        owner, _user_id, _user_email = self._get_user_data(isUser)

        return await task_api.create_task(
            creator=USER_UNIX_ID,
            title="Known Bad Tests Passing Continuously for a week. Please remove them from known bad.",
            description=self.text_format_known_bad_list(tests),
            priority=TaskPriority.HIGH,
            tags=[FBOSS_KNOWN_BAD_TESTS],
            owner=owner,
        )

    def HTTP_format_known_bad_list(
        self, tests: Dict[str, int], known_bad_tests: Dict[str, int]
    ) -> str:
        """Format the test data into a Workplace post."""
        html = "<h1>Known bad tests passing continuously for a week</h1>"
        html += "<p>Here is the list of known bad tests passing 7 times in a row:</p>"
        html += "<p>Please remove them from known bad. </p>"
        html += "<table>"
        html += "<tr><th>Test Name</th></tr>"
        for test_name, _status in tests.items():
            html += f"<tr><td>{test_name}</td></tr>"
        html += "</table>"
        html += "<h1>Total known bad tests</h1>"
        html += "<p>Here is the list of all known bad tests:</p>"
        html += "<table>"
        html += "<tr><th>Test Name</th></tr>"
        for test_name, status in known_bad_tests.items():
            html += f"<tr><td>{test_name}: {status}</td></tr>"
        html += "</table>"

        return html

    def text_format_known_bad_list(self, tests: Dict[str, int]) -> str:
        """Format the test data into a Workplace post."""
        text = "Known bad tests passing continuously for a week\n"
        text += "Here is the list of known bad tests passing 7 times in a row:\n"
        text += "Please remove them from known bad.\n"
        text += "Test Name\n"
        for test_name, _status in tests.items():
            text += f"{test_name}\n"

        text += "\nTotal known bad tests\n"
        text += "Here is the list of all known bad tests:\n"
        text += "Test Name\n"
        for test_name, status in tests.items():
            text += f"{test_name}: {status}\n"

        return text


class ScubaQueryBuilder:
    """Class to build and execute Scuba queries for Chronos jobs."""

    def build_query_for_test_results(
        self,
        job_name_prefix: str,
        limit: str = DEFAULT_MAX_RECORDS,
        consider_results_since: Optional[int] = None,
        status: Optional[bool] = None,
    ) -> str:
        """Build a Scuba query for Chronos jobs."""
        current_time = int(time.time())
        if not consider_results_since:
            consider_results_since = current_time - 60 * 60 * 24 * 7  # Last 7 days

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
                AND (CONTAINS(`sandcastle_alias`, ARRAY('{job_name_prefix}')))
                AND ((`purpose`) IN ('stress-run'))
                AND (status IS TRUE)
            GROUP BY
                `test_name`,
                `status`
            ORDER BY
                `count` DESC
            """

        return sql_query

    def build_query_for_chronos_job_id(
        self,
        job_name_prefix: str,
        cluster: str = DEFAULT_CLUSTER,
        excluded_reasons: Optional[List[str]] = None,
        limit: str = DEFAULT_MAX_RECORDS,
        queue: Optional[str] = None,
    ) -> str:
        """Build a Scuba query for Chronos jobs."""
        current_time = int(time.time())
        consider_results_since = str(current_time - 60 * 60 * 24)  # Last 24 hours

        sql_base = f"""
            SELECT
                IF(exit_code != '0', "Fail", "Success"),
                cluster_name + '/' + jobname,
                `job_instance_id`,
                `jobname`,
                `jobId`,
                `jobVersion`
            FROM `chronos_job_instance_states`
            WHERE
                {consider_results_since} <= `time`
                AND `time` <= {current_time}
                AND (CONTAINS(cluster_name + '/' + jobname, ARRAY('{job_name_prefix}')))
            """

        if cluster:
            sql_base += f" AND ((`cluster_name`) IN ('{cluster}'))"

        if excluded_reasons:
            excluded_reasons_str = "', '".join(excluded_reasons)
            sql_base += f" AND NOT ((`reason`) IN ('{excluded_reasons_str}'))"

        if queue:
            sql_base += f" AND ((`queue`) IN ('{queue}'))"

        return sql_base

    def execute_query(self, sql_query: str) -> Optional[Any]:
        """Execute a Scuba query and return the results."""
        try:
            df = bb.query_scuba_nullable(sql_query)
            return df
        except Exception as e:
            logger.error(f"Error querying Scuba: {e}")
            return None


class SandcastleClient:
    """Class to interact with Sandcastle jobs."""

    async def get_logs(self, job_id: int) -> Dict[str, str]:
        """Get logs from a Sandcastle job."""
        try:
            async with AsyncSandcastleClient() as client:
                job_logs_dict = {}

                job_spec = await client.get_spec(job_id)

                logger.info(f"Processing Sandcastle job: {job_id}")

                # Extract steps based on job type
                if "steps" in job_spec["args"]:
                    steps = [s["name"] for s in job_spec["args"]["steps"]]
                elif (
                    job_spec["command"] == "SandcastleSkycastleCommand"
                    and "steps" not in job_spec["args"]
                ):
                    # For SandcastleSkycastleCommand without steps, extract from logs
                    workflow_log = await client.get_log(job_id)
                    pattern = r'Starting step "(.*?)"'
                    steps = [
                        match
                        for line in workflow_log
                        for match in re.findall(pattern, line)
                    ]
                    logger.warning(
                        f"Skycastle {job_id} unexpectedly has no steps created"
                    )
                else:
                    raise ValueError(f"Unsupported command: {job_spec['command']}")

                logger.info(f"Found {len(steps)} steps in Sandcastle job {job_id}")
                logger.info("Getting logs for step: Run tests")
                logs_list = await client.get_log(job_id, step_name="Run tests")
                job_logs_dict["Run tests"] = "\n".join(logs_list)

                return job_logs_dict

        except Exception as e:
            logger.error(f"Error retrieving Sandcastle logs for job {job_id}: {e}")
            raise

    def _extract_content_sandcastle_logs(self, logs: str) -> int:
        """Extract content from Sandcastle logs."""
        # Extract content from logs
        tests = int(0)
        # Use regex to find the pattern "Enqueued <number> tests"
        match = re.search(r"multiprocess_test_orchestrator: Enqueued (\d+) tests", logs)
        if match:
            tests = int(match.group(1))

        return tests


class ChronosJobExtractor:
    """Main class to extract data from Chronos jobs."""

    def __init__(self) -> None:
        """Initialize the ChronosJobExtractor class."""

    def get_logs(
        self,
        job_instance_id: int,
        log_type: str = DEFAULT_LOG_TYPE,
        cluster: str = DEFAULT_CLUSTER,
    ) -> str:
        """Get logs from a Chronos job instance."""
        try:
            return get_chronos_ji_logs(
                job_instance_id=job_instance_id,
                cluster=cluster,
                log_type=log_type,
                head=100,
                tail=None,
                suppress_logs=True,
            )
        except Exception as e:
            if "Cannot find the job instance" in str(e):
                error_msg = [
                    f"Error: Job instance {job_instance_id} not found.",
                    "This could be because:",
                    "  1. The job ID is incorrect",
                    "  2. The job has expired and its logs are no longer available",
                    f"  3. The job belongs to a different cluster (default is '{cluster}')",
                    "\nPlease verify the job ID and try again.",
                    "For more information, see: https://www.internalfb.com/intern/wiki/ChronosGuide/logging-and-datasets/Chronos_Unified_Log_API/",
                ]
                logger.error("\n".join(error_msg))
                return f"ERROR: Job instance {job_instance_id} not found."
            else:
                logger.error(f"Error retrieving Chronos logs: {e}")
                raise

    def _get_sandcastle_jobs_from_instance(
        self, instance_id: int, cluster: str = DEFAULT_CLUSTER
    ) -> List[str]:
        """Get Sandcastle job IDs from a Chronos job instance."""
        try:
            instance_logs = self.get_logs(instance_id, cluster=cluster)

            # Extract Sandcastle job IDs from logs
            sandcastle_ids = []
            for line in instance_logs.split("\n"):
                if "Sandcastle job created:" in line:
                    match = re.search(
                        r"https://www\.internalfb\.com/intern/sandcastle/job/(\d+)/",
                        line,
                    )
                    if match:
                        sandcastle_ids.append(match.group(1))

            return sandcastle_ids
        except Exception as e:
            logger.error(
                f"Error getting Sandcastle jobs from instance {instance_id}: {e}"
            )
            return []

    async def process_instance(
        self,
        instance_id: int,
        job_name: str = "default_job_name",
        cluster: str = DEFAULT_CLUSTER,
        include_sandcastle: bool = True,
    ) -> int:
        """Process a Chronos job instance and return its data."""
        try:
            tests = 0
            # Get Sandcastle jobs if requested
            if include_sandcastle:
                sandcastle_ids = self._get_sandcastle_jobs_from_instance(
                    instance_id, cluster
                )

                for sc_id in sandcastle_ids:
                    try:
                        sc_logs = await SandcastleClient().get_logs(int(sc_id))

                        # Process the "Run tests" step if it exists
                        if "Run tests" in sc_logs:
                            logger.info(
                                f"Found 'Run tests' step in Sandcastle job {sc_id}"
                            )
                            tests = SandcastleClient()._extract_content_sandcastle_logs(
                                sc_logs["Run tests"]
                            )
                            logger.info(f"Enqueued {tests} tests")
                        else:
                            logger.info(
                                f"No 'Run tests' step found in Sandcastle job {sc_id}"
                            )
                            tests = 0
                    except Exception as e:
                        logger.error(f"Error processing Sandcastle job {sc_id}: {e}")

            return tests

        except Exception as e:
            logger.error(f"Error processing instance {instance_id}: {e}")
            return 0  # Changed return type to int


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

    parser.add_argument(
        "--send_email",
        action="store_true",
        help="Send email to user or oncall",
    )

    parser.add_argument(
        "--create_task",
        action="store_true",
        help="Create task for user or oncall",
    )

    args = parser.parse_args()

    if args.query_scuba:
        # Query Scuba for test result
        logger.info("Querying Scuba for test results...")
        sql_query = ScubaQueryBuilder().build_query_for_test_results(
            job_name_prefix="sai_agent_known_bad_test"
        )
        df = ScubaQueryBuilder().execute_query(sql_query)

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

        # query scuba for job ids
        sql_query = ScubaQueryBuilder().build_query_for_chronos_job_id(
            "sai_agent_known_bad_test",
        )
        df = ScubaQueryBuilder().execute_query(sql_query)
        if df is None:
            return 1

        known_bad_jobs = {}
        for _i, (job_id, job_name, instance_id) in enumerate(
            zip(df["jobId"], df["jobname"], df["job_instance_id"])
        ):
            logger.info(
                f"job id: {job_id}, job name: {job_name} instance id: {instance_id}"
            )
            known_bad_jobs[job_name] = asyncio.run(
                ChronosJobExtractor().process_instance(int(instance_id), job_name)
            )

        for job_name, job_count in known_bad_jobs.items():
            logger.info(f"Job name: {job_name}, Known bad count: {job_count}")

        # Write test data to a separate JSON file
        if tests:
            with open(DEFAULT_OUTPUT_FILE, "w") as f:
                json.dump(tests, f, indent=2, default=str)
                json.dump(known_bad_jobs, f, indent=2, default=str)
            logger.info(f"Results written to {DEFAULT_OUTPUT_FILE}")

            # send email to user or oncall
            if args.send_email:
                logger.info("Sending email...")
                html = UserAndEmailHandler().HTTP_format_known_bad_list(
                    tests, known_bad_jobs
                )

                logger.info("html generated successfully for email body")
                images = {}
                return_code = asyncio.run(
                    UserAndEmailHandler().send_email(args.user, html, images)
                )

                if return_code == 0:
                    logger.error(
                        f"Error: Failed to send email to user, return_code: {return_code}"
                    )

                logger.info(f"Email sent successfully return_code: {return_code}")
            else:
                logger.info("No email sent")

                # create task for user or oncall
            if args.create_task:
                logger.info("Creating task for oncall...")
                task = asyncio.run(UserAndEmailHandler().create_task(args.user, tests))
                logger.info(f"Task created successfully: T{task.task_number}")
            else:
                logger.info("No task created")
        else:
            logger.info("No known bad tests found")

    return 0


# No code to edit in the selected snippet.

if __name__ == "__main__":
    sys.exit(main())
