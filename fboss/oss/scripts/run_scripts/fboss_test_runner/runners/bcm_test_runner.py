#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from argparse import ArgumentParser

from fboss_test_runner.runners.test_runner import TestRunner
from fboss_test_runner.services.fboss_agent_utils import agent_can_warm_boot_file_path


class BcmTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        pass

    def _get_config_path(self):
        return "/etc/coop/bcm.conf"

    def _get_known_bad_tests_file(self):
        return ""

    def _get_unsupported_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        return "/opt/fboss/bin/bcm_test"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        # TODO
        return []

    def _get_sai_logging_flags(self):
        # N/A
        return []

    def _get_warmboot_check_file(self):
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file):
        return []

    def _setup_run(self, conf_file: str) -> None:
        pass

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        return

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        return

    def _end_run(self):
        return

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests
