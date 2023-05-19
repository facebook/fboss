#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import abc
import csv
import json
import os
import re
import subprocess
import sys
import time
from argparse import ArgumentParser
from datetime import datetime

# Helper to run HwTests
#
# RUNNING HW TESTS:
# -----------------
#
# Running HW Test Regression:
#
#  1. Run entire BCM SAI XGS Regression for a specific ASIC type and SDK
#
#   ./run_test.py sai --config $confFile --skip-known-bad-tests $test-config
#      Example to run HW Tests on Fuji Tomahawk4 Platform with bcm-sai SDK 8.2 and skips all known bad tests
#     ./run_test.py sai --config fuji.agent.materialized_JSON --skip-known-bad-tests "brcm/8.2.0.0_odp/8.2.0.0_odp/tomahawk4"
#
#  2. Run entire SAI DNX regression for Jericho2 and SDK:
#   ./run_test.py sai --config $confFile --coldboot_only --filter_file=$filter-file
#      Example to run HW Tests on Meru Jericho2 Platform and runs only the known good test listed for Jericho2 asic.
#     ./run_test.py sai --config meru400biu.agent.materialized_JSON --filter_file=known-good_regexes-brcm-sai-9.0_ea_dnx_odp-jericho2
#
# Sample invocation for HW Tests:
#
#  1. Running all HW SAI TESTS:
#      ./run_test.py sai --config $confFile
#      desc: Runs ALL Hw SAI tests: coldboot and warmboot
#
#  2. Running only coldboot tests:
#      ./run_test.py sai --config $confFile --coldboot_only
#      desc: Runs all HW tests only on coldboot mode
#
#  3. Running specific tests through regex:
#      ./run_test.py sai --config $confFile --filter=*Route*V6*
#      desc: Runs only the HW test matching the regex *Route*V6*
#
#      e./run_test.py sai --config $confFile --filter=*Vlan*:*Port*
#      desc: Run tests matching "Vlan" and "Port" filters
#
#  4. Running specific tests through regex and excluding certain tests:
#      ./run_test.py sai --config $confFile --filter=*Vlan*:*Port*:-*Mac*:*Intf*
#      desc: Run tests matching "Vlan" and "Port" filters, but excluding "Mac" and "Intf" tests in that list
#
#  5. Running a targetted test:
#      ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig
#      desc: Runs a single test by providing the entire test name as a filter
#
#  6. Enable SAI Replayer Logging:
#      ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig --sdk_logging /tmp/XYZ #
#      desc: Enables SAI Replayer Logging for the specified test case
#
#      ./run_test.py sai --config $confFile --sdk_logging /root/all_replayer_logs
#      desc: Enables SAI replayer Logging for all test cases
#
#  7. Running a single test in coldboot mode:
#      ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig --coldboot_only
#      desc: Runs a single test and performs only coldboot
#
#  8. Example of control plane tests:
#      ./run_test.py sai --config $confFile --filter=HwAclQualifierTest*
#      desc: Sample HW test by programming the hardware and validating the functionality
#
#  9. Example of data plane tests:
#      ./run_test.py sai --config $confFile --filter=HwOlympicQosSchedulerTest*
#      desc: Sample HW test by programming the hardware, creating a dataplane loop and validating functionality
#
#  10. List all tests in the test binary:
#      ./run_test.py sai --config $confFile --list_tests
#      desc: Print all the tests but do not run any test
#
#  11. List tests matching given regex:
#      ./run_test.py sai --config $confFile --filter=<filter_regex> --list_tests
#      desc: Print the matching tests but do not run any test
#
#  12. Skip running Known Bad Tests:
#      ./run_test.py sai --config $confFile --skip-known-bad-tests $test-config
#      desc: Run tests but skip running known bad tests for a specific platform
#
#  13. Use Custom Management Interface:
#      ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig --mgmt-if eth0
#      desc: Instead of eth0, provide a custom mgmt-if
#
#  14. Running non-OSS Binary using run_test helper:
#      ./run_test.py sai --config fuji.agent.materialized_JSON --filter HwVlanTest.VlanApplyConfig --sdk_logging /root/skhare/sai_replayer_logs --no-oss --sai-bin /root/skhare/sai_test-brcm-7.2.0.0_odp --mgmt-if eth0
#      desc: Runs tests but does not use OSS binary for testing
#
#
# TERMS & DEFINITIONS:
# ---------------------
#
# Known good tests:
# We maintain a list of known good tests per SDK per platform to run the HW tests
# Known good tests are listed under fboss.git/fboss/oss/hw_known_good_tests/
# This list can be provided to run_test.py as an argument --filter_file which
# will run only those tests that are listed in the file
#
# Skip known bad tests:
# We maintain a list of known bad tests per SDK per platform to skip running them
# when running the entite suite of HW tests. There is a consolidated Json file under
# fboss.git/oss/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON. This file is indexed
# based on $test-config format - vendor/coldboot-sdk-version-from/warmboot-sdk-version-to/asic.
# The known bad list can be provided to run_test.py as an argument --skip-known-bad-tests $testConfig
# which can be used to skip running known bad tests for specific platforms.
# eg: To skip running bad tests on tomahawk asic on SDK 7.2, the test-config to be used is
# "brcm/7.2.0.0_odp/7.2.0.0_odp/tomahawk"
#
# (TODO):
# SDK Logging:
# Coldboot & Warmboot:
# Regex

OPT_ARG_COLDBOOT = "--coldboot_only"
OPT_ARG_FILTER = "--filter"
OPT_ARG_FILTER_FILE = "--filter_file"
OPT_ARG_LIST_TESTS = "--list_tests"
OPT_ARG_CONFIG_FILE = "--config"
OPT_ARG_QSFP_CONFIG_FILE = "--qsfp-config"
OPT_ARG_SDK_LOGGING = "--sdk_logging"
OPT_ARG_SKIP_KNOWN_BAD_TESTS = "--skip-known-bad-tests"
OPT_ARG_OSS = "--oss"
OPT_ARG_NO_OSS = "--no-oss"
OPT_ARG_MGT_IF = "--mgmt-if"
OPT_ARG_SAI_BIN = "--sai-bin"
OPT_ARG_FRUID_PATH = "--fruid-path"
OPT_ARG_SIMULATOR = "--simulator"
OPT_ARG_SAI_LOGGING = "--sai_logging"
SUB_CMD_BCM = "bcm"
SUB_CMD_SAI = "sai"
SUB_CMD_QSFP = "qsfp"
WARMBOOT_CHECK_FILE = "/dev/shm/fboss/warm_boot/can_warm_boot_0"
SAI_HW_KNOWN_BAD_TESTS = (
    "./share/hw_known_bad_tests/sai_known_bad_tests.materialized_JSON"
)


class TestRunner(abc.ABC):
    ENV_VAR = dict(os.environ)
    WARMBOOT_SETUP_OPTION = "--setup-for-warmboot"
    COLDBOOT_PREFIX = "cold_boot."
    WARMBOOT_PREFIX = "warm_boot."
    TESTRUN_TIMEOUT = 1200

    _GTEST_RESULT_PATTERN = re.compile(
        r"""\[\s+(?P<status>(OK)|(FAILED)|(SKIPPED)|(TIMEOUT))\s+\]\s+
        (?P<test_case>[\w\.\/]+)\s+\((?P<duration>\d+)\s+ms\)$""",
        re.VERBOSE,
    )

    @abc.abstractmethod
    def _get_config_path(self):
        pass

    @abc.abstractmethod
    def _get_known_bad_tests_file(self):
        pass

    @abc.abstractmethod
    def _get_test_binary_name(self):
        pass

    @abc.abstractmethod
    def _get_sdk_logging_flags(self):
        pass

    @abc.abstractmethod
    def _get_sai_logging_flags(self):
        pass

    def _get_test_run_cmd(self, conf_file, test_to_run, flags):
        test_binary_name = self._get_test_binary_name()
        run_cmd = [
            test_binary_name,
            "--gtest_filter=" + test_to_run,
            "--fruid_filepath=" + args.fruid_path,
        ]
        run_args = []
        if "sai_test" in test_binary_name or test_binary_name == args.sai_bin:
            run_args = ["--config", conf_file, "--mgmt-if", args.mgmt_if]
        if test_binary_name == "qsfp_hw_test":
            run_args = ["--qsfp-config", args.qsfp_config]
        run_cmd += run_args

        return run_cmd + flags if flags else run_cmd

    def _add_test_prefix_to_gtest_result(self, run_test_output, test_prefix):
        run_test_result = run_test_output
        line = run_test_output.decode("utf-8")
        m = re.search(r"(?:OK|FAILED).* ] ", line)
        if m is not None:
            idx = m.end()
            run_test_result = (line[:idx] + test_prefix + line[idx:]).encode("utf-8")
        return run_test_result

    def _get_known_bad_test_regex(self):
        if not args.skip_known_bad_tests:
            return None

        known_bad_tests_file = self._get_known_bad_tests_file()

        with open(known_bad_tests_file) as f:
            known_bad_test_json = json.load(f)
            known_bad_test_structs = known_bad_test_json["known_bad_tests"][
                args.skip_known_bad_tests
            ]
            known_bad_tests = []
            for test_struct in known_bad_test_structs:
                known_bad_test = test_struct["test_name_regex"]
                known_bad_tests.append(known_bad_test)
        known_bad_tests_str = ":".join(known_bad_tests)
        return known_bad_tests_str

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
            line = line.split("#")[0].strip()
            if line.endswith("."):
                class_name = line[:-1]
            else:
                if not class_name:
                    raise "error"
                func_name = line.strip()
                ret.append("%s.%s" % (class_name, func_name))

        return ret

    def _parse_gtest_run_output(self, test_output):
        test_summary = []
        for line in test_output.decode("utf-8").split("\n"):
            match = self._GTEST_RESULT_PATTERN.match(line.strip())
            if not match:
                continue
            test_summary.append(line)
        return test_summary

    def _get_tests_to_run(self):
        # GTEST filter syntax is -
        #   --gtest_filter=<include_regexes>-<exclude_regexes>
        #   in case of multiple regexes, each one should be separated by ':'
        #
        # For example, to run all tests matching "Vlan" and "Port" but
        # excluding "Mac" tests in the list is -
        #   --gtest_filter=*Vlan*:*Port*:-*Mac*
        #
        # Also, multiple '-' is allowed but all regexes following the first '-'
        # are part of exclude list. This means following code would still work
        # as expected even if user provides an exclude list in the args.filter.
        tests_to_run = []
        # There are 4 variations of test filtering:
        # 1. Tests by filter without known bad
        # 2. Tests by filter with known bad
        # 3. All tests with known bad
        # 4. All tests without known bad
        if args.filter or args.filter_file:
            regexes = []
            if args.filter_file:
                with open(args.filter_file) as file:
                    regexes = [
                        line.strip()
                        for line in file
                        if not line.strip().startswith("#")
                    ]
            elif args.filter:
                regexes = [args.filter]

            for regex in regexes:
                filter = (regex + ":") if (regex is not None) else ""
                known_bad_tests_regex = self._get_known_bad_test_regex()
                filter = (
                    (filter + "-" + known_bad_tests_regex)
                    if (known_bad_tests_regex is not None)
                    else filter
                )
                filter = "--gtest_filter=" + filter
                # --gtest_filter matches based on wildcard, while our bad test list is
                # using regular expression. So, probperly convert regular expressions
                # like HwRouteTest/[01].StaticIp2MplsRoutes, HwLoadBalancerTestV[46].Ucmp.*
                filter = filter.replace(".*", "*")
                filter = filter.replace("[46]", "?")
                filter = filter.replace("[01]", "?")

                output = subprocess.check_output(
                    [self._get_test_binary_name(), "--gtest_list_tests", filter]
                )

                # Print all the matching tests
                print(output.decode("utf-8"))
                tests_to_run.extend(self._parse_list_test_output(output))
        else:
            known_bad_tests_regex = self._get_known_bad_test_regex()
            filter = ""
            if known_bad_tests_regex:
                filter = "*:-" + known_bad_tests_regex
                filter = "--gtest_filter=" + filter
                # --gtest_filter matches based on wildcard, while our bad test list is
                # using regular expression. So, probperly convert regular expressions
                # like HwRouteTest/[01].StaticIp2MplsRoutes, HwLoadBalancerTestV[46].Ucmp.*
                filter = filter.replace(".*", "*")
                filter = filter.replace("[46]", "?")
                filter = filter.replace("[01]", "?")
            output = subprocess.check_output(
                [self._get_test_binary_name(), "--gtest_list_tests", filter]
            )
            # Print all the matching tests
            print(output.decode("utf-8"))
            tests_to_run.extend(self._parse_list_test_output(output))
        return tests_to_run

    def _restart_bcmsim(self, asic):
        try:
            subprocess.Popen(
                # avoid warmboot, so as to run test with coldboot init, warmboot shut down
                # as a workaround for intermittent unclean exit issue in OSS environment
                ["rm", "-f", "/dev/shm/fboss/warm_boot/can_warm_boot_0"]
            )
            subprocess.Popen(
                # command to start th4 bcmsim service
                ["./runner.sh", "restart", "python3", "brcmsim.py", "-a", asic, "-s"]
            )
            time.sleep(60)
            print("Restarted bcmsim service", flush=True)
        except subprocess.CalledProcessError:
            print("Failed to restart bcmsim service", flush=True)

    def _run_test(
        self,
        conf_file,
        test_to_run,
        setup_warmboot,
        warmrun,
        sdk_logging_dir,
        sai_logging,
    ):
        flags = [self.WARMBOOT_SETUP_OPTION] if setup_warmboot else []
        test_prefix = self.WARMBOOT_PREFIX if warmrun else self.COLDBOOT_PREFIX

        if sdk_logging_dir:
            flags = flags + self._get_sdk_logging_flags(
                sdk_logging_dir, test_prefix, test_to_run
            )

        flags += self._get_sai_logging_flags(sai_logging)

        try:
            print(
                f"Running command {self._get_test_run_cmd(conf_file, test_to_run, flags)}",
                flush=True,
            )

            run_test_output = subprocess.check_output(
                self._get_test_run_cmd(conf_file, test_to_run, flags),
                timeout=self.TESTRUN_TIMEOUT,
                env=self.ENV_VAR,
            )

            # Add test prefix to test name in the result
            run_test_result = self._add_test_prefix_to_gtest_result(
                run_test_output, test_prefix
            )
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
                + str(self.TESTRUN_TIMEOUT * 1000)
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

    def _run_tests(self, tests_to_run, args):
        if args.sdk_logging:
            if os.path.isdir(args.sdk_logging) or os.path.isfile(args.sdk_logging):
                raise ValueError(
                    f"File or directory {args.sdk_logging} already exists."
                    "Remove or specify another directory and retry. Exitting"
                )

            os.makedirs(args.sdk_logging)
        if args.simulator:
            self.ENV_VAR["SOC_TARGET_SERVER"] = "127.0.0.1"
            self.ENV_VAR["BCM_SIM_PATH"] = "1"
            self.ENV_VAR["SOC_BOOT_FLAGS"] = "4325376"
            self.ENV_VAR["SAI_BOOT_FLAGS"] = "4325376"
            self.ENV_VAR["SOC_TARGET_PORT"] = "22222"
            self.ENV_VAR["SOC_TARGET_COUNT"] = "1"

        # Determine if tests need to be run with warmboot mode too
        warmboot = False
        if args.coldboot_only is False:
            warmboot = True

        test_binary_name = self._get_test_binary_name()
        conf_file = (
            args.config if (args.config is not None) else self._get_config_path()
        )
        if test_binary_name != "qsfp_hw_test" and not os.path.exists(conf_file):
            print("########## Conf file not found: " + conf_file)
            return []

        test_outputs = []
        num_tests = len(tests_to_run)
        for idx, test_to_run in enumerate(tests_to_run):
            # Run the test for coldboot verification
            print("########## Running test: " + test_to_run, flush=True)
            if args.simulator:
                self._restart_bcmsim(args.simulator)
            test_output = self._run_test(
                conf_file,
                test_to_run,
                warmboot,
                False,
                args.sdk_logging,
                args.sai_logging,
            )
            output = test_output.decode("utf-8")
            print(
                f"########## Coldboot test results ({idx+1}/{num_tests}): {output}",
                flush=True,
            )
            test_outputs.append(test_output)

            # Run the test again for warmboot verification if the test supports it
            if warmboot and os.path.isfile(WARMBOOT_CHECK_FILE):
                print(
                    "########## Verifying test with warmboot: " + test_to_run,
                    flush=True,
                )
                test_output = self._run_test(
                    conf_file,
                    test_to_run,
                    False,
                    True,
                    args.sdk_logging,
                    args.sai_logging,
                )
                output = test_output.decode("utf-8")
                print(
                    f"########## Warmboot test results ({idx+1}/{num_tests}): {output}",
                    flush=True,
                )
                test_outputs.append(test_output)

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
        for test_result in test_summary_count:
            print("  ", test_result, ":", test_summary_count[test_result])

        self._write_results_to_csv(test_summaries)

    def _write_results_to_csv(self, output):
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
        tests_to_run = self._get_tests_to_run()

        # Check if tests need to be run or only listed
        if args.list_tests is False:
            start_time = datetime.now()
            output = self._run_tests(tests_to_run, args)
            end_time = datetime.now()
            delta_time = end_time - start_time
            print(
                f"Running all tests took {delta_time} between {start_time} and {end_time}",
                flush=True,
            )
            self._print_output_summary(output)


class BcmTestRunner(TestRunner):
    def _get_config_path(self):
        return "/etc/coop/bcm.conf"

    def _get_known_bad_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        return "bcm_test"

    def _get_sdk_logging_flags(self, sdk_logging_dir, test_prefix, test_to_run):
        # TODO
        return []

    def _get_sai_logging_flags(self):
        # N/A
        return []


class SaiTestRunner(TestRunner):
    def _get_config_path(self):
        # TOOO Not available in OSS
        return ""

    def _get_known_bad_tests_file(self):
        return SAI_HW_KNOWN_BAD_TESTS

    def _get_test_binary_name(self):
        return args.sai_bin if args.sai_bin else "sai_test-sai_impl-1.12.0"

    def _get_sdk_logging_flags(self, sdk_logging_dir, test_prefix, test_to_run):
        return [
            "--enable-replayer",
            "--enable_get_attr_log",
            "--enable_packet_log",
            "--sai-log",
            os.path.join(
                sdk_logging_dir,
                "replayer-log-" + test_prefix + test_to_run.replace("/", "-"),
            ),
        ]

    def _get_sai_logging_flags(self, sai_logging):
        return ["--enable_sai_log", sai_logging]


class QsfpTestRunner(TestRunner):
    def _get_config_path(self):
        return ""

    def _get_known_bad_tests_file(self):
        return ""

    def _get_test_binary_name(self):
        return "qsfp_hw_test"

    def _get_sdk_logging_flags(self, sdk_logging_dir, test_prefix, test_to_run):
        return []

    def _get_sai_logging_flags(self, sai_logging):
        # N/A
        return []


if __name__ == "__main__":
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
            "run tests with specified qsfp config e.g. "
            + OPT_ARG_QSFP_CONFIG_FILE
            + "=./share/qsfp_test_configs/meru400bfu.materialized_JSON"
        ),
    )
    ap.add_argument(
        OPT_ARG_SDK_LOGGING,
        type=str,
        help=(
            "Enable SDK logging (e.g. SAI replayer for sai tests"
            "and store the logs in the supplied directory)"
        ),
    )
    ap.add_argument(
        OPT_ARG_SKIP_KNOWN_BAD_TESTS,
        type=str,
        help=(
            "test config to specify which known bad tests to skip e.g. "
            + OPT_ARG_SKIP_KNOWN_BAD_TESTS
            + "=brcm/7.2.0.0_odp/7.2.0.0_odp/tomahawk"
        ),
    )
    ap.add_argument(
        OPT_ARG_MGT_IF,
        type=str,
        default="eth0",
        help=("Management interface (default = eth0)"),
    )
    ap.add_argument(OPT_ARG_SAI_BIN, type=str, help=("SAI Test binary name"))

    ap.add_argument(
        OPT_ARG_OSS,
        action="store_true",
        help="OSS build",
    )

    ap.add_argument(
        OPT_ARG_NO_OSS,
        action="store_false",
        dest="oss",
        help="No OSS build",
    )
    ap.set_defaults(oss=True)

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
            "Specify what asic simulator to use if configured. "
            "Default is None, meaning physical asic is used"
        ),
    )
    ap.add_argument(
        OPT_ARG_SAI_LOGGING,
        type=str,
        default="WARN",
        help=("Enable SAI logging (Options: DEBUG|INFO|NOTICE|WARN|ERROR|CRITICAL)"),
    )

    # Add subparsers for different test types
    subparsers = ap.add_subparsers()

    # Add subparser for BCM tests
    bcm_test_parser = subparsers.add_parser(SUB_CMD_BCM, help="run bcm tests")
    bcm_test_parser.set_defaults(func=BcmTestRunner().run_test)

    # Add subparser for SAI tests
    sai_test_parser = subparsers.add_parser(SUB_CMD_SAI, help="run sai tests")
    sai_test_parser.set_defaults(func=SaiTestRunner().run_test)

    # Add subparser for QSFP tests
    qsfp_test_parser = subparsers.add_parser(SUB_CMD_QSFP, help="run qsfp tests")
    qsfp_test_parser.set_defaults(func=QsfpTestRunner().run_test)

    # Parse the args
    args = ap.parse_known_args()
    args = ap.parse_args(args[1], args[0])

    if args.oss:
        if ("FBOSS_BIN" not in os.environ) or ("FBOSS_LIB" not in os.environ):
            print(
                "FBOSS environment not set. Run `source /opt/fboss/bin/setup_fboss_env'"
            )
            sys.exit(0)

    if args.filter and args.filter_file:
        raise ValueError(
            f"Only one of the {OPT_ARG_FILTER} or {OPT_ARG_FILTER_FILE} can be specified at any time"
        )

    try:
        func = args.func
    except AttributeError as e:
        raise AttributeError("too few arguments") from e

    func(args)
