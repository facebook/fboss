#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from argparse import ArgumentParser
from datetime import datetime
from typing import ClassVar

import run_test
from fboss_test_runner.runners.test_runner import TestRunner

SUB_ARG_TEST_TYPE = "--type"
SUB_ARG_PLATFORM_HW_TEST = "platform_hw_test"
SUB_ARG_DATA_CORRAL_HW_TEST = "data_corral_service_hw_test"
SUB_ARG_FAN_HW_TEST = "fan_service_hw_test"
SUB_ARG_FW_UTIL_HW_TEST = "fw_util_hw_test"
SUB_ARG_SENSOR_HW_TEST = "sensor_service_hw_test"
SUB_ARG_WEUTIL_HW_TEST = "weutil_hw_test"
SUB_ARG_PLATFORM_MANAGER_HW_TEST = "platform_manager_hw_test"


class PlatformServicesTestRunner(TestRunner):
    TEST_TYPE_CHOICES: ClassVar[list] = [
        SUB_ARG_PLATFORM_HW_TEST,
        SUB_ARG_DATA_CORRAL_HW_TEST,
        SUB_ARG_FAN_HW_TEST,
        SUB_ARG_FW_UTIL_HW_TEST,
        SUB_ARG_SENSOR_HW_TEST,
        SUB_ARG_WEUTIL_HW_TEST,
        SUB_ARG_PLATFORM_MANAGER_HW_TEST,
    ]

    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        super().add_subcommand_arguments(sub_parser)
        sub_parser.add_argument(
            SUB_ARG_TEST_TYPE,
            choices=self.TEST_TYPE_CHOICES,
            nargs="?",
            default=None,
            help="Specify test type for platform services test.",
        )

    def _get_config_path(self):
        return ""

    def _get_known_bad_tests_file(self):
        return ""

    def _get_unsupported_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        args = run_test.args
        binary_map = {
            SUB_ARG_PLATFORM_HW_TEST: "platform_hw_test",
            SUB_ARG_DATA_CORRAL_HW_TEST: "data_corral_service_hw_test",
            SUB_ARG_FAN_HW_TEST: "fan_service_hw_test",
            SUB_ARG_FW_UTIL_HW_TEST: "fw_util_hw_test",
            SUB_ARG_SENSOR_HW_TEST: "sensor_service_hw_test",
            SUB_ARG_WEUTIL_HW_TEST: "weutil_hw_test",
            SUB_ARG_PLATFORM_MANAGER_HW_TEST: "platform_manager_hw_test",
        }

        return binary_map.get(args.type, "platform_hw_test")

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        return []

    def _get_sai_logging_flags(self, sai_logging):
        return []

    def _get_warmboot_check_file(self):
        return ""

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

    def _run_tests(self, tests_to_run, conf_file, args):
        test_binary_name = self._get_test_binary_name()
        test_outputs = []
        num_tests = len(tests_to_run)
        for idx, test_to_run in enumerate(tests_to_run):
            test_prefix = test_binary_name + "."
            print("########## Running test: " + test_to_run, flush=True)
            test_output = self._run_test(
                conf_file,
                test_prefix,
                test_to_run,
                False,  # setup_warmboot
                "WARN",  # default sai log level
                args.fboss_logging,
                None,
                args.test_run_timeout,
            )
            output = test_output.decode("utf-8")
            print(
                f"test results ({idx + 1}/{num_tests}): {output}",
                flush=True,
            )
            test_outputs.append(test_output)

        self._end_run()
        return test_outputs

    def run_test(self, args):
        args.fruid_path = None

        if args.type is not None:
            super().run_test(args)
            return

        # Initialize test lists once at the start
        self._initialize_test_lists(args)

        # Initialize test lists once at the start
        self._initialize_test_lists(args)

        output = []
        start_time = datetime.now()

        for test_type in self.TEST_TYPE_CHOICES:
            args.type = test_type
            tests_to_run = self._get_tests_to_run()
            tests_to_run = self._filter_tests(tests_to_run)

            # Check if tests need to be run or only listed
            if args.list_tests is False:
                original_conf_file = (
                    args.config
                    if (args.config is not None)
                    else self._get_config_path()
                )
                conf_file = self._backup_and_modify_config(original_conf_file)
                output.extend(self._run_tests(tests_to_run, conf_file, args))
            else:
                for test in tests_to_run:
                    print(test)
                return

        end_time = datetime.now()
        delta_time = end_time - start_time
        print(
            f"Running all tests took {delta_time} between {start_time} and {end_time}",
            flush=True,
        )
        self._print_output_summary(output)
