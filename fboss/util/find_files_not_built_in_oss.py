#!/usr/bin/env python3

# Run as python3 find_files_not_in_fboss.py
#
#
# This scripts finds files that are open sourced but *not* built in open
# source. It works as follows:
#   - recursively traverse to find all open sourced files under fboss directory
#   - parse all cmakes to find the files are built.
#   - diff and print output.

import getpass
import os


FBCODE_DIR = os.path.join("/data/users", getpass.getuser(), "fbsource/fbcode")
FBOSS_DIR = os.path.join(FBCODE_DIR, "fboss")


def get_expected_oss_files():
    expected_oss_files = []

    prefix_len = len(FBCODE_DIR) + 1  # strip off FBCODE_DIR/
    for (dirpath, _, filenames) in os.walk(FBOSS_DIR):
        if "facebook" not in dirpath:
            source_files = [
                os.path.join(dirpath[prefix_len:], f)
                for f in filenames
                if f.endswith(".cpp")
            ]
            expected_oss_files.extend(source_files)

    return sorted(set(expected_oss_files))


def get_built_oss_files_in_file(cmake_file):
    built_oss_files_for_file = []
    with open(cmake_file) as file:
        line = file.readline()
        while line:
            if line.strip().endswith(".cpp"):
                built_oss_files_for_file.append(line.strip())
            line = file.readline()

    return sorted(set(built_oss_files_for_file))


def get_built_oss_files():
    CMAKELISTS_FILE = os.path.join(FBOSS_DIR, "github/CMakeLists.txt")
    CMAKE_DIR = os.path.join(FBOSS_DIR, "github/cmake")

    built_oss_files = []

    built_oss_files.extend(get_built_oss_files_in_file(CMAKELISTS_FILE))
    for cmake_file in os.listdir(CMAKE_DIR):
        if cmake_file.endswith(".cmake"):
            built_oss_files.extend(
                get_built_oss_files_in_file(os.path.join(CMAKE_DIR, cmake_file))
            )

    return sorted(set(built_oss_files))


if __name__ == "__main__":
    not_built_in_oss = set(get_expected_oss_files()) - set(get_built_oss_files())
    print(f"Number of OSS files not built in OSS: {len(not_built_in_oss)}")
    print("List of missing files:")
    print(*not_built_in_oss, sep="\n")
