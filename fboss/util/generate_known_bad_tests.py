#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse

import json
import os
import shutil

"""
   Helper script to propagate current known bad test list to the open source.

   Sample invocation:

   # Generate known bad regexes for SAI

   ./generate_known_bad_tests.py \
    $HOME/configerator/materialized_configs/neteng/netcastle/test_config/team_test_configs/sai_test.materialized_JSON \
    $HOME/fbsource/fbcode/fboss/sai_known_bad_tests

   # Generate known bad regexes for native SDK

   ./generate_known_bad_tests.py \
    $HOME/configerator/materialized_configs/neteng/netcastle/test_config/team_test_configs/fboss_bcm.materialized_JSON \
    $HOME/fbsource/fbcode/fboss/bcm_known_bad_tests
"""


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "known_bad_tests_json",
        help="Full path to known_bad_tests_json in configerator"
        "e.g.  $HOME/configerator/materialized_configs/neteng/netcastle/test_config/team_test_configs/sai_test.materialized_JSON",
    )

    parser.add_argument(
        "output_path",
        help="Full path to directory to store known bad regexes. "
        "The directory contents will be overwritten"
        "e.g. $HOME/fbsource/fbcode/fboss/sai_known_bad_tests",
    )

    return parser.parse_args()


class GenerateKnownBadTests:
    def __init__(self, args):
        self._output_path = args.output_path
        self._known_bad_tests_json = args.known_bad_tests_json

    def _cleanup(self):
        if os.path.isdir(self._output_path):
            shutil.rmtree(self._output_path)

        os.makedirs(self._output_path)

    def _get_all_known_bad_regexes(self):
        with open(self._known_bad_tests_json) as f:
            all_known_bad_regexes = json.load(f)["known_bad_tests"]

        return all_known_bad_regexes

    def _write_to_file(self, regex_file, known_bad_regexes):
        with open(regex_file, "w", encoding="utf-8") as f:
            for item in known_bad_regexes:
                f.write(f'"{item}"\n')

    def run(self):
        self._cleanup()
        all_known_bad_regexes = self._get_all_known_bad_regexes()

        for test_type in all_known_bad_regexes.keys():
            known_bad_regexes = [
                val["test_name_regex"] for val in all_known_bad_regexes[test_type]
            ]
            regex_file = os.path.join(
                self._output_path, "known-bad-regexes-" + test_type.replace("/", "-")
            )
            self._write_to_file(regex_file, known_bad_regexes)


if __name__ == "__main__":
    args = parse_args()
    GenerateKnownBadTests(args).run()
