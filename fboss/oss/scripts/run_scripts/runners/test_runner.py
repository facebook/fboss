#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import abc
import csv
import json
import os
import re
import shutil
import subprocess
import time
from argparse import ArgumentParser
from datetime import datetime
from typing import ClassVar

import run_test

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
    def _setup_run(self, conf_file: str) -> None:
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
        args = run_test.args
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
        # Anchor to the gtest result-line format `[  STATUS ] ` so we don't
        # match incidental tokens like "FAILED to allocate" in log noise.
        m = re.search(r"\[\s*(?:OK|FAILED|SKIPPED|TIMEOUT)\s*\] ", line)
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
        args = run_test.args
        test_names = []
        if args.filter or args.filter_file:
            if args.filter_file:
                gtest_regexes = run_test._load_from_file(args.filter_file, args.profile)
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
        test_run_timeout_in_second: int = run_test.DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND,
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
            elapsed_ms = int((time.time() - start_time) * 1000)
            print(f"Test aborted with return code {e.returncode}!", flush=True)
            output = e.output.decode("utf-8") if e.output else None
            print(f"Test output {output}", flush=True)
            stderr = e.stderr.decode("utf-8") if e.stderr else None
            print(f"Test error {stderr}", flush=True)
            run_test_result = (
                "[   FAILED ] "
                + test_prefix
                + test_to_run
                + " ("
                + str(elapsed_ms)
                + " ms)"
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
        args = run_test.args
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
        if args.simulator in run_test.XGS_SIMULATOR_ASICS:
            self.ENV_VAR["SOC_TARGET_SERVER"] = "127.0.0.1"
            self.ENV_VAR["BCM_SIM_PATH"] = "1"
            self.ENV_VAR["SOC_BOOT_FLAGS"] = "4325376"
            self.ENV_VAR["SAI_BOOT_FLAGS"] = "4325376"
            self.ENV_VAR["SOC_TARGET_PORT"] = "22222"
            self.ENV_VAR["SOC_TARGET_COUNT"] = "1"
        elif args.simulator in run_test.DNX_SIMULATOR_ASICS:
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
        try:
            self._setup_run(conf_file)
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
                    args.test_run_timeout,
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
        finally:
            self._end_run()
        return test_outputs

    _GTEST_STATUS_MAP: ClassVar[dict[str, str]] = {
        "OK": "PASSED",
        "FAILED": "FAILED",
        "SKIPPED": "SKIPPED",
        "TIMEOUT": "TIMEOUT",
    }

    def _print_output_summary(self, test_outputs):
        test_summaries = []
        test_summary_count = {"PASSED": 0, "FAILED": 0, "SKIPPED": 0, "TIMEOUT": 0}
        for test_output in test_outputs:
            test_summaries += self._parse_gtest_run_output(test_output)
        # Print test results and update test result counts
        for test_summary in test_summaries:
            m = re.search(r"\[\s*(OK|FAILED|SKIPPED|TIMEOUT)\s*\]", test_summary)
            if m is None:
                print(test_summary)
                continue
            mapped_status = self._GTEST_STATUS_MAP[m.group(1)]
            line = test_summary.replace(m.group(0), f"[ {mapped_status} ]")
            print(line)
            test_summary_count[mapped_status] += 1
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
                raw_result = line.split("]")[0].strip("[ ")
                # Map gtest tokens so CSV matches the console summary
                # (OK -> PASSED, etc.).
                test_result = self._GTEST_STATUS_MAP.get(raw_result, raw_result)
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
