#!/usr/bin/env python3
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#
# This Python script parses a single SAI replayer log file and splits it
# across multiple C++ files with a single header file and main file
# for ease of compilation and execution.

import argparse
import os
import re
import shutil

from enum import Enum

# Config
DEFAULT_INPUT_DIR = "./"
DEFAULT_OUTPUT_DIR = "output"
DEFAULT_MAX_LINES = 400000
MIN_LINES = 20000

# Global variables
GENERATED_FILES = []
GENERATED_FILES_SAILOGCPP = []

# Regexes
REGEX_COMMENT = r"^(\/\/|\/\*|\*)"
REGEX_CPP_FILE = r"\.cpp$"
REGEX_VARS_BEGIN = r"Global Variables Start"
REGEX_VARS_END = r"Global Variables End"
REGEX_UNNAMED_NAMESPACE_BEGIN = r"^namespace( )*{$"
REGEX_UNNAMED_NAMESPACE_END = r"^}( )*\/\/( )*namespace$"
REGEX_RUN_TRACE_FUNC = r"^void run\_trace\(\) ?{$"
REGEX_INCLUDES = r"^\#include"
REGEX_DEFINES = r"^\#define"
REGEX_SAILOG_INCLUDE = r"^\#include.*SaiLog\.h"
REGEX_HASHAPIVAR = r"hash\_api(?![a-zA-Z0-9\_\-])"
REGEX_SAILOGCPP = r"\"SaiLog\.cpp\""

# Code snippets
LICENCE = """/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */"""
INCLUDE_SAILOG_H = '#include "SaiLog.h"'
HEADER_INCLUDES = """
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
"""
INCLUDES_EXTERN = """
extern "C" {
#include <sai.h>
}
"""
NAMESPACE_START = "namespace facebook::fboss {"
NAMESPACE_END = "} // namespace facebook::fboss"

# Functions


class Section(Enum):
    NORMAL = 1
    VARIABLES = 2
    UNNAMED_NAMESPACE = 3


def init_argparser():
    parser = argparse.ArgumentParser(
        description="Post processing for SAI replayer logs."
    )
    parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
        help="Remove the existing output directory, if it exists.",
    )
    parser.add_argument(
        "-i",
        "--input",
        help="Input directory with SaiLog.cpp and run.bzl files. Default: "
        + DEFAULT_INPUT_DIR,
        nargs="?",
        default=DEFAULT_INPUT_DIR,
    )
    parser.add_argument(
        "-l",
        "--max_lines",
        help="Maximum lines per file. Default: " + str(DEFAULT_MAX_LINES),
        nargs="?",
        type=int,
        default=DEFAULT_MAX_LINES,
    )
    return parser


def prepare_output_dir(output_dir, force_clean):
    if os.path.isdir(output_dir):
        if not force_clean:
            confirm = input(
                "Files exist in directory " + output_dir + ". Delete? (y/N) "
            )
            if confirm != "y":
                exit("Exiting.")
        print("Removing existing output directory...")
        shutil.rmtree(output_dir)
    os.mkdir(output_dir)


def index_of_var_name(line):
    # Find equals sign and move backwards until a space or * is found
    equals_index = line.find("=")
    if equals_index == -1:
        print("index_of_var_name - line: " + line + ", no index")
        return -1
    for i in range(equals_index, 0, -1):
        if line[i] == " " or line[i] == "*":
            return i + 1
    return 0


def is_cpp_file(filename):
    return re.search(REGEX_CPP_FILE, filename) is not None


def get_cpp_count(file_list):
    return sum(1 for file in file_list if is_cpp_file(file))


def get_file_intro():
    return LICENCE + "\n\n" + INCLUDE_SAILOG_H + "\n"


def get_file_outro():
    return "\n}\n\n" + NAMESPACE_END + "\n"


def open_new_output_cpp(output_dir, output_file_num):
    output_file = "SaiLog{}.cpp".format(output_file_num)
    GENERATED_FILES_SAILOGCPP.append(output_file)

    output_file_relative = os.path.join(output_dir, output_file)
    GENERATED_FILES.append(output_file_relative)
    return open(output_file_relative, "w")


def generate_header_file(contents, output_dir):
    filename = output_dir + "/SaiLog.h"
    with open(filename, "w") as file:
        file.write(LICENCE + "\n\n")
        file.write("#pragma once\n\n")
        file.write(HEADER_INCLUDES + "\n\n")
        file.write(INCLUDES_EXTERN + "\n\n")
        file.write("\n".join(contents) + "\n\n")

        file.write(NAMESPACE_START + "\n\n")
        for i in range(1, get_cpp_count(GENERATED_FILES) + 1):
            file.write("void run_trace_{}();\n".format(i))

        file.write("\n" + NAMESPACE_END + "\n")

    GENERATED_FILES.insert(0, filename)


def generate_main_file(variables, output_dir):
    filename = output_dir + "/Main.cpp"
    with open(filename, "w") as file:
        file.write(get_file_intro())
        file.write(INCLUDES_EXTERN + "\n\n")
        file.write("\n".join(variables) + "\n\n")
        file.write("int main(){\n")
        for i in range(1, get_cpp_count(GENERATED_FILES) + 1):
            file.write("  facebook::fboss::run_trace_{}();\n".format(i))
        file.write("}\n")

    GENERATED_FILES.insert(0, filename)


def generate_run_bzl(input_file, output_dir):
    cpp_file_list = "".join(
        '            "' + os.path.basename(file) + '",\n'
        for file in GENERATED_FILES_SAILOGCPP
    )

    filename = output_dir + "/run.bzl"
    GENERATED_FILES.insert(0, filename)

    with open(filename, "w") as output_file:
        for line in open(input_file, "r"):
            # Replace SaiLog.cpp with the list of SaiLog*.cpp files
            if re.search(REGEX_SAILOGCPP, line) is not None:
                output_file.write(cpp_file_list)
            else:
                output_file.write(line)


def generate_targets_file(output_dir):
    filename = output_dir + "/TARGETS"
    GENERATED_FILES.insert(0, filename)
    with open(filename, "w") as file:
        file.write(
            """
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/tracer/run/"""
            + output_dir
            + """:run.bzl", "all_replayer_binaries")

oncall("fboss_agent_push")

all_replayer_binaries()

cpp_library(
    name = "recursive_glob_headers",
    headers = ["SaiLog.h"],
    labels = ["noautodeps"],
)
            """
        )


def split_file(input_file, output_dir, max_lines):
    includes_defines = []
    variables = []
    unnamed_namespace = []

    section = Section.NORMAL
    section_prev = section

    line_num = 0
    output_file_num = 1
    output = open_new_output_cpp(output_dir, output_file_num)

    pre_code_section = True

    for line in open(input_file, "r"):
        line = line.strip()

        # Add SaiLog.h import to the first non-comment line
        if pre_code_section and re.search(REGEX_COMMENT, line) is None:
            pre_code_section = False
            output.write("\n\n" + INCLUDE_SAILOG_H + "\n")

        # Start or end a special section
        if re.search(REGEX_VARS_BEGIN, line) is not None:
            section_prev = section
            section = Section.VARIABLES
            continue
        if re.search(REGEX_VARS_END, line) is not None:
            section, section_prev = section_prev, section
            section = Section.NORMAL
            continue
        if re.search(REGEX_UNNAMED_NAMESPACE_BEGIN, line) is not None:
            unnamed_namespace.append(line)
            section_prev = section
            section = Section.UNNAMED_NAMESPACE
            continue
        if re.search(REGEX_UNNAMED_NAMESPACE_END, line) is not None:
            unnamed_namespace.append(line)
            section, section_prev = section_prev, section
            continue

        # Preprocessing
        if re.search(REGEX_HASHAPIVAR, line) is not None:
            # Avoid duplicate symbol from brcm_sai_common.h
            line = re.sub(REGEX_HASHAPIVAR, "hash_api_var", line)

        # Save header file contents
        if re.search(REGEX_SAILOG_INCLUDE, line) is not None:
            continue
        if (
            re.search(REGEX_INCLUDES, line) is not None
            or re.search(REGEX_DEFINES, line) is not None
        ):
            includes_defines.append(line)
            continue
        if section == Section.UNNAMED_NAMESPACE:
            unnamed_namespace.append(line)
            continue
        if section == Section.VARIABLES:
            assign = line.find("=")
            if assign == -1:
                # Declaration only
                variables.append(line)
            else:
                # Separate definition from declaration
                variables.append(line[:assign] + ";")
                line = line[index_of_var_name(line) :]

        # Modify the run_trace line
        if re.search(REGEX_RUN_TRACE_FUNC, line) is not None:
            output.write("void run_trace_1() {\n\n")
            continue

        # Spill to next CPP file
        if line_num > max_lines:
            output.write(get_file_outro())
            output.close()

            output_file_num += 1
            output = open_new_output_cpp(output_dir, output_file_num)
            output.write(get_file_intro())
            output.write(NAMESPACE_START + "\n\n")
            output.write("void run_trace_{}() {{\n\n".format(output_file_num))

            line_num = 0

        output.write(line + "\n")
        line_num += 1

    # Outro already written from source file
    output.close()

    extern_variables = [("extern " + var) for var in variables]

    generate_header_file(
        includes_defines + ["\n"] + extern_variables + ["\n"] + unnamed_namespace,
        output_dir,
    )
    generate_main_file(variables, output_dir)
    generate_targets_file(output_dir)


def print_summary(input_sailog, input_bzl, output_dir, max_lines):
    print("\nSUMMARY")
    print("Source files: " + input_sailog + ", " + input_bzl)
    print("Max number of lines per file: " + str(max_lines))
    print("Generated {} files:".format(len(GENERATED_FILES)))
    for f in GENERATED_FILES:
        print("  " + f)


def main():
    args = init_argparser().parse_args()

    input_sailog = os.path.join(args.input, "SaiLog.cpp")
    input_runbzl = os.path.join(args.input, "run.bzl")

    # Validate input
    if not os.path.exists(args.input):
        exit("Invalid input directory: " + args.input)
    if not os.path.isdir(args.input):
        exit("Input directory is not valid: " + args.input)

    if not os.path.exists(input_sailog):
        exit("Input file not found: " + input_sailog)
    if not os.path.isfile(input_sailog):
        exit(input_sailog + " is not a file.")

    if not os.path.exists(input_runbzl):
        exit("Input file not found: " + input_runbzl)
    if not os.path.isfile(input_runbzl):
        exit(input_runbzl + " is not a file.")

    if args.max_lines < MIN_LINES:
        exit("Minimum value for max lines per file is " + str(MIN_LINES) + ".")

    output_dir = DEFAULT_OUTPUT_DIR

    prepare_output_dir(output_dir, args.clean)
    split_file(input_sailog, output_dir, args.max_lines)
    generate_run_bzl(input_runbzl, output_dir)
    print_summary(input_sailog, input_runbzl, output_dir, args.max_lines)


if __name__ == "__main__":
    main()
