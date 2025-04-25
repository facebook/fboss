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


def generate_platform_mappings() -> None:
    input_dir, output_dir, platform_name, is_multi_npu = get_command_line_args()

    print(f"Finding vendor data in {input_dir}...", file=sys.stderr)
    vendor_data_map = {platform_name: read_vendor_data(os.path.expanduser(input_dir))}

    if not vendor_data_map:
        print("No vendor data found in the input directory.", file=sys.stderr)
        exit(1)

    print("Generating platform mapping...", file=sys.stderr)
    platform_mapping = PlatformMappingV2(
        vendor_data_map, platform_name, is_multi_npu
    ).get_platform_mapping()

    output_dir = os.path.expanduser(output_dir)
    os.makedirs(output_dir, exist_ok=True)
    output_file = f"{output_dir}/{platform_name}_platform_mapping.json"
    platform_mapping_serialized = Serializer.serialize(
        TSimpleJSONProtocol.TSimpleJSONProtocolFactory(), platform_mapping
    )

    print(f"Writing to file {output_file}...", file=sys.stderr)
    with open(os.path.expanduser(output_file), "wb") as f:
        f.write(platform_mapping_serialized)


if __name__ == "__main__":
    generate_platform_mappings()
