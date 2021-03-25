#!/usr/bin/env python3

# Run as python3 run_bcm_cint_audit.py
#
#
# This script looks for brcm APIs that are currently not logged by Bcm Cinter.
# It works as follows:
#   - recursively traverse to find all lines under agent/hw that matches
#     the bcm API regex.
#   - remove the duplicate ones
#   - diff with wrapped symbols (APIs logged by Bcm Cinter) and print output.

import subprocess

ROOT_DIR_TO_SEARCH = "fboss/agent/hw/bcm"
PATTERNS_TO_SEARCH = [r"'bcm_\S*[a-z,A-Z,0-9]('"]
PATTENRS_TO_OMIT = [
    "SdkTracer",
    "BcmSdkInterface",
    "BcmCinter",
    "BcmInterface",
    "TARGETS",
    "bzl",
]
PATTERNS_TO_REPLACE = ["'s/^.*bcm_/bcm_/'", "'s/(.*$//'"]
PATTERNS_TO_OMIT_AFTER_REPLACEMENT = [
    r"'\.'",
    "'_t[*,(,),<,>,$]'",
    "_get",
    "_list",
    "_traverse",
]

WRAPPED_SYMBOLS = "fboss/agent/hw/bcm/wrapped_symbols.bzl"
WRAPPED_SYMBOLS_TO_SEARCH = [
    "'wrap=" + search_pattern[1:-2] + "'" for search_pattern in PATTERNS_TO_SEARCH
]  # Remove trailing close bracket
WRAPPED_SYMBOLS_TO_REPLACE = ["'s/.*--wrap=//'", "'s/\",//'"]


def get_used_apis():
    command = "("
    # search all the files
    for search_pattern in PATTERNS_TO_SEARCH:
        command = command + f"grep -r {search_pattern} {ROOT_DIR_TO_SEARCH};"
    command += ")"

    # omit certain patterns
    for omit_pattern in PATTENRS_TO_OMIT:
        command += f" | grep -v {omit_pattern}"

    # strip the line
    for replace_pattern in PATTERNS_TO_REPLACE:
        command += f" | sed {replace_pattern}"

    # omit some other patterns after replacement
    for omit_pattern_after_replacement in PATTERNS_TO_OMIT_AFTER_REPLACEMENT:
        command += f" | grep -v {omit_pattern_after_replacement}"

    return set(
        subprocess.run(command, shell=True, capture_output=True)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )


def get_wrapped_apis():
    command = "("
    # search in wrapped_symbols.bzl
    for search_pattern in WRAPPED_SYMBOLS_TO_SEARCH:
        command = command + f"grep {search_pattern} {WRAPPED_SYMBOLS};"
    command += ")"

    # strip the line
    for replace_pattern in WRAPPED_SYMBOLS_TO_REPLACE:
        command += f" | sed {replace_pattern}"

    return set(
        subprocess.run(command, shell=True, capture_output=True)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )


if __name__ == "__main__":
    used_apis = get_used_apis()
    wrapped_apis = get_wrapped_apis()
    diff = used_apis - wrapped_apis
    print(f"There're {len(diff)} Bcm APIs that are not supported by Bcm Cinter.\n")
    print(*sorted(used_apis - wrapped_apis), sep="\n")
