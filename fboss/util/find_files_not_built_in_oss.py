#!/usr/bin/env python3

# pyre-unsafe

# Run as python3 find_files_not_in_fboss.py
#
#
# This scripts finds files that are open sourced but *not* built in open
# source. It works as follows:
#   - recursively traverse to find all open sourced files under fboss directory
#   - parse all cmakes to find the files are built.
#   - diff and print output.

import argparse
import getpass
import os


DEFAULT_FBCODE_DIR = os.path.join("/data/users", getpass.getuser(), "fbsource/fbcode")
SKIPPED_DIRPATH_LIST = [
    "facebook",  # not open sourced
    "agent/hw/bcm",  # only work with native Brcm SDK
]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--fbcode_dir",
        help=f"FBCode directory. Default={DEFAULT_FBCODE_DIR}",
        default=DEFAULT_FBCODE_DIR,
    )
    return parser.parse_args()


def get_expected_oss_files(fbcode_dir, fboss_dir):
    expected_oss_files = []

    prefix_len = len(fbcode_dir) + 1  # strip off FBCODE_DIR/
    for dirpath, _, filenames in os.walk(fboss_dir):
        if all(
            skipped_dirpath not in dirpath for skipped_dirpath in SKIPPED_DIRPATH_LIST
        ):
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


def get_built_oss_files(fboss_dir):
    CMAKELISTS_FILE = os.path.join(fboss_dir, "github/CMakeLists.txt")
    CMAKE_DIR = os.path.join(fboss_dir, "github/cmake")

    built_oss_files = []

    built_oss_files.extend(get_built_oss_files_in_file(CMAKELISTS_FILE))
    for cmake_file in os.listdir(CMAKE_DIR):
        if cmake_file.endswith(".cmake"):
            built_oss_files.extend(
                get_built_oss_files_in_file(os.path.join(CMAKE_DIR, cmake_file))
            )

    return sorted(set(built_oss_files))


def main(args) -> None:
    fbcode_dir = args.fbcode_dir
    fboss_dir = os.path.join(fbcode_dir, "fboss")
    not_built_in_oss = sorted(
        set(get_expected_oss_files(fbcode_dir, fboss_dir))
        - set(get_built_oss_files(fboss_dir))
    )
    print(f"Number of OSS files not built in OSS: {len(not_built_in_oss)}")
    print("List of missing files:")
    print(*not_built_in_oss, sep="\n")


if __name__ == "__main__":
    args = parse_args()
    main(args)
