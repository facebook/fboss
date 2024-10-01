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
#                                   --sai_headers /path/to/sai/inc/
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


def build_binary(cpp_path, sai_header, *libraries):
    command = f"gcc {cpp_path} -I {sai_header} -lm -lpthread -lrt -lstdc++ -ldl"
    for lib in libraries:
        if lib is not None:
            command += f" {lib}"
    print(f"\n{command}")
    subprocess.run(command, shell=True)


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
    psr.add_argument("--sai_headers", required=True, type=str)
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
    process_log(args.sai_replayer_log, SAI_REPLAYER_CPP)

    # Compile and link the binary from libraries provided.
    try:
        build_binary(
            SAI_REPLAYER_CPP,
            args.sai_headers,
            args.sai_lib,
            args.brcm_lib,
            args.brcm_phymode_lib,
            args.brcm_epdm_lib,
            args.protobuf_lib,
            args.yaml_lib,
        )
    except RuntimeError:
        os.remove(SAI_REPLAYER_CPP)

    # Delete the cpp file
    os.remove(SAI_REPLAYER_CPP)


if __name__ == "__main__":
    main()
