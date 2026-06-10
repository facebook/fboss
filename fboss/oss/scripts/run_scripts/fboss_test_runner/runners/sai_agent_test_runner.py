#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import json
import subprocess
import sys
from argparse import ArgumentParser

import run_test
from fboss_test_runner.constants import (
    OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
    SAI_AGENT_TEST_KNOWN_BAD_TESTS,
    SAI_AGENT_UNSUPPORTED_TESTS,
    SUB_ARG_AGENT_RUN_MODE,
    SUB_ARG_AGENT_RUN_MODE_MONO,
    SUB_ARG_AGENT_RUN_MODE_MULTI,
    SUB_ARG_NUM_NPUS,
)
from fboss_test_runner.runners.test_runner import TestRunner
from fboss_test_runner.runners.utils import run_script
from fboss_test_runner.services.fboss_agent_utils import (
    agent_can_warm_boot_file_path,
    cleanup_hw_agent_service,
    setup_and_start_hw_agent_service,
)

OPT_ARG_PRODUCTION_FEATURES = "--production-features"
OPT_ARG_ENABLE_PRODUCTION_FEATURES = "--enable-production-features"
OPT_ARG_LIST_TESTS_FOR_FEATURE = "--list-tests-for-features"
ASIC_PRODUCTION_FEATURES = (
    "./share/production_features/asic_production_features.materialized_JSON"
)
FEATURE_LIST_PREFIX = "Feature List: "


class SaiAgentTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        super().add_subcommand_arguments(sub_parser)
        self._add_sai_arguments(sub_parser)
        sub_parser.add_argument(
            OPT_ARG_PRODUCTION_FEATURES,
            type=str,
            help="production features json file",
            default=ASIC_PRODUCTION_FEATURES,
        )
        sub_parser.add_argument(
            OPT_ARG_ENABLE_PRODUCTION_FEATURES,
            type=str,
            metavar="ASIC",
            help="Enable filtering by production features for the specified ASIC",
            default=None,
        )
        sub_parser.add_argument(
            OPT_ARG_LIST_TESTS_FOR_FEATURE,
            type=str,
            help="Return tests whose production feature tags are all contained "
            "in the supplied comma-separated list e.g. DLB,ACL_COUNTER,SINGLE_ACL_TABLE",
            default=None,
        )
        sub_parser.add_argument(
            OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
            nargs="?",
            type=str,
            help="A file path to a platform mapping JSON file to be used.",
            default=None,
        )
        sub_parser.add_argument(
            SUB_ARG_AGENT_RUN_MODE,
            choices=[
                SUB_ARG_AGENT_RUN_MODE_MONO,
                SUB_ARG_AGENT_RUN_MODE_MULTI,
            ],
            nargs="?",
            default=SUB_ARG_AGENT_RUN_MODE_MULTI,
            help="Specify agent run mode. Default is multi_switch mode.",
        )
        sub_parser.add_argument(
            SUB_ARG_NUM_NPUS,
            choices=[1, 2],
            default=1,
            type=int,
            help="Specify number of npus to run in multi switch mode. Default is 1.",
        )

    def _get_config_path(self):
        # TOOO Not available in OSS
        return ""

    def _get_known_bad_tests_file(self):
        args = run_test.args
        if not args.known_bad_tests_file:
            return SAI_AGENT_TEST_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
        args = run_test.args
        if not args.unsupported_tests_file:
            return SAI_AGENT_UNSUPPORTED_TESTS
        return args.unsupported_tests_file

    def _get_test_binary_name(self):
        args = run_test.args
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MONO:
            return "/opt/fboss/bin/sai_agent_hw_test-sai_impl"

        # Default to multi_switch mode
        return "/opt/fboss/bin/multi_switch_agent_hw_test"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        args = run_test.args
        if sai_replayer_log_path is None:
            return []
        # Multi switch mode is using hw agent as a service, so the sai replayer logging needs to
        # be enabled in the systemd unit file instead of the test binary flags
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            return []
        return [
            "--enable-replayer",
            "--enable_get_attr_log",
            "--enable_packet_log",
            "--sai-log",
            sai_replayer_log_path,
        ]

    def _get_sai_logging_flags(self):
        args = run_test.args
        # Multi switch mode is using hw agent as a service, so the sai replayer logging needs to
        # be enabled in the systemd unit file instead of the test binary flags
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            return []
        return ["--enable_sai_log", args.sai_logging]

    def _get_warmboot_check_file(self):
        args = run_test.args
        # If it's multi_switch mode, we need to check the warmboot file for SW switch which doesn't
        # have any switch_index
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            return agent_can_warm_boot_file_path(switch_index=None)
        # If it's mono mode, we need to check the warmboot file for hw switch which has switch_index
        # as 0
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file):
        args = run_test.args
        args_list = ["--config", conf_file, "--mgmt-if", args.mgmt_if]
        # Multi switch mode is using hw agent as a service, so the override platform mapping needs
        # to be specified in the systemd unit file instead of the test binary flags
        if (
            args.platform_mapping_override_path is not None
            and args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MONO
        ):
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
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            setup_and_start_hw_agent_service(
                switch_indexes=list(range(args.num_npus)),
                fboss_agent_config_path=args.config,
                platform_mapping_override_path=args.platform_mapping_override_path,
                sai_replayer_log_path=sai_replayer_log_path,
                is_warm_boot=False,
            )

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        args = run_test.args
        if args.setup_for_warmboot:
            run_script(args.setup_for_warmboot)
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            setup_and_start_hw_agent_service(
                switch_indexes=list(range(args.num_npus)),
                fboss_agent_config_path=args.config,
                platform_mapping_override_path=args.platform_mapping_override_path,
                sai_replayer_log_path=sai_replayer_log_path,
                is_warm_boot=True,
            )

    def _end_run(self):
        args = run_test.args
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            cleanup_hw_agent_service(list(range(args.num_npus)))

    def _filter_tests(self, tests: list[str]) -> list[str]:
        args = run_test.args
        if args.list_tests_for_features:
            target_features = set(args.list_tests_for_features.split(","))
            matching_tests = []
            for test in tests:
                cmd = [
                    self._get_test_binary_name(),
                    f"--gtest_filter={test}",
                    "--list_production_feature",
                ]
                ret = subprocess.run(
                    cmd,
                    check=False,
                    capture_output=True,
                    text=True,
                )
                for line in ret.stdout.split("\n"):
                    if not line.startswith(FEATURE_LIST_PREFIX):
                        continue
                    test_feature_str = line[len(FEATURE_LIST_PREFIX) :]
                    test_features = (
                        set(test_feature_str.split(",")) if test_feature_str else set()
                    )
                    if test_features and test_features.issubset(target_features):
                        matching_tests.append(test)
                        break
            return matching_tests

        if not args.enable_production_features:
            return tests

        asic = str(args.enable_production_features)
        with open(args.production_features) as f:
            asic_production_features = json.load(f)
        asic_to_feature_names = asic_production_features["asicToFeatureNames"]

        # Check if the specified asic exists in the production features file
        if asic not in asic_to_feature_names:
            print(f"Error: ASIC '{asic}' not found in production features file.")
            sys.exit(1)

        production_features = set(asic_to_feature_names.get(asic, []))
        tests_to_run = []
        for test in tests:
            cmd = [
                self._get_test_binary_name(),
                f"--gtest_filter={test}",
                "--list_production_feature",
            ]
            ret = subprocess.run(
                cmd,
                check=False,
                capture_output=True,
                text=True,
            )
            for line in ret.stdout.split("\n"):
                if not line.startswith(FEATURE_LIST_PREFIX):
                    continue
                test_feature_str = line[len(FEATURE_LIST_PREFIX) :]
                test_features = (
                    set(test_feature_str.split(",")) if test_feature_str else set()
                )
                if "HW_SWITCH" in test_features:
                    tests_to_run += (test,)
                    break
                if test_features.issubset(production_features):
                    tests_to_run += (test,)
                    break
        return tests_to_run
