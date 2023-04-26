#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import json

from neteng.fboss.sdk_test_config.types import SdkTestConfig, TestConfig
from neteng.fboss.switch_config.types import AsicType
from thrift.py3 import deserialize, Protocol, serialize


class SdkTests:
    def __init__(self, config_file: str):
        self.sdk_test_config = self.parse_config(config_file)

    def parse_config(self, config_file: str):
        with open(config_file, "rb") as config_file:
            data = config_file.read()
            return deserialize(
                structKlass=SdkTestConfig,
                buf=data,
                protocol=Protocol.JSON,
            )

    @staticmethod
    def example_config() -> str:
        netcastleConfig = TestConfig(
            asic=[
                AsicType.ASIC_TYPE_TOMAHAWK,
                AsicType.ASIC_TYPE_TOMAHAWK3,
                AsicType.ASIC_TYPE_TOMAHAWK4,
            ],
            fromSdk="9.0_ea_odp",
            toSdk="9.0_ea_odp",
            regex="*",
        )
        dumped_sdk_test_config = SdkTestConfig(testConfig=[netcastleConfig])

        json_obj = json.loads(serialize(dumped_sdk_test_config, Protocol.JSON))
        return json.dumps(json_obj)

    def run(self) -> None:
        # TODO(zecheng): Kick off multiple processes to run different
        # types of tests on different ASICs.
        pass

    def wait_and_process(self) -> None:
        # TODO(zecheng): Wait for processes to finish.
        # If any of the tests is done, process the results and gather information.
        # e.g. Failed tests console output and replay logs.
        # Also need to handle unexpected failures and Ctrl-C.
        pass

    def generate_report(self) -> None:
        # TODO(zecheng): Generate report on test results and break them up by categories.
        pass

    def create_report(self) -> None:
        # TODO(zecheng): With all the results ready, create a report on the current drop.
        pass

    def compare_and_highlight_regression(self) -> None:
        # TODO(zecheng): If provided, compare with the reports of previous EA drop and highlight regressions.
        pass

    def generate_task_template_for_regressions(self) -> None:
        # TODO(zecheng): With regressions compared to previous EA drop, generate CSP/Jira templates
        # to be used to report the issue with vendor.
        pass


def main():
    psr = argparse.ArgumentParser(
        description="This script takes in config file and run SDK tests."
    )
    psr.add_argument(
        "--config",
        required=True,
        type=str,
        help="Json config file that specifies SDK tests need to be run. For example: "
        + SdkTests.example_config(),
    )
    args = psr.parse_args()

    sdk_tests = SdkTests(args.config)
    sdk_tests.run()
    sdk_tests.wait_and_process()
    sdk_tests.generate_report()
    sdk_tests.compare_and_highlight_regression()
    sdk_tests.generate_task_template_for_regressions()


if __name__ == "__main__":
    main()
