#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from argparse import ArgumentParser

import run_test
from run_test import (
    LINK_KNOWN_BAD_TESTS,
    OPT_ARG_BSP_PLATFORM_MAPPING_OVERRIDE_PATH,
    OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
    SUB_ARG_AGENT_RUN_MODE,
    SUB_ARG_AGENT_RUN_MODE_MONO,
    SUB_ARG_AGENT_RUN_MODE_MULTI,
    SUB_ARG_NUM_NPUS,
)
from runners.test_runner import TestRunner
from services.fboss_agent_utils import (
    agent_can_warm_boot_file_path,
    cleanup_hw_agent_service,
    setup_and_start_hw_agent_service,
)
from services.fsdb_service_utils import (
    cleanup_fsdb_service,
    setup_and_start_fsdb_service,
)
from services.qsfp_service_utils import (
    cleanup_qsfp_service,
    setup_and_start_qsfp_service,
)


class LinkTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        sub_parser.add_argument(
            OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
            nargs="?",
            type=str,
            help="A file path to a platform mapping JSON file to be used.",
            default=None,
        )
        sub_parser.add_argument(
            OPT_ARG_BSP_PLATFORM_MAPPING_OVERRIDE_PATH,
            nargs="?",
            type=str,
            help="A file path to a BSP platform mapping JSON file to be used.",
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
        return ""

    def _get_known_bad_tests_file(self):
        args = run_test.args
        if not args.known_bad_tests_file:
            return LINK_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        args = run_test.args
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MONO:
            return "/opt/fboss/bin/sai_mono_link_test-sai_impl"

        # Default to multi_switch mode
        return "/opt/fboss/bin/sai_multi_link_test-sai_impl"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        return []

    def _get_sai_logging_flags(self, sai_logging):
        return ["--enable_sai_log", sai_logging]

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
        arg_list = ["--config", conf_file, "--mgmt-if", args.mgmt_if]
        if args.platform_mapping_override_path is not None:
            arg_list.extend(
                [
                    "--platform_mapping_override_path",
                    args.platform_mapping_override_path,
                ]
            )

        arg_list.extend(["--fsdb_client_ssl_preferred=false"])

        return arg_list

    def _setup_run(self, conf_file: str) -> None:
        pass

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        args = run_test.args
        # Start FSDB service if not disabled
        if not args.disable_fsdb:
            setup_and_start_fsdb_service(
                fsdb_service_config_path=args.fsdb_config,
                is_warm_boot=False,
            )

        setup_and_start_qsfp_service(
            qsfp_service_config_path=args.qsfp_config,
            platform_mapping_override_path=args.platform_mapping_override_path,
            bsp_platform_mapping_override_path=args.bsp_platform_mapping_override_path,
            is_fsdb_disabled=args.disable_fsdb,
            is_warm_boot=False,
        )
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            setup_and_start_hw_agent_service(
                switch_indexes=list(range(args.num_npus)),
                fboss_agent_config_path=args.config,
                platform_mapping_override_path=args.platform_mapping_override_path,
                sai_replayer_log_path=sai_replayer_log_path,
                is_fsdb_disabled=args.disable_fsdb,
                is_warm_boot=False,
            )

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        args = run_test.args
        # Start FSDB service if not disabled
        if not args.disable_fsdb:
            setup_and_start_fsdb_service(
                fsdb_service_config_path=args.fsdb_config,
                is_warm_boot=True,
            )
        setup_and_start_qsfp_service(
            qsfp_service_config_path=args.qsfp_config,
            platform_mapping_override_path=args.platform_mapping_override_path,
            bsp_platform_mapping_override_path=args.bsp_platform_mapping_override_path,
            is_fsdb_disabled=args.disable_fsdb,
            is_warm_boot=True,
        )
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            setup_and_start_hw_agent_service(
                switch_indexes=list(range(args.num_npus)),
                fboss_agent_config_path=args.config,
                platform_mapping_override_path=args.platform_mapping_override_path,
                sai_replayer_log_path=sai_replayer_log_path,
                is_fsdb_disabled=args.disable_fsdb,
                is_warm_boot=True,
            )

    def _end_run(self):
        args = run_test.args
        cleanup_qsfp_service()
        if not args.disable_fsdb:
            cleanup_fsdb_service()
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            cleanup_hw_agent_service(list(range(args.num_npus)))

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests
