#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from argparse import ArgumentParser

import run_test
from fboss_test_runner.constants import OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH
from fboss_test_runner.runners.test_runner import TestRunner
from fboss_test_runner.runners.utils import run_script
from fboss_test_runner.services.fboss_agent_utils import agent_can_warm_boot_file_path

SAI_HW_KNOWN_BAD_TESTS = (
    "./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON"
)
SAI_UNSUPPORTED_TESTS = (
    "./share/sai_hw_unsupported_tests/sai_hw_unsupported_tests.materialized_JSON"
)


class SaiTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        super().add_subcommand_arguments(sub_parser)
        self._add_sai_arguments(sub_parser)
        sub_parser.add_argument(
            OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
            nargs="?",
            type=str,
            help="A file path to a platform mapping JSON file to be used.",
            default=None,
        )

    def _get_config_path(self):
        # TOOO Not available in OSS
        return ""

    def _get_known_bad_tests_file(self):
        args = run_test.args
        if not args.known_bad_tests_file:
            return SAI_HW_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
        args = run_test.args
        if not args.unsupported_tests_file:
            return SAI_UNSUPPORTED_TESTS
        return args.unsupported_tests_file

    def _get_test_binary_name(self):
        return "/opt/fboss/bin/sai_test-sai_impl"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        if sai_replayer_log_path is None:
            return []
        return [
            "--enable-replayer",
            "--enable_get_attr_log",
            "--enable_packet_log",
            "--sai-log",
            sai_replayer_log_path,
        ]

    def _get_sai_logging_flags(self):
        return ["--enable_sai_log", run_test.args.sai_logging]

    def _get_warmboot_check_file(self):
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file):
        args = run_test.args
        args_list = ["--config", conf_file, "--mgmt-if", args.mgmt_if]
        if args.platform_mapping_override_path is not None:
            args_list.extend(
                [
                    "--platform_mapping_override_path",
                    args.platform_mapping_override_path,
                ]
            )
        return args_list

    def _setup_run(self, conf_file: str) -> None:
        pass

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        args = run_test.args
        if args.setup_for_coldboot:
            run_script(args.setup_for_coldboot)

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        args = run_test.args
        if args.setup_for_warmboot:
            run_script(args.setup_for_warmboot)

    def _end_run(self):
        return

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests
