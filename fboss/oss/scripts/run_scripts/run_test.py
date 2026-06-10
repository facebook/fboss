#!/usr/bin/env python3
# @noautodeps
# Copyright Meta Platforms, Inc. and affiliates.

import os
import subprocess
import sys

# FBOSS Hardware Test Runner
#
# This script runs hardware tests for FBOSS. It supports multiple test types:
# - sai_agent: SAI agent hardware tests
# - sai: SAI hardware tests
# - bcm: BCM hardware tests
# - qsfp: QSFP hardware tests
# - link: Link tests
# - platform: Platform service hardware tests
# - sai_agent_scale: SAI agent scale tests
# - sai_invariant_agent: SAI agent invariant config tests
# - benchmark: Benchmark tests
# - cli: CLI tests
#
# USAGE EXAMPLES:
#
# Run all SAI agent tests:
#   ./run_test.py sai_agent --config $CONFIG
#
# Run all SAI tests:
#   ./run_test.py sai --config $CONFIG
#
# Run all QSFP tests:
#   ./run_test.py qsfp --qsfp-config $QSFP_CONFIG
#
# Run all link tests:
#   ./run_test.py link --config $CONFIG --qsfp-config $QSFP_CONFIG
#
# Run only coldboot tests:
#   ./run_test.py sai --config $CONFIG --coldboot_only
#
# Run specific tests using filter:
#   ./run_test.py sai --config $CONFIG --filter=HwVlanTest.VlanApplyConfig
#   ./run_test.py sai --config $CONFIG --filter=*Route*V6*
#
# Run tests with multiple filters and negative filter:
#   ./run_test.py sai --config $CONFIG --filter=*Vlan*:*Port*:-*Mac*:*Intf*
#
# Run tests from a filter file:
#   ./run_test.py sai_agent --config $CONFIG --filter_file=./share/hw_sanity_tests/t0_agent_hw_tests.conf
#
# Skip known bad tests:
#   ./run_test.py sai --config $CONFIG --skip-known-bad-tests $KEY
#   Example: --skip-known-bad-tests "brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk4"
#
# Filter by production features (sai_agent only):
#   ./run_test.py sai_agent --config $CONFIG --enable-production-features $ASIC
#
# List tests without running:
#   ./run_test.py sai --config $CONFIG --list_tests
#
# Enable SAI replayer logging:
#   ./run_test.py sai --config $CONFIG --sai_replayer_logging /tmp/logs
#
# Enable FBOSS logging:
#   ./run_test.py sai --config $CONFIG --fboss_logging DBG5
#
# KNOWN BAD TESTS AND UNSUPPORTED TESTS:
#
# Known bad tests and unsupported tests are maintained in JSON files indexed by test configuration key.
# Key format: vendor/coldboot-sai/warmboot-sai/asic (e.g., "brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk4")
#
# File locations:
# - SAI:           ./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON
#                  ./share/sai_hw_unsupported_tests/sai_hw_unsupported_tests.materialized_JSON
# - SAI Agent:     ./share/hw_known_bad_tests/sai_agent_known_bad_tests.materialized_JSON
#                  ./share/sai_hw_unsupported_tests/sai_agent_hw_unsupported_tests.materialized_JSON
# - QSFP: ./share/qsfp_known_bad_tests/fboss_qsfp_known_bad_tests.materialized_JSON
#         ./share/qsfp_unsupported_tests/fboss_qsfp_unsupported_tests.materialized_JSON
# - Link: ./share/link_known_bad_tests/agent_ensemble_link_known_bad_tests.materialized_JSON
#         ./share/link_known_bad_tests/agent_ensemble_link_unsupported_tests.materialized_JSON


def _load_from_file(file_path, profile=None):
    """Load list from a configuration file, skipping comment lines.

    Args:
        file_path: Path to the configuration file
        profile: Optional tag to filter the input file

    Returns:
        List of strings in the file
    """
    file_lines = []
    if os.path.exists(file_path):
        with open(file_path) as file:
            file_lines = []
            for line in file:
                stripped_line = line.strip()
                if not stripped_line or stripped_line.startswith("#"):
                    continue
                parts = stripped_line.split()
                pattern = parts[0]
                tags = parts[1:] if len(parts) > 1 else []
                if profile:
                    if profile not in tags:
                        continue
                # no --profile: include untagged lines and t-tagged lines
                elif tags and "t" not in tags:
                    continue
                file_lines.append(pattern)
    return file_lines


def run_script(script_file: str):
    if not os.path.exists(script_file):
        raise Exception(f"Script file {script_file} does not exist")
    if not os.access(script_file, os.X_OK):
        raise Exception(f"Script file {script_file} is not executable")
    subprocess.run(script_file, check=False, shell=True)


if __name__ == "__main__":
    from argparse import ArgumentParser

    from constants import (
        ALL_SIMUALTOR_ASICS_STR,
        DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND,
        OPT_ARG_COLDBOOT,
        OPT_ARG_CONFIG_FILE,
        OPT_ARG_DISABLE_FSDB,
        OPT_ARG_FBOSS_LOGGING,
        OPT_ARG_FILTER,
        OPT_ARG_FILTER_FILE,
        OPT_ARG_FRUID_PATH,
        OPT_ARG_FSDB_CONFIG_FILE,
        OPT_ARG_LIST_TESTS,
        OPT_ARG_MGT_IF,
        OPT_ARG_PROFILE,
        OPT_ARG_QSFP_CONFIG_FILE,
        OPT_ARG_SAI_LOGGING,
        OPT_ARG_SAI_REPLAYER_LOGGING,
        OPT_ARG_SETUP_CB,
        OPT_ARG_SETUP_WB,
        OPT_ARG_SIMULATOR,
        OPT_ARG_SKIP_KNOWN_BAD_TESTS,
        OPT_ARG_TEST_RUN_TIMEOUT,
        OPT_KNOWN_BAD_TESTS_FILE,
        OPT_UNSUPPORTED_TESTS_FILE,
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
    from runners.bcm_test_runner import BcmTestRunner
    from runners.benchmark_test_runner import BenchmarkTestRunner
    from runners.fboss2_integration_test_runner import Fboss2IntegrationTestRunner
    from runners.link_test_runner import LinkTestRunner
    from runners.platform_services_test_runner import PlatformServicesTestRunner
    from runners.qsfp_test_runner import QsfpTestRunner
    from runners.sai_agent_scale_test_runner import SaiAgentScaleTestRunner
    from runners.sai_agent_test_runner import SaiAgentTestRunner
    from runners.sai_invariant_agent_test_runner import SaiInvariantAgentTestRunner
    from runners.sai_test_runner import SaiTestRunner
    from runners.test_runner import TestRunner
    from setup import setup_fboss_env

    os.chdir("/opt/fboss")
    setup_fboss_env()
    TestRunner.ENV_VAR = dict(os.environ)

    ap = ArgumentParser(description="Run tests.")

    # Define common args
    ap.add_argument(
        OPT_ARG_COLDBOOT,
        action="store_true",
        default=False,
        help="Run tests without warmboot",
    )
    ap.add_argument(
        OPT_ARG_FILTER,
        type=str,
        help=(
            "only run tests that match the filter e.g. "
            + OPT_ARG_FILTER
            + "=*Route*V6*"
        ),
    )
    ap.add_argument(
        OPT_ARG_FILTER_FILE,
        type=str,
        help=(
            "only run tests that match the filters in filter file e.g. "
            + OPT_ARG_FILTER_FILE
            + "=/fboss.git/fboss/oss/hw_known_good_tests//known-good_regexes-brcm-sai-9.0_ea_dnx_odp-jericho2"
        ),
    )
    ap.add_argument(
        OPT_ARG_PROFILE,
        type=str,
        help=(
            "when used with "
            + OPT_ARG_FILTER_FILE
            + ", only include patterns tagged with this profile (e.g. t for traditional, s for scale-up). Without this flag, all patterns are included."
        ),
    )
    ap.add_argument(
        OPT_ARG_LIST_TESTS,
        action="store_true",
        default=False,
        help="Only lists the tests, do not run any test",
    )
    ap.add_argument(
        OPT_ARG_CONFIG_FILE,
        type=str,
        help=(
            "run with the specified config file e.g. "
            + OPT_ARG_CONFIG_FILE
            + "=./share/bcm-configs/WEDGE100+RSW-bcm.conf"
        ),
    )
    ap.add_argument(
        OPT_ARG_QSFP_CONFIG_FILE,
        type=str,
        help=(
            "run tests with specified qsfp config with the absolute path e.g. "
            + OPT_ARG_QSFP_CONFIG_FILE
            + "=/opt/fboss/share/qsfp_test_configs/meru800bia.materialized_JSON"
        ),
    )
    ap.add_argument(
        OPT_ARG_SAI_REPLAYER_LOGGING,
        type=str,
        help=(
            "Enable SAI Replayer logging (e.g. SAI replayer for sai tests"
            "and store the logs in the supplied directory)"
        ),
    )
    ap.add_argument(
        OPT_ARG_SKIP_KNOWN_BAD_TESTS,
        type=str,
        help=(
            "test config key to specify which known bad tests to skip. "
            "The key format is: vendor/coldboot-sai/warmboot-sai/asic. "
            "Example: "
            + OPT_ARG_SKIP_KNOWN_BAD_TESTS
            + "=brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk"
        ),
    )
    ap.add_argument(
        OPT_KNOWN_BAD_TESTS_FILE,
        type=str,
        default=None,
        help=("Specify file for storing the known bad tests. "),
    )
    ap.add_argument(
        OPT_UNSUPPORTED_TESTS_FILE,
        type=str,
        default=None,
        help=("Specify file for storing the unsupported tests. "),
    )
    ap.add_argument(
        OPT_ARG_MGT_IF,
        type=str,
        default="eth0",
        help=("Management interface (default = eth0)"),
    )

    ap.add_argument(
        OPT_ARG_FRUID_PATH,
        type=str,
        default="/var/facebook/fboss/fruid.json",
        help=(
            "Specify file for storing the fruid data. "
            "Default is /var/facebook/fboss/fruid.json"
        ),
    )
    ap.add_argument(
        OPT_ARG_SIMULATOR,
        type=str,
        help=(
            "Specify what asic simulator to use if configured. These are options"
            + ALL_SIMUALTOR_ASICS_STR
            + "Default is None, meaning physical asic is used"
        ),
    )
    ap.add_argument(
        OPT_ARG_SAI_LOGGING,
        type=str,
        default="WARN",
        help=("Enable SAI logging (Options: DEBUG|INFO|NOTICE|WARN|ERROR|CRITICAL)"),
    )
    ap.add_argument(
        OPT_ARG_FBOSS_LOGGING,
        type=str,
        default="DBG4",
        help=("Enable FBOSS logging (Options: INFO|ERR|DBG0-9)"),
    )

    ap.add_argument(
        OPT_ARG_SETUP_CB,
        type=str,
        default=None,
        help=("run script before cold boot run"),
    )
    ap.add_argument(
        OPT_ARG_SETUP_WB,
        type=str,
        default=None,
        help=("run script before warm boot run"),
    )
    ap.add_argument(
        OPT_ARG_TEST_RUN_TIMEOUT,
        type=int,
        default=DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND,
        help="Specify test run timeout in seconds",
    )
    ap.add_argument(
        "--run-on-reference-board",
        action="store_true",
        help="Modify SAI settings to run on reference board instead of real product",
        default=False,
    )

    ap.add_argument(
        OPT_ARG_DISABLE_FSDB,
        action="store_true",
        help="Disable FSDB service for link tests",
        default=False,
    )
    ap.add_argument(
        OPT_ARG_FSDB_CONFIG_FILE,
        type=str,
        help=(
            "run tests with specified fsdb config with the absolute path e.g. "
            + OPT_ARG_FSDB_CONFIG_FILE
            + "=/opt/fboss/share/fsdb_test_configs/meru800bia.materialized_JSON"
        ),
        default=None,
    )
    # Add subparsers for different test types
    subparsers = ap.add_subparsers()

    def _register_runner(cmd, help_text, runner_cls):
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

    # Parse the args
    args = ap.parse_known_args()
    args = ap.parse_args(args[1], args[0])

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

    try:
        func = args.func
    except AttributeError as e:
        raise AttributeError("too few arguments") from e

    func(args)
