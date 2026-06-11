#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import os
import subprocess
from argparse import ArgumentParser

import run_test
from fboss_test_runner.runners.test_runner import TestRunner
from fboss_test_runner.services.fboss_agent_utils import (
    cleanup_hw_agent_service,
    cleanup_sw_agent_service,
    cold_boot_agents,
    HW_AGENT_SERVICE_PROD,
    is_agent_running,
    setup_and_start_hw_agent_service,
    setup_and_start_sw_agent_service,
    SW_AGENT_SERVICE_PROD,
)

FBOSS2_INTEGRATION_KNOWN_BAD_TESTS = "./share/fboss2_integration_known_bad_tests/fboss2_integration_known_bad_tests.materialized_JSON"


class Fboss2IntegrationTestRunner(TestRunner):
    """
    Runner for fboss2 integration tests.

    fboss2 integration tests are C++ gtest-based tests that run CLI commands and verify output.
    They test the CLI tool itself (fboss2-dev) on a running FBOSS instance.

    fboss2 integration tests are platform/SAI independent - they test the CLI binary which
    communicates with the agent via Thrift, regardless of the underlying
    hardware abstraction layer.

    Agent lifecycle: uses production service names (fboss_sw_agent, fboss_hw_agent@N)
    because fboss2-dev may restart agents during config commits. Detects whether the
    device has production multi-switch services running or needs service setup from scratch.
    Cold boots both agents before each test for isolation.
    """

    _AGENT_CONFIG_PATH = "/etc/coop/agent.conf"
    _CONFIG_SNAPSHOT_PATH = "/tmp/agent.conf.fboss2_test_snapshot"

    def __init__(self) -> None:
        super().__init__()
        # Whether fboss_sw_agent and fboss_hw_agent@N are already running
        self._is_prod_multi_switch: bool = False
        self._switch_indexes: list[int] = []
        self._test_config_source: str = self._AGENT_CONFIG_PATH
        # Tracks whether the one-time initial cold boot has run, so that
        # --skip-coldboot can still bootstrap the test config/clean state once
        # before skipping the per-test cold boots.
        self._initial_coldboot_done: bool = False

    def add_subcommand_arguments(self, sub_parser: ArgumentParser) -> None:
        """Add CLI test-specific command line arguments"""
        super().add_subcommand_arguments(sub_parser)
        sub_parser.set_defaults(fruid_path=None, coldboot_only=True)
        sub_parser.add_argument(
            "--num-npus",
            type=int,
            choices=[1, 2],
            default=1,
            help="Number of NPUs (switch indexes). Default is 1.",
        )
        sub_parser.add_argument(
            "--skip-coldboot",
            action="store_true",
            default=False,
            help="Skip per-test cold boots (one initial cold boot still runs).",
        )

    def _get_config_path(self) -> str:
        return self._AGENT_CONFIG_PATH

    def _get_known_bad_tests_file(self) -> str:
        args = run_test.args
        if args.known_bad_tests_file:
            if os.path.exists(args.known_bad_tests_file):
                print(
                    f"Using user-specified known bad tests file: {args.known_bad_tests_file}"
                )
                return args.known_bad_tests_file
            print(
                f"Warning: User-specified known bad tests file not found: {args.known_bad_tests_file}"
            )
        if os.path.exists(FBOSS2_INTEGRATION_KNOWN_BAD_TESTS):
            print(
                f"Using default known bad tests file: {FBOSS2_INTEGRATION_KNOWN_BAD_TESTS}"
            )
            return FBOSS2_INTEGRATION_KNOWN_BAD_TESTS
        print("No known bad tests file found, skipping known bad test filtering")
        return ""

    def _get_test_binary_name(self) -> str:
        return "fboss2_integration_test"

    def _get_warmboot_check_file(self) -> str:
        return ""

    def _get_test_run_args(self, conf_file: str) -> list[str]:
        return []

    def _setup_run(self, conf_file: str) -> None:
        args = run_test.args
        self._switch_indexes = list(range(args.num_npus))
        self._is_prod_multi_switch = all(
            is_agent_running(
                self._switch_indexes,
                hw_agent_service_name=HW_AGENT_SERVICE_PROD,
                sw_agent_service_name=SW_AGENT_SERVICE_PROD,
            )
        )
        self._test_config_source = conf_file

        if self._is_prod_multi_switch:
            print(
                "Production multi-switch detected — "
                f"{SW_AGENT_SERVICE_PROD} and "
                f"{HW_AGENT_SERVICE_PROD}N already running. "
                "Snapshotting agent config."
            )
            subprocess.run(
                ["cp", self._AGENT_CONFIG_PATH, self._CONFIG_SNAPSHOT_PATH],
                check=True,
            )
        else:
            print("No running agents detected — setting up agent services.")

        if conf_file != self._AGENT_CONFIG_PATH:
            print(f"Copying test config {conf_file} to {self._AGENT_CONFIG_PATH}")
            subprocess.run(["cp", conf_file, self._AGENT_CONFIG_PATH], check=True)

        if not self._is_prod_multi_switch:
            setup_and_start_hw_agent_service(
                switch_indexes=self._switch_indexes,
                fboss_agent_config_path=self._AGENT_CONFIG_PATH,
                is_warm_boot=False,
                hw_agent_service_name=HW_AGENT_SERVICE_PROD,
                hw_agent_for_testing=False,
            )
            setup_and_start_sw_agent_service(
                fboss_agent_config_path=self._AGENT_CONFIG_PATH,
                is_warm_boot=False,
                sw_agent_service_name=SW_AGENT_SERVICE_PROD,
            )

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None) -> None:
        # With --skip-coldboot, cold boot only once (to bootstrap the test
        # config and a clean state) and skip the per-test cold boots that
        # follow. Most fboss2 integration tests self-revert their config
        # changes, so rebooting between every test is largely wasted.
        args = run_test.args
        if getattr(args, "skip_coldboot", False) and self._initial_coldboot_done:
            print("########## Skipping per-test cold boot (--skip-coldboot).")
            return

        if self._test_config_source != self._AGENT_CONFIG_PATH:
            subprocess.run(
                ["cp", self._test_config_source, self._AGENT_CONFIG_PATH], check=True
            )
        cold_boot_agents(
            self._switch_indexes,
            hw_agent_service_name=HW_AGENT_SERVICE_PROD,
            sw_agent_service_name=SW_AGENT_SERVICE_PROD,
        )
        self._initial_coldboot_done = True

    def _end_run(self) -> None:
        if self._is_prod_multi_switch:
            print("Restoring original agent config and restarting agents.")
            subprocess.run(
                ["cp", self._CONFIG_SNAPSHOT_PATH, self._AGENT_CONFIG_PATH],
                check=False,
            )
            try:
                cold_boot_agents(
                    self._switch_indexes,
                    hw_agent_service_name=HW_AGENT_SERVICE_PROD,
                    sw_agent_service_name=SW_AGENT_SERVICE_PROD,
                )
            except Exception as e:
                # Broad catch: cold_boot_agents raises generic Exception;
                # cleanup must not prevent config restoration below.
                print(f"Warning: error restarting agents during cleanup: {e}")
            subprocess.run(["rm", "-f", self._CONFIG_SNAPSHOT_PATH], check=False)
        else:
            print("Cleaning up agent services.")
            cleanup_sw_agent_service(SW_AGENT_SERVICE_PROD)
            cleanup_hw_agent_service(self._switch_indexes)
