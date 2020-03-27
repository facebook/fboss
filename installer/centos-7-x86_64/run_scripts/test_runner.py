#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.


import os
import re
import subprocess


class TestRunner:
    SCRIPT_DIR_ABS = os.path.dirname(os.path.realpath(__file__))
    ENV_VAR = dict(os.environ, LD_LIBRARY_PATH=SCRIPT_DIR_ABS)
    BCM_CONFIG_DIR = os.path.join(SCRIPT_DIR_ABS, "bcm_configs")
    WEDGE100S_RSW_BCM_CONF_PATH = os.path.join(BCM_CONFIG_DIR, "WEDGE100S+RSW-bcm.conf")

    _GTEST_RESULT_PATTERN = re.compile(
        r"""\[\s+(?P<status>(OK)|(FAILED)|(SKIPPED))\s+\]\s+
        (?P<test_case>[\w\.]+)\s+\((?P<duration>\d+)\s+ms\)$""",
        re.VERBOSE,
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
            line = line.split("#")[0].strip()
            if line.endswith("."):
                class_name = line[:-1]
            else:
                if not class_name:
                    raise "error"
            func_name = line.strip()
            ret.append("%s.%s" % (class_name, func_name))

        return ret

    def _parse_gtest_run_output_and_print(self, test_output):
        for line in test_output.decode("utf-8").split("\n"):
            match = self._GTEST_RESULT_PATTERN.match(line.strip())
            if not match:
                continue
            print(line)

    def _get_tests_to_run(self):
        # TODO(skhare) generalize to run SAI tests as well
        output = subprocess.check_output(
            ["./bcm_test", "--gtest_list_tests"], env=self.ENV_VAR
        )
        return self._parse_list_test_output(output)

    def _run_test(self, test_to_run):
        try:
            run_test_output = subprocess.check_output(
                [
                    "./bcm_test",
                    "--bcm_config",
                    self.WEDGE100S_RSW_BCM_CONF_PATH,
                    "--flexports",
                    "--gtest_filter=" + test_to_run,
                ],
                env=self.ENV_VAR,
            )
        except subprocess.CalledProcessError as e:
            run_test_output = e.output
        return run_test_output

    def _run_tests(self, tests_to_run):
        # TODO(skhare)  Run warmboot test as well
        test_outputs = []
        for test_to_run in tests_to_run:
            print("########## Running Test: " + test_to_run)
            test_output = self._run_test(test_to_run)
            test_outputs.append(test_output)

        return test_outputs

    def _print_output_summary(self, test_outputs):
        for test_output in test_outputs:
            self._parse_gtest_run_output_and_print(test_output)

    def run(self):
        tests_to_run = self._get_tests_to_run()
        output = self._run_tests(tests_to_run)
        self._print_output_summary(output)


if __name__ == "__main__":
    TestRunner().run()
