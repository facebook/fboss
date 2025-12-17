# pyre-strict
import argparse
import os
import sys
from typing import Dict, Tuple

from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingV2
from thrift.protocol import TSimpleJSONProtocol
from thrift.util import Serializer

_FBOSS_DIR: str = os.getcwd() + "/fboss"
INPUT_DIR: str = f"{_FBOSS_DIR}/lib/platform_mapping_v2/platforms/"


def get_command_line_args() -> Tuple[str, str, str, bool]:
    parser = argparse.ArgumentParser(description="OSS platform mapping generation.")
    parser.add_argument(
        "--input-dir",
        type=str,
        default=INPUT_DIR,  # temporary location until platform name is read in
        required=False,
        help="Path to input directory holding CSVs (default: fboss/lib/platform_mapping_v2/platforms/PLATFORM_NAME)",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=f"{_FBOSS_DIR}/lib/platform_mapping_v2/generated_platform_mappings/",
        required=False,
        help="Path to output directory for JSON (default: fboss/lib/platform_mapping_v2/generated_platform_mappings/)",
    )
    parser.add_argument(
        "--platform-name",
        type=str,
        required=True,
        help="Name of platform that each CSV file has as prefix",
    )
    parser.add_argument(
        "--multi-npu",
        action="store_true",
        help="Generate multi-NPU platform mapping (default: False)",
    )

    args = parser.parse_args()
    return (
        args.input_dir + args.platform_name,
        args.output_dir,
        args.platform_name,
        args.multi_npu,
    )


def read_vendor_data(input_file_path: str) -> Dict[str, str]:
    vendor_data = {}
    if not os.path.exists(input_file_path):
        raise FileNotFoundError(f"The folder '{input_file_path}' does not exist.")

    for filename in os.listdir(input_file_path):
        filepath = os.path.join(input_file_path, filename)
        if (
            filepath.endswith(".csv") or filepath.endswith(".json")
        ) and not os.path.isdir(filepath):
            with open(filepath, "r") as file:
                content = file.read()
            vendor_data[filename] = content

    return vendor_data


def generate_platform_mappings(
    input_dir: str, output_dir: str, platform_name: str, is_multi_npu: bool
) -> None:
    print(f"Finding vendor data in {input_dir}...", file=sys.stderr)
    input_dir = os.path.expanduser(input_dir)
    vendor_data_map = read_all_vendor_data()

    if not vendor_data_map:
        print("No vendor data found in the input directory.", file=sys.stderr)
        exit(1)

    print("Generating platform mapping...", file=sys.stderr)
    platform_mapping = PlatformMappingV2(
        vendor_data_map, platform_name, is_multi_npu
    ).get_platform_mapping()

    output_dir = os.path.expanduser(output_dir)
    os.makedirs(output_dir, exist_ok=True)
    platform_file_name = f"{platform_name}_platform_mapping" + (
        "_is_multi_npu" if is_multi_npu else ""
    )
    output_file = f"{output_dir}/{platform_file_name}.json"
    platform_mapping_serialized = Serializer.serialize(
        TSimpleJSONProtocol.TSimpleJSONProtocolFactory(), platform_mapping
    )

    # Ensure that files end in a newline (we have pre-commit hooks in OSS that
    # will fail if this isn't done.)
    if platform_mapping_serialized[-1] != b"\n":
        platform_mapping_serialized += b"\n"

    print(f"Writing to file {output_file}...", file=sys.stderr)
    with open(os.path.expanduser(output_file), "wb") as f:
        f.write(platform_mapping_serialized)


def generate_mappings_without_args() -> None:
    generate_platform_mappings(*get_command_line_args())


def read_all_vendor_data() -> Dict[str, Dict[str, str]]:
    all_vendor_data = {}
    data_path = INPUT_DIR
    print(
        f"Reading all vendor data in {data_path}...",
        file=sys.stderr,
    )
    for filename in os.listdir(data_path):
        filepath = os.path.join(data_path, filename)
        if not os.path.isdir(filepath):
            continue
        all_vendor_data[filename] = read_vendor_data(filepath)

    return all_vendor_data


if __name__ == "__main__":
    generate_mappings_without_args()
