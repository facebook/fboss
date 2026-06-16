#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import abc
import json
import os
import re
import shutil
import subprocess
import time
from argparse import ArgumentParser, Namespace
from datetime import datetime

import run_test
from fboss_test_runner.constants import (
    ALL_SIMUALTOR_ASICS_STR,
    DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND,
    DNX_SIMULATOR_ASICS,
    DNX_SIMULATOR_ENV,
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
    OPT_ARG_NUM_WARMBOOT_ITERATIONS,
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
    XGS_SIMULATOR_ASICS,
    XGS_SIMULATOR_ENV,
)
from fboss_test_runner.reporters.console_reporter import ConsoleReporter
from fboss_test_runner.reporters.csv_reporter import CsvReporter
from fboss_test_runner.result_types import GtestResult, GtestStatus, RunOutcome
from fboss_test_runner.runners.utils import load_from_file

_YELLOW = "\033[1;33m"
_RED = "\033[1;31m"
_RESET = "\033[0m"


def _print_deprecation_banner(lines: list[str]) -> None:
    """Print a highly visible warning banner with color and box formatting."""
    width = max(len(line) for line in lines) + 4
    border = "*" * width
    print(f"\n{_RED}{border}")
    for line in lines:
        print(f"* {_YELLOW}{line.ljust(width - 4)}{_RED} *")
    print(f"{border}{_RESET}\n", flush=True)


class TestRunner(abc.ABC):
    WARMBOOT_SETUP_OPTION = "--setup-for-warmboot"
    COLDBOOT_PREFIX = "cold_boot."
    WARMBOOT_PREFIX = "warm_boot."

    def __init__(self) -> None:
        self._known_bad_test_regexes: list[str] | None = None
        self._unsupported_test_regexes: list[str] | None = None
        self.env_var: dict[str, str] = dict(os.environ)

    def _get_common_gflags(self) -> list[str]:
        """
        Return pass-through gflags appended to every test binary invocation.
        No parsing or validation - just forward whatever the user provides.
        """
        args = run_test.args
        if hasattr(args, "extra_gflags") and args.extra_gflags:
            return args.extra_gflags.split()
        return []

    def _get_config_path(self) -> str:
        return ""

    def _get_known_bad_tests_file(self) -> str:
        return ""

    def _get_unsupported_tests_file(self) -> str:
        return ""

    @abc.abstractmethod
    def _get_test_binary_name(self) -> str:
        pass

    def _get_sai_replayer_logging_flags(
        self, sai_replayer_log_path: str | None
    ) -> list[str]:
        return []

    def _get_sai_logging_flags(self) -> list[str]:
        return []

    @abc.abstractmethod
    def _get_warmboot_check_file(self) -> str:
        pass

    @abc.abstractmethod
    def _get_test_run_args(self, conf_file: str) -> list[str]:
        pass

    def _setup_run(self, conf_file: str) -> None:  # noqa: B027
        pass

    def _setup_coldboot_test(self, sai_replayer_log_path: str | None = None) -> None:  # noqa: B027
        pass

    def _setup_warmboot_test(self, sai_replayer_log_path: str | None = None) -> None:  # noqa: B027
        pass

    def _end_run(self) -> None:  # noqa: B027
        pass

    def add_subcommand_arguments(self, sub_parser: ArgumentParser) -> None:
        sub_parser.add_argument(
            OPT_ARG_COLDBOOT,
            action="store_true",
            default=False,
            help="Run tests without warmboot",
        )
        sub_parser.add_argument(
            OPT_ARG_FILTER,
            type=str,
            help="only run tests that match the filter e.g. --filter=*Route*V6*",
        )
        sub_parser.add_argument(
            OPT_ARG_FILTER_FILE,
            type=str,
            help="only run tests that match the filters in filter file",
        )
        sub_parser.add_argument(
            OPT_ARG_PROFILE,
            type=str,
            help="only include patterns tagged with this profile (requires --filter_file)",
        )
        sub_parser.add_argument(
            OPT_ARG_LIST_TESTS,
            action="store_true",
            default=False,
            help="Only lists the tests, do not run any test",
        )
        sub_parser.add_argument(
            OPT_ARG_CONFIG_FILE,
            type=str,
            help="run with the specified config file",
        )
        sub_parser.add_argument(
            OPT_ARG_SKIP_KNOWN_BAD_TESTS,
            type=str,
            help="test config key for known bad tests. Format: vendor/coldboot-sai/warmboot-sai/asic",
        )
        sub_parser.add_argument(
            OPT_KNOWN_BAD_TESTS_FILE,
            type=str,
            default=None,
            help="Specify file for storing the known bad tests",
        )
        sub_parser.add_argument(
            OPT_UNSUPPORTED_TESTS_FILE,
            type=str,
            default=None,
            help="Specify file for storing the unsupported tests",
        )
        sub_parser.add_argument(
            OPT_ARG_MGT_IF,
            type=str,
            default="eth0",
            help="Management interface (default = eth0)",
        )
        sub_parser.add_argument(
            OPT_ARG_FRUID_PATH,
            type=str,
            default="/var/facebook/fboss/fruid.json",
            help="Specify file for storing the fruid data",
        )
        sub_parser.add_argument(
            OPT_ARG_FBOSS_LOGGING,
            type=str,
            default="DBG4",
            help="Enable FBOSS logging (Options: INFO|ERR|DBG0-9)",
        )
        sub_parser.add_argument(
            OPT_ARG_TEST_RUN_TIMEOUT,
            type=int,
            default=DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND,
            help="Specify test run timeout in seconds",
        )
        sub_parser.add_argument(
            OPT_ARG_NUM_WARMBOOT_ITERATIONS,
            type=int,
            default=1,
            help=(
                "Number of consecutive warmboot iterations to run after coldboot "
                "(default 1). Use with --filter to soak a single test across many "
                "warmboots, e.g. --filter AgentEmptyTest.CheckInit "
                "--num-warmboot-iterations 64. Ignored when --coldboot_only is set."
            ),
        )
        # --- Pass-through gflags for test binaries ---
        sub_parser.add_argument(
            "--extra-gflags",
            type=str,
            default=None,
            help=(
                "Pass-through flags appended to every test binary invocation. "
                "Use = to attach value (required when value starts with --). "
                "Example: --extra-gflags='--asic_feature_support_overrides=73=false --v=2'"
            ),
        )

    @staticmethod
    def _add_sai_arguments(sub_parser: ArgumentParser) -> None:
        sub_parser.add_argument(
            OPT_ARG_SAI_LOGGING,
            type=str,
            default="WARN",
            help="Enable SAI logging (Options: DEBUG|INFO|NOTICE|WARN|ERROR|CRITICAL)",
        )
        sub_parser.add_argument(
            OPT_ARG_SAI_REPLAYER_LOGGING,
            type=str,
            help="Enable SAI Replayer logging and store logs in the supplied directory",
        )
        sub_parser.add_argument(
            OPT_ARG_SIMULATOR,
            type=str,
            help="Specify ASIC simulator. Options: " + ALL_SIMUALTOR_ASICS_STR,
        )
        sub_parser.add_argument(
            "--run-on-reference-board",
            action="store_true",
            help="Modify SAI settings to run on reference board instead of real product",
            default=False,
        )
        sub_parser.add_argument(
            OPT_ARG_SETUP_CB,
            type=str,
            default=None,
            help="run script before cold boot run",
        )
        sub_parser.add_argument(
            OPT_ARG_SETUP_WB,
            type=str,
            default=None,
            help="run script before warm boot run",
        )

    @staticmethod
    def _add_service_arguments(sub_parser: ArgumentParser) -> None:
        sub_parser.add_argument(
            OPT_ARG_QSFP_CONFIG_FILE,
            type=str,
            help="run tests with specified qsfp config (absolute path)",
        )
        sub_parser.add_argument(
            OPT_ARG_DISABLE_FSDB,
            action="store_true",
            help="Disable FSDB service for link tests",
            default=False,
        )
        sub_parser.add_argument(
            OPT_ARG_FSDB_CONFIG_FILE,
            type=str,
            help="run tests with specified fsdb config (absolute path)",
            default=None,
        )

    def _filter_tests(self, tests: list[str]) -> list[str]:
        return tests

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

    def _get_test_run_cmd(
        self, conf_file: str, test_to_run: str, flags: list[str]
    ) -> list[str]:
        args = run_test.args
        test_binary_name = self._get_test_binary_name()
        run_cmd = [
            test_binary_name,
            "--gtest_filter=" + test_to_run,
        ]
        if args.fruid_path is not None:
            run_cmd.append("--fruid_filepath=" + args.fruid_path)
        run_cmd += self._get_test_run_args(conf_file)
        run_cmd += self._get_common_gflags()

        return run_cmd + flags if flags else run_cmd

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

    def _initialize_test_lists(self, args: Namespace) -> None:
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

    def _parse_list_test_output(self, output: bytes) -> list[str]:
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

    def _parse_gtest_run_output(self, test_output: bytes) -> list[GtestResult]:
        return GtestResult.parse_output(test_output)

    def _list_tests_to_run(self, test_filter: str) -> list[str]:
        output = subprocess.check_output(
            [
                self._get_test_binary_name(),
                "--gtest_list_tests",
                f"--gtest_filter={test_filter}",
            ]
        )
        return self._parse_list_test_output(output)

    def _test_matches_any_regex(self, test: str, regex_list: list[str]) -> bool:
        """
        Check if a test name matches any regex in the provided list.

        Args:
            test: Test name to check
            regex_list: List of regex patterns to match against

        Returns:
            True if test matches any regex in the list, False otherwise
        """
        return any(re.match(regex_pattern, test) for regex_pattern in regex_list)

    def _is_known_bad_test(self, test: str) -> bool:
        """Check if a test is in the known bad tests list."""
        return self._test_matches_any_regex(test, self._known_bad_test_regexes)

    def _is_unsupported_test(self, test: str) -> bool:
        """Check if a test is in the unsupported tests list."""
        return self._test_matches_any_regex(test, self._unsupported_test_regexes)

    def _get_tests_to_run(self) -> list[str]:
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
        args = run_test.args
        test_names = []
        if args.filter or args.filter_file:
            if args.filter_file:
                gtest_regexes = load_from_file(args.filter_file, args.profile)
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
                print(f"  >> SKIPPING (known bad/unsupported): {test_name}")
                continue
            test_filter += f"{test_name}:"
        if not test_filter:
            return []
        return self._list_tests_to_run(test_filter)

    def _restart_bcmsim(self, asic: str) -> None:
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
        conf_file: str,
        test_prefix: str,
        test_to_run: str,
        setup_warmboot: bool,
        sai_replayer_logging_path: str | None = None,
    ) -> RunOutcome:
        args = run_test.args
        # Setup flags for the test binary before running the tests
        flags = [self.WARMBOOT_SETUP_OPTION] if setup_warmboot else []
        flags += self._get_sai_replayer_logging_flags(sai_replayer_logging_path)
        flags += self._get_sai_logging_flags()
        flags += ["--logging", args.fboss_logging]

        try:
            test_run_cmd = self._get_test_run_cmd(conf_file, test_to_run, flags)
            print(
                f"Running command {test_run_cmd}",
                flush=True,
            )

            start_time = time.time()
            run_test_output = subprocess.check_output(
                test_run_cmd,
                timeout=args.test_run_timeout,
                env=self.env_var,
            )
            elapsed_ms = int((time.time() - start_time) * 1000)

            # Parse the real gtest output and tag each result with the
            # boot-phase prefix (cold_boot./warm_boot.).
            results = self._parse_gtest_run_output(run_test_output)
            for result in results:
                result.test_name = test_prefix + result.test_name
            if not results:
                # No gtest result line found (e.g. --setup-for-warmboot causes
                # an early exit); synthesize an OK so the test still appears in
                # the summary.
                synthesized = GtestResult(
                    test_prefix + test_to_run, GtestStatus.OK, elapsed_ms
                )
                return RunOutcome(synthesized.as_log_line(), [synthesized])
            return RunOutcome(run_test_output.decode("utf-8"), results)
        except subprocess.TimeoutExpired as e:
            # Test timed out, mark it as TIMEOUT
            print("Test timeout!", flush=True)
            output = e.output.decode("utf-8") if e.output else None
            print(f"Test output {output}", flush=True)
            stderr = e.stderr.decode("utf-8") if e.stderr else None
            print(f"Test error {stderr}", flush=True)
            result = GtestResult(
                test_prefix + test_to_run,
                GtestStatus.TIMEOUT,
                args.test_run_timeout * 1000,
            )
            return RunOutcome(result.as_log_line(), [result])
        except subprocess.CalledProcessError as e:
            # Test aborted, mark it as FAILED
            elapsed_ms = int((time.time() - start_time) * 1000)
            print(f"Test aborted with return code {e.returncode}!", flush=True)
            output = e.output.decode("utf-8") if e.output else None
            print(f"Test output {output}", flush=True)
            stderr = e.stderr.decode("utf-8") if e.stderr else None
            print(f"Test error {stderr}", flush=True)
            result = GtestResult(
                test_prefix + test_to_run, GtestStatus.FAILED, elapsed_ms
            )
            return RunOutcome(result.as_log_line(), [result])

    def _string_in_file(self, file_path: str, string: str) -> bool | None:
        try:
            with open(file_path) as file:
                file_contents = file.read()
            return string in file_contents
        except FileNotFoundError:
            print(f"File not found when replacing string: {file_path}")

    def _replace_string_in_file(
        self, file_path: str, old_str: str, new_str: str
    ) -> None:
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

    def _backup_and_modify_config(self, conf_file: str) -> str:
        """Create a copy of the config and modify settings"""
        args = run_test.args
        if getattr(args, "run_on_reference_board", False):
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

    def _apply_simulator_env(self, simulator: str | None) -> None:
        """Overlay simulator-specific env vars onto this run's environment."""
        if simulator in XGS_SIMULATOR_ASICS:
            self.env_var.update(XGS_SIMULATOR_ENV)
        elif simulator in DNX_SIMULATOR_ASICS:
            self.env_var.update(DNX_SIMULATOR_ENV)

    def _run_tests(  # noqa: PLR0915 - complex orchestration; splitting would harm readability
        self, tests_to_run: list[str], conf_file: str, args: Namespace
    ) -> list[GtestResult]:
        sai_replayer_logging = getattr(args, "sai_replayer_logging", None)
        simulator = getattr(args, "simulator", None)

        if sai_replayer_logging:
            if os.path.isdir(sai_replayer_logging) or os.path.isfile(
                sai_replayer_logging
            ):
                raise ValueError(
                    f"File or directory {sai_replayer_logging} already exists."
                    "Remove or specify another directory and retry. Exitting"
                )

            os.makedirs(sai_replayer_logging)
        self._apply_simulator_env(simulator)

        # Determine if tests need to be run with warmboot mode too
        warmboot = False
        if args.coldboot_only is False:
            warmboot = True

        if conf_file and not os.path.exists(conf_file):
            print(f"########## Required configuration file not found: {conf_file}")
            return []

        all_results: list[GtestResult] = []
        try:
            self._setup_run(conf_file)
            num_tests = len(tests_to_run)
            for idx, test_to_run in enumerate(tests_to_run):
                test_prefix = self.COLDBOOT_PREFIX
                sai_replayer_log_path = self._get_sai_replayer_log_path(
                    test_prefix, test_to_run, sai_replayer_logging
                )
                # Run the test for coldboot verification
                self._setup_coldboot_test(sai_replayer_log_path)
                print("########## Running test: " + test_to_run, flush=True)
                if simulator:
                    self._restart_bcmsim(simulator)
                run_outcome = self._run_test(
                    conf_file,
                    test_prefix,
                    test_to_run,
                    warmboot,  # setup_warmboot
                    sai_replayer_log_path,
                )
                print(
                    f"########## Coldboot test results ({idx + 1}/{num_tests}): {run_outcome.console_output}",
                    flush=True,
                )
                all_results.extend(run_outcome.results)

                # Run the test again for warmboot verification if the test supports it.
                # With --num-warmboot-iterations N, soak the test across N consecutive
                # warmboots; each iteration re-arms --setup-for-warmboot so the next boot
                # can warmboot, and we stop early on the first failure.
                num_wb_iterations = max(1, getattr(args, "num_warmboot_iterations", 1))
                for wb_iter in range(num_wb_iterations):
                    if not (
                        warmboot and os.path.isfile(self._get_warmboot_check_file())
                    ):
                        break
                    # Re-arm warmboot for every iteration except the last so the next
                    # boot warmboots; the final iteration need not set up again.
                    setup_next_warmboot = wb_iter < num_wb_iterations - 1
                    test_prefix = (
                        self.WARMBOOT_PREFIX
                        if num_wb_iterations == 1
                        else f"{self.WARMBOOT_PREFIX}{wb_iter}."
                    )
                    sai_replayer_log_path = self._get_sai_replayer_log_path(
                        test_prefix, test_to_run, sai_replayer_logging
                    )
                    self._setup_warmboot_test(sai_replayer_log_path)
                    print(
                        "########## Verifying test with warmboot "
                        f"({wb_iter + 1}/{num_wb_iterations}): {test_to_run}",
                        flush=True,
                    )
                    run_outcome = self._run_test(
                        conf_file,
                        test_prefix,
                        test_to_run,
                        setup_next_warmboot,  # setup_warmboot
                        sai_replayer_log_path,
                    )
                    print(
                        f"########## Warmboot test results ({idx + 1}/{num_tests}, "
                        f"iter {wb_iter + 1}/{num_wb_iterations}): {run_outcome.console_output}",
                        flush=True,
                    )
                    all_results.extend(run_outcome.results)
                    # Stop the warmboot soak on first non-passing iteration.
                    # GtestStatus.OK is the passing status (displayed as "PASSED").
                    if any(r.status != GtestStatus.OK for r in run_outcome.results):
                        break
        finally:
            self._end_run()
        return all_results

    def _print_output_summary(self, results: list[GtestResult]) -> None:
        ConsoleReporter().print_gtest_summary(results)
        CsvReporter().write_gtest_results(results)

    def run_test(self, args: Namespace) -> None:
        test_binary = self._get_test_binary_name()
        # Some runners return an absolute path (e.g. /opt/fboss/bin/sai_test-sai_impl);
        # others return a bare binary name resolved via $PATH (e.g. platform_hw_test).
        if os.path.isabs(test_binary):
            binary_found = os.path.isfile(test_binary)
        else:
            binary_found = shutil.which(test_binary) is not None
        if not binary_found:
            print(
                f"Error: test binary not found: {test_binary}\n"
                f"\tMake sure the binary is installed at the expected path."
            )
            return

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
            results = self._run_tests(tests_to_run, conf_file, args)
            end_time = datetime.now()
            delta_time = end_time - start_time
            print(
                f"Running all tests took {delta_time} between {start_time} and {end_time}",
                flush=True,
            )
            self._print_output_summary(results)
        else:
            # Print the filtered tests
            for test in tests_to_run:
                print(test)
