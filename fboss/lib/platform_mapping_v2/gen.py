# pyre-strict
import argparse
import os
from typing import Dict, Tuple


def get_command_line_args() -> Tuple[str, str, str, bool]:
    parser = argparse.ArgumentParser(description="OSS platform mapping generation.")
    parser.add_argument(
        "--input-dir",
        type=str,
        required=True,
        help="Path to input directory holding CSVs",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default="/tmp/generated_platform_mappings",
        required=False,
        help="Path to output directory for JSON (default: /tmp/generated_platform_mappings)",
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
    return args.input_dir, args.output_dir, args.platform_name, args.multi_npu


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
