#!/usr/bin/env python3

"""
Scan every word in the SAI Spec for all SAI symbols of interest => Set1.
Scan every word in the FBOSS SAI API code for all SAI symbols of interest => Set2.
Print intersection of Set1 and Set2.

Sample invocation:

fboss/util/find_sai_spec_symbols_in_fboss.py $HOME/local/github/sai.git/inc $HOME/fbsource/fbcode/fboss/agent/hw/sai/api
"""

import argparse
import os
import re


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("sai_spec", help="Directory for SAI spec")
    parser.add_argument("fboss_sai_api", help="Directory for FBOSS SAI API")
    parser.add_argument(
        "--verbose", help="Verbose, print all SAI symbols", action="store_true"
    )

    return parser.parse_args()


class FindSaiSymbols:
    _PATTERN = re.compile(r"(sai|SAI|create_|remove_|get_|set_)(.*)")

    def _get_sai_symbols(self, directory):
        all_sai_symbols = set()

        for filename in os.listdir(directory):
            # skip hidden files
            if filename.startswith("."):
                continue

            f = os.path.join(directory, filename)
            if not os.path.isfile(f):
                continue

            for line in open(f):
                for word in line.split():
                    for match in re.finditer(self._PATTERN, word):
                        sai_attr = re.split(r"\.|:|;|,|\(|\)", match.group())[0]
                        all_sai_symbols.add(sai_attr)

        return all_sai_symbols

    def __init__(self, args):
        self._sai_spec_directory = args.sai_spec
        self._fboss_sai_api_directory = args.fboss_sai_api
        self._verbose = args.verbose

    def run(self):
        sai_spec_symbols = self._get_sai_symbols(self._sai_spec_directory)
        fboss_sai_symbols = self._get_sai_symbols(self._fboss_sai_api_directory)

        if self._verbose:
            print("##### SAI Spec symbols:")
            print("\n".join(sorted(sai_spec_symbols)))

            print("##### FBOSS SAI symbols:")
            print("\n".join(sorted(fboss_sai_symbols)))

            print("##### SAI symbols used by FBOSS:")

        common_symbols = sorted(sai_spec_symbols.intersection(fboss_sai_symbols))
        print("\n".join(common_symbols))


if __name__ == "__main__":
    args = parse_args()
    FindSaiSymbols(args).run()
