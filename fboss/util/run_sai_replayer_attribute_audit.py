#!/usr/bin/env python3

# Run as python3 run_sai_replayer_attribute_audit.py
#
#
# This script looks for Sai attributes that are not logged by Sai Replayer.
# It works as follows:
#   - For each Sai API, go to /agent/hw/sai/api/*Api.h to search for
#     Sai Attributes.
#   - Compare the ones appeared in /agent/hw/sai/tracer/*ApiTracer.h.
#   - Print out those attributes not exist in Sai Replayer.

import re
import subprocess


def get_attr(file_path):
    grep_command = f"grep -r '_ATTR_' {file_path}"
    grep_lines = (
        subprocess.run(grep_command, shell=True, capture_output=True)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )

    attributes = set()

    for line in grep_lines:
        for match in re.findall(r"SAI_\w+_ATTR\w+", line):
            attributes.add(match)

    return attributes


if __name__ == "__main__":
    used_attributes = get_attr("fboss/agent/hw/sai/api/*Api.h")
    logged_attributes = get_attr("fboss/agent/hw/sai/tracer/*ApiTracer.cpp")

    diff = used_attributes - logged_attributes
    print("Attributes not logged by Sai Replayer:\n")
    print(*sorted(diff), sep="\n")
