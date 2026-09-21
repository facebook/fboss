#!/usr/bin/env python3
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#

import argparse
import sys
from pathlib import Path

import yaml

HEADER_TEMPLATE = """# Automatically generated file; DO NOT EDIT.
# Generated for kernel version {kernel_version}

"""


def load_yaml_config(yaml_path):
    """Load and parse YAML configuration file."""
    with open(yaml_path, "r") as f:
        return yaml.safe_load(f)


def generate_config_content(config_data, kernel_version):
    """Generate kernel config content for specified version."""
    lines = [HEADER_TEMPLATE.format(kernel_version=kernel_version)]
    # Add common overrides
    if "common_overrides" in config_data and config_data["common_overrides"]:
        common_content = config_data["common_overrides"].strip()
        if common_content:
            lines.extend(common_content.split("\n"))
            lines.append("")
    # Add version-specific overrides
    version_overrides = config_data.get("kernel_version_overrides", {})
    if kernel_version in version_overrides and version_overrides[kernel_version]:
        version_content = version_overrides[kernel_version].strip()
        if version_content:
            lines.extend(version_content.split("\n"))
            lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Generate kernel config overrides from YAML"
    )
    parser.add_argument("yaml_file", help="Path to YAML configuration file")
    parser.add_argument("kernel_version", help="Kernel version (e.g., 6.4.3)")
    parser.add_argument("output_file", help="Output config file path")
    args = parser.parse_args()
    try:
        config_data = load_yaml_config(args.yaml_file)
        content = generate_config_content(config_data, args.kernel_version)
        Path(args.output_file).write_text(content)
    except FileNotFoundError as e:
        print(f"Error: File not found: {e.filename}", file=sys.stderr)
        sys.exit(1)
    except yaml.YAMLError as e:
        print(f"Error: Invalid YAML: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
