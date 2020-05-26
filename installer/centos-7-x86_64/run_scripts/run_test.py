#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.


import os
import re
import subprocess
import sys
from argparse import ArgumentParser


OPT_ARG_COLDBOOT = "--coldboot_only"
OPT_ARG_FILTER = "--filter"
OPT_ARG_BCM_CONF_FILE = "--conf-file"
SUB_CMD_BCM = "bcm"
WARMBOOT_CHECK_FILE = "/dev/shm/fboss/bcm_test/warm_boot/can_warm_boot_0"


class TestRunner:
    ENV_VAR = dict(os.environ)
    BCM_CONF_PATH = "/etc/coop/bcm.conf"
    WARMBOOT_SETUP_OPTION = "--setup-for-warmboot"
    COLDBOOT_PREFIX = "cold_boot."
    WARMBOOT_PREFIX = "warm_boot."
    TESTRUN_TIMEOUT = 300

    _GTEST_RESULT_PATTERN = re.compile(
        r"""\[\s+(?P<status>(OK)|(FAILED)|(SKIPPED)|(TIMEOUT))\s+\]\s+
        (?P<test_case>[\w\.]+)\s+\((?P<duration>\d+)\s+ms\)$""",
        re.VERBOSE,
    )

    def _add_test_prefix_to_gtest_result(self, run_test_output, test_prefix):
        run_test_result = run_test_output
        line = run_test_output.decode("utf-8")
        m = re.search(r"(?:OK|FAILED).* ] ", line)
        if m is not None:
            idx = m.end()
            run_test_result = (line[:idx] + test_prefix + line[idx:]).encode("utf-8")
        return run_test_result

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

    def _get_bcm_tests_to_run(self, args):
        filter = ("--gtest_filter=" + args.filter) if (args.filter is not None) else ""
        output = subprocess.check_output(
            ["bcm_test", "--gtest_list_tests", filter], env=self.ENV_VAR
        )
        return self._parse_list_test_output(output)

    def _run_bcm_test(self, conf_file, test_to_run, setup_warmboot, warmrun):
        flags = self.WARMBOOT_SETUP_OPTION if setup_warmboot else ""
        test_prefix = self.WARMBOOT_PREFIX if warmrun else self.COLDBOOT_PREFIX
        try:
            run_test_output = subprocess.check_output(
                [
                    "bcm_test",
                    "--bcm_config",
                    conf_file,
                    "--flexports",
                    "--gtest_filter=" + test_to_run,
                    flags,
                ],
                timeout=self.TESTRUN_TIMEOUT,
                env=self.ENV_VAR,
            )
            # Add test prefix to test name in the result
            run_test_result = self._add_test_prefix_to_gtest_result(
                run_test_output, test_prefix
            )
        except subprocess.TimeoutExpired:
            # Test timed out, mark it as TIMEOUT
            run_test_result = (
                "[  TIMEOUT ] "
                + test_prefix
                + test_to_run
                + " ("
                + str(self.TESTRUN_TIMEOUT * 1000)
                + " ms)"
            ).encode("utf-8")
        except subprocess.CalledProcessError:
            # Test aborted, mark it as FAILED
            run_test_result = (
                "[   FAILED ] " + test_prefix + test_to_run + " (0 ms)"
            ).encode("utf-8")
        return run_test_result

    def _run_bcm_tests(self, tests_to_run, args):
        # Determine if tests need to be run with warmboot mode too
        warmboot = False
        if args.coldboot_only is False:
            warmboot = True

        conf_file = (
            args.conf_file if (args.conf_file is not None) else self.BCM_CONF_PATH
        )
        if not os.path.exists(conf_file):
            print("########## Conf file not found: " + conf_file)
            return []

        test_outputs = []
        for test_to_run in tests_to_run:
            # Run the test for coldboot verification
            print("########## Running test: " + test_to_run, flush=True)
            test_output = self._run_bcm_test(conf_file, test_to_run, warmboot, False)
            test_outputs.append(test_output)

            # Run the test again for warmboot verificationif the test supports it
            if warmboot and os.path.isfile(WARMBOOT_CHECK_FILE):
                print(
                    "########## Verifying test with warmboot: " + test_to_run,
                    flush=True,
                )
                test_output = self._run_bcm_test(conf_file, test_to_run, False, True)
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

    def run_bcm_test(self, args):
        tests_to_run = self._get_bcm_tests_to_run(args)
        output = self._run_bcm_tests(tests_to_run, args)
        self._print_output_summary(output)


if __name__ == "__main__":
    if ("FBOSS_BIN" not in os.environ) or ("FBOSS_LIB" not in os.environ):
        print("FBOSS environment not set. Run `source /opt/fboss/bin/setup_fboss_env'")
        sys.exit(0)

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
        OPT_ARG_BCM_CONF_FILE,
        type=str,
        help=(
            "run with the specified config file e.g. "
            + OPT_ARG_BCM_CONF_FILE
            + "=./share/bcm-configs/WEDGE100+RSW-bcm.conf"
        ),
    )

    # Add subparsers for different test types
    subparsers = ap.add_subparsers()

    # Add subparser for BCM tests
    bcm_test_parser = subparsers.add_parser(SUB_CMD_BCM, help="run bcm tests")
    bcm_test_parser.set_defaults(func=TestRunner().run_bcm_test)

    # Parse the args
    args = ap.parse_known_args()
    args = ap.parse_args(args[1], args[0])
    args.func(args)
