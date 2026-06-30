#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import os
import sys
from argparse import ArgumentParser

from fboss_test_runner.constants import (
    OPT_ARG_FILTER,
    OPT_ARG_FILTER_FILE,
    OPT_ARG_PROFILE,
    SUB_CMD_BCM,
    SUB_CMD_BENCHMARK,
    SUB_CMD_FBOSS2_INTEGRATION,
    SUB_CMD_LINK,
    SUB_CMD_PLATFORM,
    SUB_CMD_QSFP,
    SUB_CMD_SAI,
    SUB_CMD_SAI_AGENT,
    SUB_CMD_SAI_AGENT_SCALE,
    SUB_CMD_SAI_INVARIANT_AGENT,
)
from fboss_test_runner.runners.bcm_test_runner import BcmTestRunner
from fboss_test_runner.runners.benchmark_test_runner import BenchmarkTestRunner
from fboss_test_runner.runners.fboss2_integration_test_runner import (
    Fboss2IntegrationTestRunner,
)
from fboss_test_runner.runners.link_test_runner import LinkTestRunner
from fboss_test_runner.runners.platform_services_test_runner import (
    PlatformServicesTestRunner,
)
from fboss_test_runner.runners.qsfp_test_runner import QsfpTestRunner
from fboss_test_runner.runners.sai_agent_scale_test_runner import (
    SaiAgentScaleTestRunner,
)
from fboss_test_runner.runners.sai_agent_test_runner import SaiAgentTestRunner
from fboss_test_runner.runners.sai_invariant_agent_test_runner import (
    SaiInvariantAgentTestRunner,
)
from fboss_test_runner.runners.sai_test_runner import SaiTestRunner
from fboss_test_runner.runners.test_runner import TestRunner
from setup import setup_fboss_env


def main() -> None:
    os.chdir("/opt/fboss")
    setup_fboss_env()

    ap = ArgumentParser(
        description="FBOSS Hardware Test Runner",
    )
    subparsers = ap.add_subparsers(dest="command", title="test types")

    def _register_runner(
        cmd: str, help_text: str, runner_cls: type[TestRunner]
    ) -> None:
        parser = subparsers.add_parser(cmd, help=help_text)
        runner = runner_cls()
        parser.set_defaults(func=runner.run_test)
        runner.add_subcommand_arguments(parser)

    _register_runner(SUB_CMD_BCM, "run bcm tests", BcmTestRunner)
    _register_runner(SUB_CMD_SAI, "run sai tests", SaiTestRunner)
    _register_runner(SUB_CMD_QSFP, "run qsfp tests", QsfpTestRunner)
    _register_runner(SUB_CMD_LINK, "run link tests", LinkTestRunner)
    _register_runner(SUB_CMD_SAI_AGENT, "run sai agent tests", SaiAgentTestRunner)
    _register_runner(
        SUB_CMD_SAI_AGENT_SCALE, "run sai agent scale tests", SaiAgentScaleTestRunner
    )
    _register_runner(
        SUB_CMD_SAI_INVARIANT_AGENT,
        "run sai agent invariant config tests",
        SaiInvariantAgentTestRunner,
    )
    _register_runner(
        SUB_CMD_PLATFORM, "run platform services test", PlatformServicesTestRunner
    )
    _register_runner(
        SUB_CMD_FBOSS2_INTEGRATION,
        "run fboss2 integration tests",
        Fboss2IntegrationTestRunner,
    )
    _register_runner(SUB_CMD_BENCHMARK, "run benchmark tests", BenchmarkTestRunner)

    if len(sys.argv) < 2:
        ap.print_help()
        sys.exit(1)

    args = ap.parse_args()

    if ("FBOSS_BIN" not in os.environ) or ("FBOSS_LIB" not in os.environ):
        print("FBOSS environment not set. Run `source /opt/fboss/bin/setup_fboss_env'")
        sys.exit(0)

    if args.filter and args.filter_file:
        raise ValueError(
            f"Only one of the {OPT_ARG_FILTER} or {OPT_ARG_FILTER_FILE} can be specified at any time"
        )

    if args.profile and not args.filter_file:
        raise ValueError(
            f"{OPT_ARG_PROFILE} requires {OPT_ARG_FILTER_FILE} to be specified"
        )

    args.func(args)
