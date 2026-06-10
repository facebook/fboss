#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from argparse import ArgumentParser

from fboss_test_runner.runners.test_runner import TestRunner
from fboss_test_runner.services.fboss_agent_utils import agent_can_warm_boot_file_path


class BcmTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser) -> None:
        super().add_subcommand_arguments(sub_parser)
        self._add_sai_arguments(sub_parser)

    def _get_config_path(self) -> str:
        return "/etc/coop/bcm.conf"

    def _get_test_binary_name(self) -> str:
        return "/opt/fboss/bin/bcm_test"

    def _get_warmboot_check_file(self) -> str:
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file: str) -> list[str]:
        return []
