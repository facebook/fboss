#!/usr/bin/env python3

# Run as python3 fboss/util/run_sai_replayer_audit.py
#
#
# This script looks for SAI attributes that are currently not logged by Sai Replayer.
# It works as follows:
#   - find all attributes defined in *Api.h
#   - match them with attributes logged by Sai Replayer.
#   - If not, flag and printout the diff.

import subprocess
import sys

API_FILES_TO_SEARCH = "fboss/agent/hw/sai/api/*Api.h"
TRACER_FILES_TO_SEARCH = "fboss/agent/hw/sai/tracer/*ApiTracer.cpp"


def get_used_attributes():
    grep_cmd = f"grep SAI_ATTRIBUTE_NAME {API_FILES_TO_SEARCH}"
    grep_output = (
        subprocess.run(grep_cmd, shell=True, stdout=subprocess.PIPE)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )
    used_attributes = set()
    for line in grep_output:
        used_attributes.add(line.split("(")[1].split(")")[0])
    return used_attributes


def get_logged_attributes():
    grep_cmd = f"grep -e SAI_ATTR_MAP -e SAI_EXT_ATTR_MAP {TRACER_FILES_TO_SEARCH}"
    grep_output = (
        subprocess.run(grep_cmd, shell=True, stdout=subprocess.PIPE)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )
    logged_attributes = set()
    for line in grep_output:
        # Due to formatting, there could be multiple SAI_ATTR_MAP on single line
        attributes = line.split(", SAI_ATTR_MAP")
        for attr in attributes:
            logged_attributes.add(attr.split("(")[1].split(")")[0])
    return logged_attributes


if __name__ == "__main__":
    used_attributes = get_used_attributes()
    logged_attributes = get_logged_attributes()
    diff = used_attributes - logged_attributes
    if len(diff):
        print("Attributes missing from Sai Replayer:")
        for attr in sorted(diff):
            print(f"    {attr}")
        sys.exit(1)
    else:
        print("No missing attribute from Sai Replayer")
        sys.exit(0)
