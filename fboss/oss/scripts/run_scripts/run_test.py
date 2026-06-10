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

OPT_ARG_COLDBOOT = "--coldboot_only"
OPT_ARG_FILTER = "--filter"
OPT_ARG_FILTER_FILE = "--filter_file"
OPT_ARG_PROFILE = "--profile"
OPT_ARG_LIST_TESTS = "--list_tests"
OPT_ARG_CONFIG_FILE = "--config"
OPT_ARG_QSFP_CONFIG_FILE = "--qsfp-config"
OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH = "--platform_mapping_override_path"
OPT_ARG_BSP_PLATFORM_MAPPING_OVERRIDE_PATH = "--bsp_platform_mapping_override_path"
OPT_ARG_SAI_REPLAYER_LOGGING = "--sai_replayer_logging"
OPT_ARG_SKIP_KNOWN_BAD_TESTS = "--skip-known-bad-tests"
OPT_ARG_MGT_IF = "--mgmt-if"
OPT_ARG_FRUID_PATH = "--fruid-path"
OPT_ARG_SIMULATOR = "--simulator"
OPT_ARG_SAI_LOGGING = "--sai_logging"
OPT_ARG_FBOSS_LOGGING = "--fboss_logging"
OPT_ARG_PRODUCTION_FEATURES = "--production-features"
OPT_ARG_ENABLE_PRODUCTION_FEATURES = "--enable-production-features"
OPT_ARG_LIST_TESTS_FOR_FEATURE = "--list-tests-for-features"
OPT_KNOWN_BAD_TESTS_FILE = "--known-bad-tests-file"
OPT_UNSUPPORTED_TESTS_FILE = "--unsupported-tests-file"
OPT_ARG_SETUP_CB = "--setup-for-coldboot"
OPT_ARG_SETUP_WB = "--setup-for-warmboot"
OPT_ARG_TEST_RUN_TIMEOUT = "--test-run-timeout"
OPT_ARG_DISABLE_FSDB = "--disable-fsdb"
OPT_ARG_FSDB_CONFIG_FILE = "--fsdb-config"
SUB_CMD_BCM = "bcm"
SUB_CMD_SAI = "sai"
SUB_CMD_QSFP = "qsfp"
SUB_CMD_LINK = "link"
SUB_CMD_SAI_AGENT = "sai_agent"
SUB_CMD_PLATFORM = "platform"
SUB_CMD_FBOSS2_INTEGRATION = "fboss2_integration"
SUB_CMD_SAI_AGENT_SCALE = "sai_agent_scale"
SUB_CMD_SAI_INVARIANT_AGENT = "sai_invariant_agent"
SUB_CMD_BENCHMARK = "benchmark"
SUB_ARG_AGENT_RUN_MODE = "--agent-run-mode"
SUB_ARG_AGENT_RUN_MODE_MONO = "mono"
SUB_ARG_AGENT_RUN_MODE_MULTI = "multi_switch"
SUB_ARG_NUM_NPUS = "--num-npus"
SUB_ARG_TEST_TYPE = "--type"
SUB_ARG_PLATFORM_HW_TEST = "platform_hw_test"
SUB_ARG_DATA_CORRAL_HW_TEST = "data_corral_service_hw_test"
SUB_ARG_FAN_HW_TEST = "fan_service_hw_test"
SUB_ARG_FW_UTIL_HW_TEST = "fw_util_hw_test"
SUB_ARG_SENSOR_HW_TEST = "sensor_service_hw_test"
SUB_ARG_WEUTIL_HW_TEST = "weutil_hw_test"
SUB_ARG_PLATFORM_MANAGER_HW_TEST = "platform_manager_hw_test"

SAI_HW_KNOWN_BAD_TESTS = (
    "./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON"
)
QSFP_KNOWN_BAD_TESTS = (
    "./share/qsfp_known_bad_tests/fboss_qsfp_known_bad_tests.materialized_JSON"
)
QSFP_UNSUPPORTED_TESTS = (
    "./share/qsfp_unsupported_tests/fboss_qsfp_unsupported_tests.materialized_JSON"
)
LINK_KNOWN_BAD_TESTS = (
    "./share/link_known_bad_tests/fboss_link_known_bad_tests.materialized_JSON"
)
SAI_AGENT_TEST_KNOWN_BAD_TESTS = (
    "./share/hw_known_bad_tests/sai_agent_known_bad_tests.materialized_JSON"
)
FBOSS2_INTEGRATION_KNOWN_BAD_TESTS = "./share/fboss2_integration_known_bad_tests/fboss2_integration_known_bad_tests.materialized_JSON"
ASIC_PRODUCTION_FEATURES = (
    "./share/production_features/asic_production_features.materialized_JSON"
)
SAI_UNSUPPORTED_TESTS = (
    "./share/sai_hw_unsupported_tests/sai_hw_unsupported_tests.materialized_JSON"
)
SAI_AGENT_UNSUPPORTED_TESTS = (
    "./share/sai_hw_unsupported_tests/sai_agent_hw_unsupported_tests.materialized_JSON"
)
SAI_BENCH_CONFIG = "./share/hw_benchmark_tests/sai_bench.materialized_JSON"

QSFP_SERVICE_DIR = "/dev/shm/fboss/qsfp_service"
QSFP_WARMBOOT_CHECK_FILE = f"{QSFP_SERVICE_DIR}/can_warm_boot"

XGS_SIMULATOR_ASICS = ["th3", "th4", "th4_b0", "th5"]
DNX_SIMULATOR_ASICS = ["j3"]

ALL_SIMUALTOR_ASICS_STR = "|".join(XGS_SIMULATOR_ASICS + DNX_SIMULATOR_ASICS)

GTEST_NAME_PREFIX = "[ RUN      ] "
FEATURE_LIST_PREFIX = "Feature List: "

DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND = 1200


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


def setup_fboss_env() -> None:
    print("Setting fboss environment variables")

    fboss = os.getcwd()
    os.environ["FBOSS"] = fboss
    os.environ["FBOSS_BIN"] = f"{fboss}/bin"
    os.environ["FBOSS_LIB"] = f"{fboss}/lib"
    os.environ["FBOSS_LIB64"] = f"{fboss}/lib64"
    os.environ["FBOSS_KMODS"] = f"{fboss}/lib/modules"
    os.environ["FBOSS_DATA"] = f"{fboss}/share"

    if os.environ.get("PATH") is not None:
        os.environ["PATH"] = f"{os.environ['FBOSS_BIN']}:{os.environ['PATH']}"
    else:
        os.environ["PATH"] = os.environ["FBOSS_BIN"]

    if os.environ.get("LD_LIBRARY_PATH") is not None:
        os.environ["LD_LIBRARY_PATH"] = (
            f"{os.environ['FBOSS_LIB64']}:{os.environ['FBOSS_LIB']}:{os.environ['LD_LIBRARY_PATH']}"
        )
    else:
        os.environ["LD_LIBRARY_PATH"] = (
            f"{os.environ['FBOSS_LIB64']}:{os.environ['FBOSS_LIB']}"
        )


if __name__ == "__main__":
    from argparse import ArgumentParser

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

    # Add subparser for BCM tests
    bcm_test_parser = subparsers.add_parser(SUB_CMD_BCM, help="run bcm tests")
    bcm_test_parser.set_defaults(func=BcmTestRunner().run_test)

    # Add subparser for SAI tests
    sai_test_parser = subparsers.add_parser(SUB_CMD_SAI, help="run sai tests")
    sai_test_runner = SaiTestRunner()
    sai_test_parser.set_defaults(func=sai_test_runner.run_test)
    sai_test_runner.add_subcommand_arguments(sai_test_parser)

    # Add subparser for QSFP tests
    qsfp_test_parser = subparsers.add_parser(SUB_CMD_QSFP, help="run qsfp tests")
    qsfp_test_runner = QsfpTestRunner()
    qsfp_test_parser.set_defaults(func=qsfp_test_runner.run_test)
    qsfp_test_runner.add_subcommand_arguments(qsfp_test_parser)

    # Add subparser for Link tests
    link_test_parser = subparsers.add_parser(SUB_CMD_LINK, help="run link tests")
    link_test_runner = LinkTestRunner()
    link_test_parser.set_defaults(func=link_test_runner.run_test)
    link_test_runner.add_subcommand_arguments(link_test_parser)

    # Add subparser for SAI Agent tests
    sai_agent_test_parser = subparsers.add_parser(
        SUB_CMD_SAI_AGENT, help="run sai agent tests"
    )
    sai_agent_test_runner = SaiAgentTestRunner()
    sai_agent_test_parser.set_defaults(func=sai_agent_test_runner.run_test)
    sai_agent_test_runner.add_subcommand_arguments(sai_agent_test_parser)

    # Add subparser for SAI Agent Scale tests
    sai_agent_scale_test_parser = subparsers.add_parser(
        SUB_CMD_SAI_AGENT_SCALE, help="run sai agent scale tests"
    )
    sai_agent_scale_test_runner = SaiAgentScaleTestRunner()
    sai_agent_scale_test_parser.set_defaults(func=sai_agent_scale_test_runner.run_test)
    sai_agent_scale_test_runner.add_subcommand_arguments(sai_agent_scale_test_parser)

    # Add subparser for SAI Invariant Agent tests
    sai_invariant_agent_test_parser = subparsers.add_parser(
        SUB_CMD_SAI_INVARIANT_AGENT, help="run sai agent invariant config tests"
    )
    sai_invariant_agent_test_runner = SaiInvariantAgentTestRunner()
    sai_invariant_agent_test_parser.set_defaults(
        func=sai_invariant_agent_test_runner.run_test
    )
    sai_invariant_agent_test_runner.add_subcommand_arguments(
        sai_invariant_agent_test_parser
    )

    # Add subparser for platform tests
    platform_test_parser = subparsers.add_parser(
        SUB_CMD_PLATFORM, help="run platform services test"
    )
    platform_test_runner = PlatformServicesTestRunner()
    platform_test_parser.set_defaults(func=platform_test_runner.run_test)
    platform_test_runner.add_subcommand_arguments(platform_test_parser)

    # Add subparser for fboss2 integration tests
    fboss2_integration_test_parser = subparsers.add_parser(
        SUB_CMD_FBOSS2_INTEGRATION, help="run fboss2 integration tests"
    )
    fboss2_integration_test_runner = Fboss2IntegrationTestRunner()
    fboss2_integration_test_parser.set_defaults(
        func=fboss2_integration_test_runner.run_test
    )
    fboss2_integration_test_runner.add_subcommand_arguments(
        fboss2_integration_test_parser
    )

    # Add subparser for Benchmark tests
    benchmark_test_parser = subparsers.add_parser(
        SUB_CMD_BENCHMARK, help="run benchmark tests"
    )
    benchmark_test_runner = BenchmarkTestRunner()
    benchmark_test_parser.set_defaults(func=benchmark_test_runner.run_test)
    benchmark_test_runner.add_subcommand_arguments(benchmark_test_parser)

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
