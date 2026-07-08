#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import os
import subprocess
import time
from argparse import ArgumentParser

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
    Cold boots both agents once to establish a clean state, then reuses the running
    agents between tests and only cold boots again when an agent is no longer up.
    """

    _AGENT_CONFIG_PATH = "/etc/coop/agent.conf"
    _CONFIG_SNAPSHOT_PATH = "/tmp/agent.conf.fboss2_test_snapshot"

    # Grace window we allow an agent that is still coming up (systemd
    # "activating", e.g. fboss2-dev bounced it during a config commit) to reach
    # "active" before we give up and cold boot. A healthy agent passes on the
    # first poll, and a dead agent ("failed"/"inactive") is cold booted
    # immediately without waiting, so this window only applies while an agent is
    # genuinely mid-restart.
    _AGENT_READY_GRACE_RETRIES: int = 30
    _AGENT_READY_GRACE_SLEEP_SEC: int = 1
    _SYSTEMD_ACTIVE: str = "active"
    _SYSTEMD_ACTIVATING: str = "activating"

    def __init__(self) -> None:
        super().__init__()
        # Whether fboss_sw_agent and fboss_hw_agent@N are already running
        self._is_prod_multi_switch: bool = False
        self._switch_indexes: list[int] = []
        self._test_config_source: str = self._AGENT_CONFIG_PATH
        # Tracks whether the one-time initial cold boot has run. The first cold
        # boot always happens (to load the test config and a clean state); after
        # that we health-gate per-test cold boots (or skip them entirely with
        # --skip-coldboot).
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
            help="Aggressively skip all per-test cold boots after the initial "
            "one, even if an agent is down (default behavior already skips the "
            "cold boot when the agents are still up).",
        )

    def _get_config_path(self) -> str:
        return self._AGENT_CONFIG_PATH

    def _get_known_bad_tests_file(self) -> str:
        args = self.args
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
        args = self.args
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
        # The first cold boot always runs: it loads the test config and
        # establishes a clean state (in prod multi-switch the running agents
        # still hold the old config until rebooted).
        if self._initial_coldboot_done:
            # Aggressive override: skip all per-test cold boots after the first.
            if getattr(self.args, "skip_coldboot", False):
                print("########## Skipping per-test cold boot (--skip-coldboot).")
                return
            # Default: reuse the already-running agents and only cold boot when
            # an agent is down (e.g. a prior test crashed/wedged it). Most fboss2
            # integration tests self-revert their config changes, so rebooting
            # between every test is largely wasted.
            if self._agents_ready():
                print("########## Agents still up, skipping per-test cold boot.")
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

    def _agent_services(self) -> list[str]:
        """systemd unit names for the sw_agent and every hw_agent."""
        services = [SW_AGENT_SERVICE_PROD]
        services += [
            f"{HW_AGENT_SERVICE_PROD}{switch_index}"
            for switch_index in self._switch_indexes
        ]
        return services

    def _agents_ready(self) -> bool:
        """Return True only if the sw_agent and every hw_agent service is active.

        We poll only while an agent is still coming up (systemd "activating").
        A dead agent ("failed"/"inactive"/etc. -- e.g. a prior test crashed it)
        will not recover on its own, so we return immediately and let the caller
        cold boot rather than waste the grace window polling it; this is what
        makes recovery after a failed test fast.

        The sw_agent and each hw_agent are checked independently, so in
        multi-switch mode a crashed hw_agent is not masked by a healthy sw_agent.

        Note: this is a liveness (systemd active) check. The OSS agent utils do
        not expose a thrift run-state query, so unlike the netcastle path we
        cannot additionally require CONFIGURED here.
        """
        services = self._agent_services()
        for attempt in range(self._AGENT_READY_GRACE_RETRIES):
            states = self._service_states(services)
            if states == [self._SYSTEMD_ACTIVE] * len(services):
                return True
            if self._SYSTEMD_ACTIVATING not in states:
                # Nothing is coming up -> a cold boot is required now; do not
                # waste the grace window polling a dead agent.
                return False
            if attempt + 1 < self._AGENT_READY_GRACE_RETRIES:
                time.sleep(self._AGENT_READY_GRACE_SLEEP_SEC)
        return False

    def _service_states(self, services: list[str]) -> list[str]:
        """Return the `systemctl is-active` state of each service (one call).

        `systemctl is-active s1 s2 ...` prints one state per unit ("active",
        "activating", "failed", "inactive", ...). Returns an empty list on any
        error, which the caller treats as not-ready.
        """
        try:
            result = subprocess.run(
                ["systemctl", "is-active", *services],
                capture_output=True,
                text=True,
                check=False,
            )
            return [line.strip() for line in result.stdout.splitlines() if line.strip()]
        except Exception as e:
            print(f"Warning: failed to query agent service states: {e}")
            return []

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
