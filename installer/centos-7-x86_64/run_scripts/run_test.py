#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import abc
import os
import re
import subprocess
import sys
from argparse import ArgumentParser

# Helper to run HwTests
#
# Sample invocation:
#  ./run_test.py sai --config $confFile # run ALL Hw SAI tests: coldboot + #  warmboot
#  ./run_test.py sai --config $confFile --coldboot_only # run cold boot only
#  ./run_test.py sai --config $confFile --filter=*Route*V6* # run tests #  matching filter
#  ./run_test.py sai --config $confFile --filter=*Vlan*:*Port* # run tests matching "Vlan" and "Port" filters
#  ./run_test.py sai --config $confFile --filter=*Vlan*:*Port*:-*Mac*:*Intf* # run tests matching "Vlan" and "Port" filters, but excluding "Mac" and "Intf" tests in that list

#  ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig # single test
#  ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig --sdk_logging /tmp/XYZ # SAI replayer logging
#  ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig --coldboot_only # single test, cold boot
#  ./run_test.py sai --config $confFile --filter=HwAclQualifierTest* # some control plane tests
#  ./run_test.py sai --config $confFile --filter=HwOlympicQosSchedulerTest* # some dataplane tests
#  ./run_test.py sai --config $confFile --list_tests # Print all the tests but do not run any test
#  ./run_test.py sai --config $confFile --filter=<filter_regex> --list_tests # Print the matching tests but do not run any test
#  ./run_test.py sai --config $confFile --sdk_logging /root/all_replayer_logs # ALL tests with SAI replayer logging

#  ./run_test.py sai --config $confFile --skip-known-bad-tests $knownBadTestsFile --file HwRouteScaleTest.turboFabricScaleTest # skip running known bad tests
#  ./run_test.py sai --config $confFile --skip-known-bad-tests $knownBadTestsFile # skip running known bad tests
#
#  ./run_test.py sai --config $confFile --filter=HwVlanTest.VlanApplyConfig #  --mgmt-if eth0 # Using custom mgmt-if
#
# Running non-OSS:
#  ./run_test.py sai --config fuji.agent.materialized_JSON --filter HwVlanTest.VlanApplyConfig --sdk_logging /root/skhare/sai_replayer_logs --no-oss --sai-bin /root/skhare/sai_test-brcm-7.2.0.0_odp --mgmt-if eth0
#
# All tests matching the following filters pass on w400C in VOQ mode
# VOQ tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwVoqSwitchTest.*:-*sendPacketCpu*:*trapPktsOnPort*:*AclQualifier*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwVoqSwitchWithFabricPortsTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwVoqSwitchWithMultipleDsfNodesTest.*
# ECMP Tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwEcmpTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=SaiNextHopGroupTest.*
# Basic forwarding tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwLoopBackTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwL4PortBlackHolingTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwJumboFramesTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwRxReasonTests.*
# Route programming tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwRouteTest/0.*:-*Mpls*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwRouteTest/1.*:-*Mpls*
# Neighbor programming tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwNeighborTest/0.*:-*LookupClass
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwNeighborTest/2.*:-*LookupClass
# ACL tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwAclCounterTest.*:-*Sport*:*BumpOn*Cpu*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=SaiAclTableRecreateTests.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwAclPriorityTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwAclMatchActionsTest.*
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwAclStatTest.*
# Counter Tests
# ./run_test.py sai --config wedge400c_voq.agent.materialized_JSON --filter=HwInDiscardsCounterTest.*
#
# All tests matching the following filters pass on w400C in fabric mode
# ./run_test.py sai --config wedge400c_fabric.agent.materialized_JSON --filter=HwFabricSwitchTest.*

# All tests matching following filters are expected to PASS on Makalu
# Basic VOQ switch tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwVoqSwitchTest*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwVoqSwitchWithFabriPortsTest.*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwVoqSwitchWithMultipleDsfNodesTest.*
# Basic forwarding tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwJumboFramesTest.*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwLoopBackTest.*
# ./run_test.py sai --config makalu.agent.materialized_JSON  --filter=HwL4PortBlackHolingTest.*
# Counter tests
# ./run_test.py sai --config makalu.agent.materialized_JSON  --filter=HwInPauseDiscardsCounterTest.*
# ECMP Tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwEcmpTest.*:-*Ucmp*
# Load Balancer Tests
# UCMP support lacking in DNX
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwLoadBalancerTestV4.*:-*Ucmp*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwLoadBalancerTestV6.*:-*Ucmp*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwHashConsistencyTest.*:-*EcmpExpand*:*MemberOrder*
# LB Tests with ROCE traffic
# ./run_test.py sai --config makalu.agent.materialized_JSON  --filter=HwLoadBalancerNegativeTestV6RoCE.*
# Route programming tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwAlpmStressTest.*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=SaiNextHopGroupTest.*
# Neighbor programming tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwNeighborTest/0.*:-*LookupClass
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwNeighborTest/2.*:-*LookupClass
# V4 routes
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwRouteTest/0.*:-*Mpls*:*ClassId*:*ClassID*
# V6 routes
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwRouteTest/1.*:-*Mpls*:*ClassId*:*ClassID*
# ACLs
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwAclPriorityTest.*:-*AclsChanged*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwAclCounterTest.*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=SaiAclTableRecreateTests.*
# ./run_test.py sai --config makalu.agent.materialized_JSON
# --filter=HwAclStatTest.*:-*AclStatCreate:*AclStatCreateShared:*AclStatCreateMultiple:*AclStatMultipleActions:*AclStatDeleteShared*:*AclStatDeleteSharedPostWarmBoot:*AclStatRename*:*AclStatModify:*AclStatShuffle:*StatNumberOfCounters:*AclStatChangeCounterType
# Packet send test
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwPacketSendTest.PortTxEnableTest
# PFC tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwPfcTest.*:-*Watchdog*
# PFC traffic tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwTrafficPfc*:-*Watchdog*:*Zero*
# Qos  tests
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwOlympicQosTests.VerifyDscpQueueMappingFrontPanel
# CoPP
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwRxReasonTests.*
# ./run_test.py sai --config makalu.agent.materialized_JSON --filter=HwCoppTest/0.Ipv6LinkLocalMcastToMidPriQ:HwCoppTest/0.Ipv6LinkLocalMcastNetworkControlDscpToHighPriQ:HwCoppTest/0.L3MTUErrorToLowPriQ:HwCoppTest/0.UnresolvedRoutesToLowPriQueue
# All tests matching following filter are expected to PASS on Kamet
# ./run_test.py sai --config kamet.agent.materialized_JSON --filter=HwFabricSwitchTest*

OPT_ARG_COLDBOOT = "--coldboot_only"
OPT_ARG_FILTER = "--filter"
OPT_ARG_LIST_TESTS = "--list_tests"
OPT_ARG_CONFIG_FILE = "--config"
OPT_ARG_SDK_LOGGING = "--sdk_logging"
OPT_ARG_SKIP_KNOWN_BAD_TESTS = "--skip-known-bad-tests"
OPT_ARG_OSS = "--oss"
OPT_ARG_NO_OSS = "--no-oss"
OPT_ARG_MGT_IF = "--mgmt-if"
OPT_ARG_SAI_BIN = "--sai-bin"
SUB_CMD_BCM = "bcm"
SUB_CMD_SAI = "sai"
WARMBOOT_CHECK_FILE = "/dev/shm/fboss/warm_boot/can_warm_boot_0"


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
    def _get_test_binary_name(self):
        pass

    @abc.abstractmethod
    def _get_sdk_logging_flags(self):
        pass

    def _get_test_run_cmd(self, conf_file, test_to_run, flags):
        run_cmd = [
            self._get_test_binary_name(),
            "--mgmt-if",
            args.mgmt_if,
            "--config",
            conf_file,
            "--gtest_filter=" + test_to_run,
        ]

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
        if not args.skip_known_bad_tests or not os.path.isfile(
            args.skip_known_bad_tests
        ):
            return None

        with open(args.skip_known_bad_tests) as f:
            lines = f.readlines()
            lines = [line.strip().strip('"') for line in lines]
        known_bad_tests = ":".join(lines)
        return known_bad_tests

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

    def _get_tests_to_run(self, args):
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
        filter = (args.filter + ":") if (args.filter is not None) else ""
        known_bad_tests_regex = self._get_known_bad_test_regex()
        filter = (
            (filter + "-" + known_bad_tests_regex)
            if (known_bad_tests_regex is not None)
            else filter
        )
        filter = "--gtest_filter=" + filter
        output = subprocess.check_output(
            [self._get_test_binary_name(), "--gtest_list_tests", filter]
        )

        # Print all the matching tests
        print(output.decode("utf-8"))

        return self._parse_list_test_output(output)

    def _run_test(
        self, conf_file, test_to_run, setup_warmboot, warmrun, sdk_logging_dir
    ):
        flags = [self.WARMBOOT_SETUP_OPTION] if setup_warmboot else []
        test_prefix = self.WARMBOOT_PREFIX if warmrun else self.COLDBOOT_PREFIX

        if sdk_logging_dir:
            flags = flags + self._get_sdk_logging_flags(
                sdk_logging_dir, test_prefix, test_to_run
            )

        try:
            print(
                f"Running command {self._get_test_run_cmd(conf_file, test_to_run, flags)}"
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

    def _run_tests(self, tests_to_run, args):
        if args.sdk_logging:
            if os.path.isdir(args.sdk_logging) or os.path.isfile(args.sdk_logging):
                raise ValueError(
                    f"File or directory {args.sdk_logging} already exists."
                    "Remove or specify another directory and retry. Exitting"
                )

            os.makedirs(args.sdk_logging)

        # Determine if tests need to be run with warmboot mode too
        warmboot = False
        if args.coldboot_only is False:
            warmboot = True

        conf_file = (
            args.config if (args.config is not None) else self._get_config_path()
        )
        if not os.path.exists(conf_file):
            print("########## Conf file not found: " + conf_file)
            return []

        test_outputs = []
        for test_to_run in tests_to_run:
            # Run the test for coldboot verification
            print("########## Running test: " + test_to_run, flush=True)
            test_output = self._run_test(
                conf_file, test_to_run, warmboot, False, args.sdk_logging
            )
            test_outputs.append(test_output)

            # Run the test again for warmboot verification if the test supports it
            if warmboot and os.path.isfile(WARMBOOT_CHECK_FILE):
                print(
                    "########## Verifying test with warmboot: " + test_to_run,
                    flush=True,
                )
                test_output = self._run_test(
                    conf_file, test_to_run, False, True, args.sdk_logging
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

    def run_test(self, args):
        tests_to_run = self._get_tests_to_run(args)

        # Check if tests need to be run or only listed
        if args.list_tests is False:
            output = self._run_tests(tests_to_run, args)
            self._print_output_summary(output)


class BcmTestRunner(TestRunner):
    def _get_config_path(self):
        return "/etc/coop/bcm.conf"

    def _get_test_binary_name(self):
        return "bcm_test"

    def _get_sdk_logging_flags(self, sdk_logging_dir, test_prefix, test_to_run):
        # TODO
        return []


class SaiTestRunner(TestRunner):
    def _get_config_path(self):
        # TOOO Not available in OSS
        return ""

    def _get_test_binary_name(self):
        return args.sai_bin if args.sai_bin else "sai_test-sai_impl-1.10.2"

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
            "skip running known bad tests specified in a file e.g. "
            + OPT_ARG_SKIP_KNOWN_BAD_TESTS
            + "=path-to-known-bad-tests-file"
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

    # Add subparsers for different test types
    subparsers = ap.add_subparsers()

    # Add subparser for BCM tests
    bcm_test_parser = subparsers.add_parser(SUB_CMD_BCM, help="run bcm tests")
    bcm_test_parser.set_defaults(func=BcmTestRunner().run_test)

    # Add subparser for SAI tests
    sai_test_parser = subparsers.add_parser(SUB_CMD_SAI, help="run sai tests")
    sai_test_parser.set_defaults(func=SaiTestRunner().run_test)

    # Parse the args
    args = ap.parse_known_args()
    args = ap.parse_args(args[1], args[0])

    if args.oss:
        if ("FBOSS_BIN" not in os.environ) or ("FBOSS_LIB" not in os.environ):
            print(
                "FBOSS environment not set. Run `source /opt/fboss/bin/setup_fboss_env'"
            )
            sys.exit(0)

    try:
        func = args.func
    except AttributeError as e:
        raise AttributeError("too few arguments") from e

    func(args)
