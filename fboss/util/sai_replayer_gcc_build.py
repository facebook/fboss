#!/usr/bin/env python3

# Run as python3 sai_replayer_gcc_build.py
#
#
# This script modifies the internal sai replayer logs
# and builds sai replayer logs using GCC.
#
# For example:
# If SAI SDK, native SDK and PHY libs are built separately as
# different libs -
# python3 sai_replayer_gcc_build.py --sai_replayer_log sai_replayer.log
#                                   --sai_headers /path/to/sai/inc/,/path/to/sai/experimental
#                                   --sai_lib /path/to/libsai.a
#                                   --brcm_lib /path/to/libxgs_robo.a
#                                   --brcm_phymode_lib /path/to/libphymodepil.a
#                                   --brcm_epdm_lib /path/to/libepdm.a
#                                   --protobuf_lib /path/to/libprotobuf.a
#                                   --yaml_lib /path/to/libyaml.a
#
# If SAI SDK, native SDK and PHY libs are built together and linked as
# one static lib (e.g libsai_impl.a) -
# python3 sai_replayer_gcc_build.py --sai_replayer_log sai_replayer.log
#                                   --sai_headers /path/to/sai/inc/
#                                   --sai_lib /path/to/libsai_impl.a

import argparse
import os
import subprocess


HEADERS = """#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <sai.h>
}"""

SAI_REPLAYER_CPP = "sai_replayer.cpp"


def process_log(log_path, output_path):
    with open(log_path) as log_file:
        with open(output_path, "w") as output_file:
            write_include = False

            for line in log_file:
                # Modify includes
                if line.startswith("#include"):
                    if not write_include:
                        write_include = True
                        output_file.write(HEADERS)
                # Rewrite run_trace() into main function
                elif line.startswith("void run_trace()"):
                    output_file.write("int main() {\n")
                # Skip fboss namespace
                elif "namespace facebook::fboss" not in line:
                    output_file.write(line)


def build_binary(cpp_paths, sai_headers, *libraries):
    sai_header_list = "-I " + " -I ".join(sai_headers.split(","))
    flags = " -lm -lpthread -lrt -lstdc++ -ldl"

    print("\nCompiling files...")
    for cpp_path in cpp_paths:
        command = f"gcc -c {cpp_path} " + sai_header_list + flags
        print(f"  {command}")
        subprocess.run(command, shell=True, check=True)

    # Get list of built object files in current directory
    command = "gcc " + " ".join(
        [os.path.basename(file).replace("cpp", "o") for file in cpp_paths]
    )
    for lib in libraries:
        if lib is not None:
            command += f" {lib}"
    command += " " + sai_header_list + flags
    print(f"\nLinking files... \n  {command}")
    subprocess.run(command, shell=True, check=True)

    print("\nDone. Executable: a.out")


def main():
    psr = argparse.ArgumentParser(
        description="This script modifies the internal sai replayer logs "
        "and builds sai replayer logs using GCC."
    )
    psr.add_argument(
        "--sai_replayer_log",
        required=False,
        type=str,
        default="sai_replayer.log",
        help="Input sai replayer log generated from runtime. This should be "
        "the log file produced by FBOSS agent.",
    )
    psr.add_argument(
        "--sai_headers",
        required=True,
        type=str,
        help="Comma-separated list of SAI header file directories.",
    )
    psr.add_argument("--sai_lib", required=True, type=str)
    psr.add_argument("--brcm_lib", required=False, type=str)
    psr.add_argument("--brcm_phymode_lib", required=False, type=str)
    psr.add_argument("--brcm_epdm_lib", required=False, type=str)
    psr.add_argument("--protobuf_lib", required=False, type=str)
    psr.add_argument(
        "--yaml_lib",
        required=False,
        type=str,
        help="yaml_lib is required after 5.0*",
    )
    args = psr.parse_args()

    # Process Sai Replayer Log from internal build form
    # to standalone main function.
    split_logs = os.path.isdir(args.sai_replayer_log)
    if split_logs:
        cpp_files = [
            os.path.join(args.sai_replayer_log, file)
            for file in os.listdir(args.sai_replayer_log)
            if file.endswith("cpp")
        ]
        print(
            "\nMultiple SAI log files found. Source files:\n  " + "\n  ".join(cpp_files)
        )

    else:
        print("\nPreprocessing " + args.sai_replayer_log + "...")
        process_log(args.sai_replayer_log, SAI_REPLAYER_CPP)
        cpp_files = [SAI_REPLAYER_CPP]

    # Compile and link the binary from libraries provided.
    try:
        build_binary(
            cpp_files,
            args.sai_headers,
            args.sai_lib,
            args.brcm_lib,
            args.brcm_phymode_lib,
            args.brcm_epdm_lib,
            args.protobuf_lib,
            args.yaml_lib,
        )
    except RuntimeError as e:
        print(e)
    finally:
        # Delete the cpp file
        if not split_logs:
            os.remove(SAI_REPLAYER_CPP)


if __name__ == "__main__":
    main()
