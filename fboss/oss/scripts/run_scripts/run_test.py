#!/usr/bin/env python3
# @noautodeps
# Copyright Meta Platforms, Inc. and affiliates.

import abc
import csv
import json
import os
import re
import shutil
import subprocess
import sys
import time
from argparse import ArgumentParser
from datetime import datetime
from typing import ClassVar

from fboss_agent_utils import (
    agent_can_warm_boot_file_path,
    cleanup_hw_agent_service,
    setup_and_start_hw_agent_service,
)
from fsdb_service_utils import cleanup_fsdb_service, setup_and_start_fsdb_service
from qsfp_service_utils import cleanup_qsfp_service, setup_and_start_qsfp_service

# FBOSS Hardware Test Runner
#
# This script runs hardware tests for FBOSS. It supports multiple test types:
# - sai_agent: SAI agent hardware tests
# - sai: SAI hardware tests
# - bcm: BCM hardware tests
# - qsfp: QSFP hardware tests
# - link: Link tests
# - platform: Platform service hardware tests
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
# - SAI/SAI Agent: ./share/hw_known_bad_tests/sai_*_known_bad_tests.materialized_JSON
#                  ./share/sai_hw_unsupported_tests/sai_hw_unsupported_tests.materialized_JSON
# - QSFP: ./share/qsfp_known_bad_tests/fboss_qsfp_known_bad_tests.materialized_JSON
#         ./share/qsfp_unsupported_tests/fboss_qsfp_unsupported_tests.materialized_JSON
# - Link: ./share/link_known_bad_tests/agent_ensemble_link_known_bad_tests.materialized_JSON

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
OPT_ARG_LINK_STRESS_DURATION = "--link_stress_duration"
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
ASIC_PRODUCTION_FEATURES = (
    "./share/production_features/asic_production_features.materialized_JSON"
)
SAI_UNSUPPORTED_TESTS = (
    "./share/sai_hw_unsupported_tests/sai_hw_unsupported_tests.materialized_JSON"
)

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

    # Update TestRunner.ENV_VAR to pick up the modified environment
    TestRunner.ENV_VAR = dict(os.environ)


class TestRunner(abc.ABC):
    ENV_VAR: ClassVar[dict] = dict(os.environ)
    WARMBOOT_SETUP_OPTION = "--setup-for-warmboot"
    COLDBOOT_PREFIX = "cold_boot."
    WARMBOOT_PREFIX = "warm_boot."

    _GTEST_RESULT_PATTERN = re.compile(
        r"""\[\s+(?P<status>(OK)|(FAILED)|(SKIPPED)|(TIMEOUT))\s+\]\s+
        (?P<test_case>[\w\.\/]+)\s+\((?P<duration>\d+)\s+ms\)$""",
        re.VERBOSE,
    )

    def __init__(self):
        # Test lists will be initialized in run_test() when args are available
        self._known_bad_test_regexes = None
        self._unsupported_test_regexes = None

    @abc.abstractmethod
    def _get_config_path(self):
        pass

    @abc.abstractmethod
    def _get_known_bad_tests_file(self):
        pass

    @abc.abstractmethod
    def _get_unsupported_tests_file(self):
        pass

    @abc.abstractmethod
    def _get_test_binary_name(self):
        pass

    @abc.abstractmethod
    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        pass

    @abc.abstractmethod
    def _get_sai_logging_flags(self):
        pass

    @abc.abstractmethod
    def _get_warmboot_check_file(self):
        pass

    @abc.abstractmethod
    def _get_test_run_args(self, conf_file):
        pass

    @abc.abstractmethod
    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        pass

    @abc.abstractmethod
    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        pass

    @abc.abstractmethod
    def _end_run(self):
        pass

    @abc.abstractmethod
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        pass

    @abc.abstractmethod
    def _filter_tests(self, tests: list[str]) -> list[str]:
        pass

    def _get_sai_replayer_log_path(
        self,
        test_prefix: str,
        test_name: str,
        sai_replayer_logging_dir: str | None = None,
    ) -> str | None:
        if sai_replayer_logging_dir is None:
            return None
        return os.path.join(
            sai_replayer_logging_dir,
            "replayer-log-" + test_prefix + test_name.replace("/", "-"),
        )

    def _get_test_run_cmd(self, conf_file, test_to_run, flags):
        test_binary_name = self._get_test_binary_name()
        run_cmd = [
            test_binary_name,
            "--gtest_filter=" + test_to_run,
        ]
        if args.fruid_path is not None:
            run_cmd.append("--fruid_filepath=" + args.fruid_path)
        run_cmd += self._get_test_run_args(conf_file)

        return run_cmd + flags if flags else run_cmd

    def _add_test_prefix_to_gtest_result(self, run_test_output, test_prefix):
        run_test_result = run_test_output
        line = run_test_output.decode("utf-8")
        m = re.search(r"(?:OK|FAILED).* ] ", line)
        if m is not None:
            idx = m.end()
            run_test_result = (line[:idx] + test_prefix + line[idx:]).encode("utf-8")
        return run_test_result

    def _get_test_regexes_from_file(
        self,
        file_path: str,
        test_dict_key: str,
        keys_to_try: list[str],
    ) -> list[str]:
        """
        Helper function to extract test regexes from a JSON file.

        Tries multiple key variants provided in keys_to_try list.
        Collects regexes from all matching keys.

        Args:
            file_path: Path to the JSON file containing test configurations
            test_dict_key: Key in the JSON to access the test dictionary (e.g., "known_bad_tests", "unsupported_tests")
            keys_to_try: List of keys to try in the test dictionary

        Returns:
            List of test name regexes from all matching keys
        """
        if not os.path.exists(file_path):
            print(f"Warning: Test file {file_path} does not exist")
            return []

        with open(file_path) as f:
            test_json = json.load(f)
            test_dict = test_json[test_dict_key]

            # Collect regexes from all matching keys
            test_regexes = set()
            for key in keys_to_try:
                if key in test_dict:
                    for test_struct in test_dict[key]:
                        test_regexes.add(test_struct["test_name_regex"])

            if not test_regexes:
                print(
                    f"Warning: Could not find tests for key '{keys_to_try[0]}'. "
                    f"Available keys: {list(test_dict.keys())}"
                )

            return list(test_regexes)

    def _initialize_test_lists(self, args):
        """
        Initialize known bad and unsupported test lists.
        This should be called once at the start of run_test() when args are available.
        """
        if not args.skip_known_bad_tests:
            print(
                "The --skip-known-bad-tests option is not set, therefore unsupported tests will be run."
            )
            self._known_bad_test_regexes = []
            self._unsupported_test_regexes = []
            return

        # Build list of keys to try - always try exactly two keys:
        # 1. The key as provided
        # 2. Either the key with run mode suffix stripped (if it ends with one)
        #    or the key with run mode suffix appended (if it doesn't)
        base_key = args.skip_known_bad_tests
        keys_to_try = [base_key]

        # Only try run mode suffix if the subcommand has agent_run_mode
        if hasattr(args, "agent_run_mode"):
            run_mode_suffix = f"/{args.agent_run_mode}"
            if base_key.endswith(run_mode_suffix):
                # Key already has the run mode suffix, try the stripped version
                stripped_key = base_key[: -len(run_mode_suffix)]
                keys_to_try.append(stripped_key)
            else:
                # Key doesn't have the run mode suffix, try adding it
                keys_to_try.append(f"{base_key}{run_mode_suffix}")

        # Load known bad tests
        known_bad_tests_file = self._get_known_bad_tests_file()
        self._known_bad_test_regexes = self._get_test_regexes_from_file(
            file_path=known_bad_tests_file,
            test_dict_key="known_bad_tests",
            keys_to_try=keys_to_try,
        )

        # Load unsupported tests
        unsupported_tests_file = self._get_unsupported_tests_file()
        self._unsupported_test_regexes = self._get_test_regexes_from_file(
            file_path=unsupported_tests_file,
            test_dict_key="unsupported_tests",
            keys_to_try=keys_to_try,
        )

    def _parse_list_test_output(self, output):
        ret = []
        class_name = None
        for line in output.decode("utf-8").split("\n"):
            if not line:
                continue
            # for tests that are templdated, gtest will print a comment of the
            # template. Example output:
            #
            # BcmMirrorTest/0.  # TypeParam = folly::IPAddressV4
            #   ResolvedSpanMirror
            #
            # In this case, we just need to ignore the comment (starts with '#')
            sanitized_line = line.split("#")[0].strip()
            if sanitized_line.endswith("."):
                class_name = sanitized_line[:-1]
            else:
                if not class_name:
                    raise RuntimeError("error")
                func_name = sanitized_line.strip()
                ret.append(f"{class_name}.{func_name}")

        return ret

    def _parse_gtest_run_output(self, test_output):
        test_summary = []
        for line in test_output.decode("utf-8").split("\n"):
            match = self._GTEST_RESULT_PATTERN.match(line.strip())
            if not match:
                continue
            test_summary.append(line)
        return test_summary

    def _list_tests_to_run(self, test_filter):
        output = subprocess.check_output(
            [
                self._get_test_binary_name(),
                "--gtest_list_tests",
                f"--gtest_filter={test_filter}",
            ]
        )
        return self._parse_list_test_output(output)

    def _test_matches_any_regex(self, test, regex_list):
        """
        Check if a test name matches any regex in the provided list.

        Args:
            test: Test name to check
            regex_list: List of regex patterns to match against

        Returns:
            True if test matches any regex in the list, False otherwise
        """
        return any(re.match(regex_pattern, test) for regex_pattern in regex_list)

    def _is_known_bad_test(self, test):
        """Check if a test is in the known bad tests list."""
        return self._test_matches_any_regex(test, self._known_bad_test_regexes)

    def _is_unsupported_test(self, test):
        """Check if a test is in the unsupported tests list."""
        return self._test_matches_any_regex(test, self._unsupported_test_regexes)

    def _get_tests_to_run(self):
        # Filter syntax is -
        #   --filter=<include_regexes>-<exclude_regexes>
        #   in case of multiple regexes, each one should be separated by ':'
        #
        # For example, to run all tests matching "Vlan" and "Port" but
        # excluding "Mac" tests in the list is -
        #   --filter=*Vlan*:*Port*:-*Mac*
        #
        # Also, multiple '-' is allowed but all regexes following the first '-'
        # are part of exclude list.
        #
        # There are 4 variations of test filtering:
        # 1. Tests by filter without known bad
        # 2. Tests by filter with known bad
        # 3. All tests with known bad
        # 4. All tests without known bad
        test_names = []
        if args.filter or args.filter_file:
            if args.filter_file:
                gtest_regexes = _load_from_file(args.filter_file, args.profile)
                test_names = self._list_tests_to_run(":".join(gtest_regexes))
            elif args.filter:
                test_names = self._list_tests_to_run(args.filter)
        else:
            test_names = self._list_tests_to_run("*")
        test_filter = ""
        for test_name in test_names:
            if self._is_known_bad_test(test_name) or self._is_unsupported_test(
                test_name
            ):
                continue
            test_filter += f"{test_name}:"
        if not test_filter:
            return []
        return self._list_tests_to_run(test_filter)

    def _restart_bcmsim(self, asic):
        try:
            subprocess.Popen(
                # avoid warmboot, so as to run test with coldboot init, warmboot shut down
                # as a workaround for intermittent unclean exit issue in OSS environment
                ["rm", "-f", "/dev/shm/fboss/warm_boot/can_warm_boot_0"]
            )
            subprocess.Popen(
                # command to start bcmsim service
                ["./runner.sh", "restart", "python3", "brcmsim.py", "-a", asic, "-s"]
            )
            time.sleep(60)
            print("Restarted bcmsim service", flush=True)
        except subprocess.CalledProcessError:
            print("Failed to restart bcmsim service", flush=True)

    def _run_test(
        self,
        conf_file,
        test_prefix,
        test_to_run,
        setup_warmboot,
        sai_logging,
        fboss_logging,
        sai_replayer_logging_path: str | None = None,
        test_run_timeout_in_second: int = DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND,
    ):
        # Setup flags for the test binary before running the tests
        flags = [self.WARMBOOT_SETUP_OPTION] if setup_warmboot else []
        flags += self._get_sai_replayer_logging_flags(sai_replayer_logging_path)
        flags += self._get_sai_logging_flags(sai_logging)
        flags += ["--logging", fboss_logging]

        try:
            test_run_cmd = self._get_test_run_cmd(conf_file, test_to_run, flags)
            print(
                f"Running command {test_run_cmd}",
                flush=True,
            )

            start_time = time.time()
            run_test_output = subprocess.check_output(
                test_run_cmd,
                timeout=test_run_timeout_in_second,
                env=self.ENV_VAR,
            )
            elapsed_ms = int((time.time() - start_time) * 1000)

            # Add test prefix to test name in the result
            run_test_result = self._add_test_prefix_to_gtest_result(
                run_test_output, test_prefix
            )
            # If no gtest result line found (e.g. --setup-for-warmboot
            # causes early exit), use return code to synthesize result
            if run_test_result == run_test_output:
                run_test_result = (
                    "[       OK ] "
                    + test_prefix
                    + test_to_run
                    + " ("
                    + str(elapsed_ms)
                    + " ms)"
                ).encode("utf-8")
        except subprocess.TimeoutExpired as e:
            # Test timed out, mark it as TIMEOUT
            print("Test timeout!", flush=True)
            output = e.output.decode("utf-8") if e.output else None
            print(f"Test output {output}", flush=True)
            stderr = e.stderr.decode("utf-8") if e.stderr else None
            print(f"Test error {stderr}", flush=True)
            run_test_result = (
                "[  TIMEOUT ] "
                + test_prefix
                + test_to_run
                + " ("
                + str(test_run_timeout_in_second * 1000)
                + " ms)"
            ).encode("utf-8")
        except subprocess.CalledProcessError as e:
            # Test aborted, mark it as FAILED
            print(f"Test aborted with return code {e.returncode}!", flush=True)
            output = e.output.decode("utf-8") if e.output else None
            print(f"Test output {output}", flush=True)
            stderr = e.stderr.decode("utf-8") if e.stderr else None
            print(f"Test error {stderr}", flush=True)
            run_test_result = (
                "[   FAILED ] " + test_prefix + test_to_run + " (0 ms)"
            ).encode("utf-8")
        return run_test_result

    def _string_in_file(self, file_path, string):
        try:
            with open(file_path) as file:
                file_contents = file.read()
            return string in file_contents
        except FileNotFoundError:
            print(f"File not found when replacing string: {file_path}")

    def _replace_string_in_file(self, file_path, old_str, new_str):
        try:
            with open(file_path) as file:
                file_contents = file.read()

            new_file_contents = file_contents.replace(old_str, new_str)

            if new_file_contents != file_contents:
                with open(file_path, "w") as file:
                    file.write(new_file_contents)
                print(f"Replaced {old_str} by {new_str} in file {file_path}")
        except FileNotFoundError:
            print(f"File not found when replacing string: {file_path}")
        except Exception as e:
            print(f"Error when replacing string in {file_path}: {e!s}")

    def _backup_and_modify_config(self, conf_file):
        """Create a copy of the config and modify settings"""
        if args.run_on_reference_board:
            # Create a copy of the config file for modification
            try:
                # Create a modified copy in /tmp with standard name
                config_filename = os.path.basename(conf_file)
                _config_file_modified = f"/tmp/modified-{config_filename}"
                shutil.copy2(conf_file, _config_file_modified)

                print(
                    f"Using a modified config file {_config_file_modified} for test runs"
                )
                # Some platforms, like TH5 SVK, need to set
                # AUTOLOAD_BOARD_SETTINGS=1 to autodetect reference board
                self._replace_string_in_file(
                    _config_file_modified,
                    "AUTOLOAD_BOARD_SETTINGS: 0",
                    "AUTOLOAD_BOARD_SETTINGS: 1",
                )
                return _config_file_modified
            except Exception as e:
                print(f"Error creating config copy {conf_file}: {e!s}")
                return conf_file
        return conf_file

    def _run_tests(self, tests_to_run, conf_file, args):  # noqa: PLR0915 - complex orchestration; splitting would harm readability
        if args.sai_replayer_logging:
            if os.path.isdir(args.sai_replayer_logging) or os.path.isfile(
                args.sai_replayer_logging
            ):
                raise ValueError(
                    f"File or directory {args.sai_replayer_logging} already exists."
                    "Remove or specify another directory and retry. Exitting"
                )

            os.makedirs(args.sai_replayer_logging)
        if args.simulator in XGS_SIMULATOR_ASICS:
            self.ENV_VAR["SOC_TARGET_SERVER"] = "127.0.0.1"
            self.ENV_VAR["BCM_SIM_PATH"] = "1"
            self.ENV_VAR["SOC_BOOT_FLAGS"] = "4325376"
            self.ENV_VAR["SAI_BOOT_FLAGS"] = "4325376"
            self.ENV_VAR["SOC_TARGET_PORT"] = "22222"
            self.ENV_VAR["SOC_TARGET_COUNT"] = "1"
        elif args.simulator in DNX_SIMULATOR_ASICS:
            self.ENV_VAR["BCM_SIM_PATH"] = "1"
            self.ENV_VAR["SOC_BOOT_FLAGS"] = "0x1020000"
            self.ENV_VAR["ADAPTER_DEVID_0"] = "8860"
            self.ENV_VAR["ADAPTER_REVID_0"] = "1"
            self.ENV_VAR["ADAPTER_SERVER_MODE"] = "1"
            self.ENV_VAR["CMODEL_DEVID_0"] = "8860"
            self.ENV_VAR["CMODEL_REVID_0"] = "1"
            self.ENV_VAR["CMODEL_MEMORY_PORT_0"] = "1222"
            self.ENV_VAR["CMODEL_PACKET_PORT_0"] = "6815"
            self.ENV_VAR["CMODEL_SDK_INTERFACE_PORT_0"] = "6816"
            self.ENV_VAR["CMODEL_EXTERNAL_EVENTS_PORT_0"] = "6817"
            self.ENV_VAR["cmodel_ip_address"] = "localhost"
            self.ENV_VAR["SOC_TARGET_SERVER"] = "localhost"
            self.ENV_VAR["SOC_TARGET_SERVER_0"] = "localhost"
            self.ENV_VAR["SAI_BOOT_FLAGS"] = "0x1020000"

        # Determine if tests need to be run with warmboot mode too
        warmboot = False
        if args.coldboot_only is False:
            warmboot = True

        if conf_file and not os.path.exists(conf_file):
            print(f"########## Required configuration file not found: {conf_file}")
            return []

        test_outputs = []
        num_tests = len(tests_to_run)
        for idx, test_to_run in enumerate(tests_to_run):
            test_prefix = self.COLDBOOT_PREFIX
            sai_replayer_log_path = self._get_sai_replayer_log_path(
                test_prefix, test_to_run, args.sai_replayer_logging
            )
            # Run the test for coldboot verification
            self._setup_coldboot_test(sai_replayer_log_path)
            print("########## Running test: " + test_to_run, flush=True)
            if args.simulator:
                self._restart_bcmsim(args.simulator)
            test_output = self._run_test(
                conf_file,
                test_prefix,
                test_to_run,
                warmboot,  # setup_warmboot
                args.sai_logging,
                args.fboss_logging,
                sai_replayer_log_path,
                args.test_run_timeout + args.link_stress_duration * 60,
            )
            output = test_output.decode("utf-8")
            print(
                f"########## Coldboot test results ({idx + 1}/{num_tests}): {output}",
                flush=True,
            )
            test_outputs.append(test_output)

            # Run the test again for warmboot verification if the test supports it
            if warmboot and os.path.isfile(self._get_warmboot_check_file()):
                test_prefix = self.WARMBOOT_PREFIX
                sai_replayer_log_path = self._get_sai_replayer_log_path(
                    test_prefix, test_to_run, args.sai_replayer_logging
                )
                self._setup_warmboot_test(sai_replayer_log_path)
                print(
                    "########## Verifying test with warmboot: " + test_to_run,
                    flush=True,
                )
                test_output = self._run_test(
                    conf_file,
                    test_prefix,
                    test_to_run,
                    False,  # setup_warmboot
                    args.sai_logging,
                    args.fboss_logging,
                    sai_replayer_log_path,
                    args.test_run_timeout,
                )
                output = test_output.decode("utf-8")
                print(
                    f"########## Warmboot test results ({idx + 1}/{num_tests}): {output}",
                    flush=True,
                )
                test_outputs.append(test_output)
        self._end_run()
        return test_outputs

    def _print_output_summary(self, test_outputs):
        test_summaries = []
        test_summary_count = {"OK": 0, "FAILED": 0, "SKIPPED": 0, "TIMEOUT": 0}
        for test_output in test_outputs:
            test_summaries += self._parse_gtest_run_output(test_output)
        # Print test results and update test result counts
        for test_summary in test_summaries:
            print(test_summary)
            m = re.search(r"[.*[A-Z]{2,10}", test_summary)
            if m is not None:
                test_summary_count[m.group()] += 1
        # Print test result counts
        print("Summary:")
        for test_result, value in test_summary_count.items():
            print("  ", test_result, ":", value)

        self._write_results_to_csv(test_summaries)

    def _write_results_to_csv(self, output):
        if not output:
            print("No tests we were run.")
            return
        output_csv = (
            f"hwtest_results_{datetime.now().strftime('%Y_%b_%d-%I_%M_%S_%p')}.csv"
        )

        with open(output_csv, "w") as f:
            writer = csv.writer(f)
            writer.writerow(["Test Name", "Result"])
            for line in output:
                test_result = line.split("]")[0].strip("[ ")
                test_name = line.split("]")[1].split("(")[0].strip()
                writer.writerow([test_name, test_result])

        print(f"\nTest output stored at: {output_csv}")

    def run_test(self, args):
        # Initialize test lists once at the start
        self._initialize_test_lists(args)

        if hasattr(args, "agent_run_mode"):
            _print_deprecation_banner(
                [
                    "NOTE: Default run mode is now multi_switch.",
                    "Mono mode is DEPRECATED and will be removed in a future.",
                ]
            )
        tests_to_run = self._get_tests_to_run()
        tests_to_run = self._filter_tests(tests_to_run)

        # Check if tests need to be run or only listed
        if (
            args.list_tests is False
            and getattr(args, "list_tests_for_features", None) is None
        ):
            start_time = datetime.now()
            original_conf_file = (
                args.config if (args.config is not None) else self._get_config_path()
            )
            conf_file = self._backup_and_modify_config(original_conf_file)
            output = self._run_tests(tests_to_run, conf_file, args)
            end_time = datetime.now()
            delta_time = end_time - start_time
            print(
                f"Running all tests took {delta_time} between {start_time} and {end_time}",
                flush=True,
            )
            self._print_output_summary(output)
        else:
            # Print the filtered tests
            for test in tests_to_run:
                print(test)


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

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        return

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        return

    def _end_run(self):
        return

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests


class SaiTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
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
        if not args.known_bad_tests_file:
            return SAI_HW_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
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

    def _get_sai_logging_flags(self, sai_logging):
        return ["--enable_sai_log", sai_logging]

    def _get_warmboot_check_file(self):
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file):
        args_list = ["--config", conf_file, "--mgmt-if", args.mgmt_if]
        if args.platform_mapping_override_path is not None:
            args_list.extend(
                [
                    "--platform_mapping_override_path",
                    args.platform_mapping_override_path,
                ]
            )
        return args_list

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        if args.setup_for_coldboot:
            run_script(args.setup_for_coldboot)

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        if args.setup_for_warmboot:
            run_script(args.setup_for_warmboot)

    def _end_run(self):
        return

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests


class QsfpTestRunner(TestRunner):
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

    def _get_config_path(self):
        return ""

    def _get_known_bad_tests_file(self):
        if not args.known_bad_tests_file:
            return QSFP_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
        return QSFP_UNSUPPORTED_TESTS

    def _get_test_binary_name(self):
        return "/opt/fboss/bin/qsfp_hw_test"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        return []

    def _get_sai_logging_flags(self, sai_logging):
        # N/A
        return []

    def _get_warmboot_check_file(self):
        return QSFP_WARMBOOT_CHECK_FILE

    def _get_test_run_args(self, conf_file):
        arg_list = ["--qsfp-config", args.qsfp_config]
        if args.platform_mapping_override_path is not None:
            arg_list.extend(
                [
                    "--platform_mapping_override_path",
                    args.platform_mapping_override_path,
                ]
            )
        if args.bsp_platform_mapping_override_path is not None:
            arg_list.extend(
                [
                    "--bsp_platform_mapping_override_path",
                    args.bsp_platform_mapping_override_path,
                ]
            )
        return arg_list

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        subprocess.Popen(
            # Clean up left over flags
            ["rm", "-rf", QSFP_SERVICE_DIR]
        )

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        return

    def _end_run(self):
        return

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests


_YELLOW = "\033[1;33m"
_RED = "\033[1;31m"
_RESET = "\033[0m"


def _print_deprecation_banner(lines):
    """Print a highly visible warning banner with color and box formatting."""
    width = max(len(line) for line in lines) + 4
    border = "*" * width
    print(f"\n{_RED}{border}")
    for line in lines:
        print(f"* {_YELLOW}{line.ljust(width - 4)}{_RED} *")
    print(f"{border}{_RESET}\n", flush=True)


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
        sub_parser.add_argument(
            OPT_ARG_LINK_STRESS_DURATION,
            type=int,
            default=0,
            help="Specify link stress test run duration in minutes.",
        )

    def _get_config_path(self):
        return ""

    def _get_known_bad_tests_file(self):
        if not args.known_bad_tests_file:
            return LINK_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
        return ""

    def _get_test_binary_name(self):
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
        # If it's multi_switch mode, we need to check the warmboot file for SW switch which doesn't
        # have any switch_index
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            return agent_can_warm_boot_file_path(switch_index=None)
        # If it's mono mode, we need to check the warmboot file for hw switch which has switch_index
        # as 0
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file):
        arg_list = ["--config", conf_file, "--mgmt-if", args.mgmt_if]
        if args.platform_mapping_override_path is not None:
            arg_list.extend(
                [
                    "--platform_mapping_override_path",
                    args.platform_mapping_override_path,
                ]
            )

        arg_list.extend(["--fsdb_client_ssl_preferred=false"])
        if args.link_stress_duration is not None:
            arg_list.extend(
                [
                    "--link_stress_duration",
                    str(args.link_stress_duration),
                ]
            )

        return arg_list

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
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
        cleanup_qsfp_service()
        if not args.disable_fsdb:
            cleanup_fsdb_service()
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            cleanup_hw_agent_service(list(range(args.num_npus)))

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests


class SaiAgentTestRunner(TestRunner):
    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
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
        if not args.known_bad_tests_file:
            return SAI_AGENT_TEST_KNOWN_BAD_TESTS
        return args.known_bad_tests_file

    def _get_unsupported_tests_file(self):
        if not args.unsupported_tests_file:
            return SAI_UNSUPPORTED_TESTS
        return args.unsupported_tests_file

    def _get_test_binary_name(self):
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MONO:
            return "/opt/fboss/bin/sai_agent_hw_test-sai_impl"

        # Default to multi_switch mode
        return "/opt/fboss/bin/multi_switch_agent_hw_test"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
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

    def _get_sai_logging_flags(self, sai_logging):
        # Multi switch mode is using hw agent as a service, so the sai replayer logging needs to
        # be enabled in the systemd unit file instead of the test binary flags
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            return []
        return ["--enable_sai_log", sai_logging]

    def _get_warmboot_check_file(self):
        # If it's multi_switch mode, we need to check the warmboot file for SW switch which doesn't
        # have any switch_index
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            return agent_can_warm_boot_file_path(switch_index=None)
        # If it's mono mode, we need to check the warmboot file for hw switch which has switch_index
        # as 0
        return agent_can_warm_boot_file_path(switch_index=0)

    def _get_test_run_args(self, conf_file):
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

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
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
        if args.setup_for_coldboot:
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
        if args.agent_run_mode == SUB_ARG_AGENT_RUN_MODE_MULTI:
            cleanup_hw_agent_service(list(range(args.num_npus)))

    def _filter_tests(self, tests: list[str]) -> list[str]:
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
                args.sai_logging,
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


class Fboss2IntegrationTestRunner(TestRunner):
    """
    Runner for fboss2 integration tests.

    fboss2 integration tests are C++ gtest-based tests that run CLI commands and verify output.
    They test the CLI tool itself (fboss2-dev) on a running FBOSS instance.

    fboss2 integration tests are platform/SAI independent - they test the CLI binary which
    communicates with the agent via Thrift, regardless of the underlying
    hardware abstraction layer.
    """

    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        """Add CLI test-specific command line arguments"""
        # Override defaults for CLI tests:
        # - fruid_path: CLI tests don't use fruid files
        # - coldboot_only: Some CLI tests use warmboot/coldboot but the test binary doesn't support the --setup-for-warmboot flag.
        sub_parser.set_defaults(fruid_path=None, coldboot_only=True)

    def _get_config_path(self):
        return "/etc/coop/agent.conf"

    def _get_known_bad_tests_file(self):
        return ""

    def _get_unsupported_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        return "fboss2_integration_test"

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        return []

    def _get_sai_logging_flags(self, sai_logging):
        # CLI tests don't use SAI logging
        return []

    def _get_warmboot_check_file(self):
        return ""

    def _get_test_run_args(self, conf_file):
        # CLI tests don't need any additional args
        return []

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None):
        pass

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None):
        pass

    def _end_run(self):
        pass

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests


class BenchmarkTestRunner:
    """
    Runner for benchmark test binaries.

    Unlike gtest-based test runners, benchmark tests are standalone performance
    measurement binaries that output metrics like throughput, latency, and speed.
    """

    # Benchmark test suite configuration file paths
    BENCHMARK_CONFIG_DIR = "./share/hw_benchmark_tests"
    T1_BENCHMARKS_CONF = os.path.join(BENCHMARK_CONFIG_DIR, "t1_benchmarks.conf")
    T2_BENCHMARKS_CONF = os.path.join(BENCHMARK_CONFIG_DIR, "t2_benchmarks.conf")
    ADDITIONAL_BENCHMARKS_CONF = os.path.join(
        BENCHMARK_CONFIG_DIR, "additional_benchmarks.conf"
    )
    BENCHMARK_BIN_DIR = "/opt/fboss/bin"

    def add_subcommand_arguments(self, sub_parser: ArgumentParser):
        """Add benchmark-specific command line arguments"""
        sub_parser.add_argument(
            OPT_ARG_FILTER_FILE,
            type=str,
            help=("File containing list of benchmark binaries to run (one per line)."),
            default=None,
        )
        sub_parser.add_argument(
            OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH,
            nargs="?",
            type=str,
            help="A file path to a platform mapping JSON file to be used.",
            default=None,
        )

    def _parse_benchmark_output(self, binary_name, stdout):
        """Parse benchmark output to extract metrics.

        Returns a dict with:
        - benchmark_binary_name: str
        - benchmark_test_name: str
        - test_status: str (OK, FAILED, or TIMEOUT)
        - relative_time_per_iter: str (includes time unit)
        - iters_per_sec: str (includes possible numeric suffix)
        - cpu_time_usec: str
        - max_rss: str
        """
        result = {
            "benchmark_binary_name": binary_name,
            "benchmark_test_name": "",
            "test_status": "FAILED",
            "relative_time_per_iter": "",
            "iters_per_sec": "",
            "cpu_time_usec": "",
            "max_rss": "",
        }

        # Look for the benchmark name line (e.g., "RibResolutionBenchmark                                       1.46s   684.78m")
        # Pattern: benchmark name followed by time and rate
        benchmark_line_pattern = (
            r"^([A-Za-z0-9_]+)\s+(\d+\.?\d*[a-z]?s)\s+(\d+\.?\d*[a-z]?)$"
        )

        # Look for cpu_time_usec and max_rss in JSON output (separate patterns so order doesn't matter)
        cpu_time_pattern = r'"cpu_time_usec":\s*(\d+)'
        max_rss_pattern = r'"max_rss":\s*(\d+)'

        found_benchmark_line = False
        found_json = False

        # Parse multiline benchmark result line
        match = re.search(benchmark_line_pattern, stdout, re.MULTILINE)
        if match:
            result["benchmark_test_name"] = match.group(1)
            result["relative_time_per_iter"] = match.group(2)
            result["iters_per_sec"] = match.group(3)
            found_benchmark_line = True

        # Parse JSON output for cpu_time_usec and max_rss independently
        cpu_match = re.search(cpu_time_pattern, stdout)
        rss_match = re.search(max_rss_pattern, stdout)
        if cpu_match and rss_match:
            result["cpu_time_usec"] = cpu_match.group(1)
            result["max_rss"] = rss_match.group(1)
            found_json = True

        # Only mark as OK if we found both the benchmark line and JSON
        if found_benchmark_line and found_json:
            result["test_status"] = "OK"

        return result

    def _run_benchmark_binary(self, binary_name, args):
        """Run a single benchmark binary and return parsed results"""
        print(f"########## Running benchmark binary: {binary_name}", flush=True)

        # Build command to run the benchmark
        run_cmd = [binary_name]

        # Add config and other args if provided
        if args.config:
            run_cmd.extend(["--config", args.config, "--mgmt-if", args.mgmt_if])
        if args.platform_mapping_override_path is not None:
            run_cmd.extend(
                [
                    "--platform_mapping_override_path",
                    args.platform_mapping_override_path,
                ]
            )
        if args.fruid_path is not None:
            run_cmd.extend(["--fruid_filepath=" + args.fruid_path])

        # Add logging flags
        run_cmd.extend(["--enable_sai_log", args.sai_logging])
        run_cmd.extend(["--logging", args.fboss_logging])

        print(f"Running command: {' '.join(run_cmd)}", flush=True)

        try:
            # Run the benchmark binary
            result = subprocess.run(
                run_cmd,
                check=False,
                timeout=args.test_run_timeout,
                capture_output=True,
                text=True,
            )

            print(f"########## Benchmark output for {binary_name}:")
            print(result.stdout)
            if result.stderr:
                print(f"########## Benchmark stderr for {binary_name}:")
                print(result.stderr)

            if result.returncode != 0:
                print(
                    f"########## Benchmark {binary_name} failed with return code {result.returncode}"
                )
                # Parse output even on failure to get partial results
            else:
                print(f"########## Benchmark {binary_name} completed")
            return self._parse_benchmark_output(binary_name, result.stdout)

        except subprocess.TimeoutExpired:
            print(
                f"########## Benchmark {binary_name} timed out after {args.test_run_timeout} seconds"
            )
            # Return timed out result with no metrics
            return {
                "benchmark_binary_name": binary_name,
                "benchmark_test_name": "",
                "test_status": "TIMEOUT",
                "relative_time_per_iter": "",
                "iters_per_sec": "",
                "cpu_time_usec": "",
                "max_rss": "",
            }
        except Exception as e:
            print(f"########## Error running benchmark {binary_name}: {e!s}")
            # Return failed result with no metrics
            return {
                "benchmark_binary_name": binary_name,
                "benchmark_test_name": "",
                "test_status": "FAILED",
                "relative_time_per_iter": "",
                "iters_per_sec": "",
                "cpu_time_usec": "",
                "max_rss": "",
            }

    def _get_benchmarks_to_run(self, filter_file=None):
        """Get list of benchmarks to run based on filter_file or default config.

        Args:
            filter_file: Optional path to file containing list of benchmarks.
                        If None, loads from T1, T2, and additional benchmark configs

        Returns:
            List of benchmark names to run, or None if no benchmarks found
        """
        benchmarks_to_run = []

        if filter_file:
            # User specified a custom filter file
            if not os.path.exists(filter_file):
                print(f"Error: Benchmark configuration file not found: {filter_file}")
                return None
            benchmarks_to_run = _load_from_file(filter_file)
        else:
            # Default: concatenate T1, T2, and additional benchmarks
            for conf_file in [
                self.T1_BENCHMARKS_CONF,
                self.T2_BENCHMARKS_CONF,
                self.ADDITIONAL_BENCHMARKS_CONF,
            ]:
                if os.path.exists(conf_file):
                    benchmarks_from_file = _load_from_file(conf_file)
                    benchmarks_to_run.extend(benchmarks_from_file)
                else:
                    print(f"  Warning: Configuration file not found: {conf_file}")

        if not benchmarks_to_run:
            print("Error: No benchmarks found in configuration files")
            return None

        return benchmarks_to_run

    def run_test(self, args):  # noqa: PLR0912 - complex benchmark orchestration; splitting would harm readability
        """Run benchmark test binaries"""
        benchmarks_to_run = self._get_benchmarks_to_run(args.filter_file)

        if benchmarks_to_run is None:
            return

        # If --list_tests is specified, just list the benchmarks and exit
        if args.list_tests:
            for benchmark in benchmarks_to_run:
                print(benchmark)
            return

        print(f"Total benchmarks to run: {len(benchmarks_to_run)}")

        # Filter out binaries that don't exist
        existing_benchmarks = []
        missing_benchmarks = []
        for benchmark in benchmarks_to_run:
            # Construct full path to binary
            binary_path = os.path.join(self.BENCHMARK_BIN_DIR, benchmark)
            if os.path.exists(binary_path) and os.path.isfile(binary_path):
                existing_benchmarks.append(binary_path)
            else:
                missing_benchmarks.append(benchmark)

        if missing_benchmarks:
            print(
                f"\nWarning: {len(missing_benchmarks)} benchmark binaries not found in {self.BENCHMARK_BIN_DIR}:"
            )
            for benchmark in missing_benchmarks:
                print(f"  - {benchmark}")

        if not existing_benchmarks:
            print(f"\nError: No benchmark binaries found in {self.BENCHMARK_BIN_DIR}.")
            print(
                f"Make sure you have built the benchmarks with BENCHMARK_INSTALL=1 and copied them to {self.BENCHMARK_BIN_DIR} directory."
            )
            return

        print(f"\nFound {len(existing_benchmarks)} benchmark binaries to run")

        # Run each benchmark and collect detailed results
        results = []
        for benchmark_path in existing_benchmarks:
            benchmark_result = self._run_benchmark_binary(benchmark_path, args)
            results.append(benchmark_result)

        # Write results to CSV file
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"benchmark_results_{timestamp}.csv"

        with open(csv_filename, "w", newline="") as csvfile:
            fieldnames = [
                "benchmark_binary_name",
                "benchmark_test_name",
                "test_status",
                "relative_time_per_iter",
                "iters_per_sec",
                "cpu_time_usec",
                "max_rss",
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

            writer.writeheader()
            for result in results:
                writer.writerow(result)

        print(f"\n########## Benchmark results written to: {csv_filename}")

        # Print summary
        print("\n" + "=" * 80)
        print("BENCHMARK RESULTS SUMMARY")
        print("=" * 80)
        for result in results:
            print(f"{result['benchmark_binary_name']}: {result['test_status']}")
        print("=" * 80)

        # Count results
        ok = sum(1 for r in results if r["test_status"] == "OK")
        failed = sum(1 for r in results if r["test_status"] == "FAILED")
        timed_out = sum(1 for r in results if r["test_status"] == "TIMEOUT")
        print(f"\nTotal: {len(results)} benchmarks")
        print(f"OK: {ok}")
        print(f"Failed: {failed}")
        print(f"Timed Out: {timed_out}")

        # Exit with error if any benchmarks failed or timed out
        if failed > 0 or timed_out > 0:
            sys.exit(1)


if __name__ == "__main__":
    os.chdir("/opt/fboss")
    # Set env variables for FBOSS
    setup_fboss_env()

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
