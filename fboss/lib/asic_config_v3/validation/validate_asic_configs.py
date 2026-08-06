#!/usr/bin/env python3

"""
Validate ASIC config JSON files against their JSON schemas.

Usage:
    python3 -m fboss.lib.asic_config_v3.validation.validate_asic_configs \
        --fboss-root /path/to/fboss
"""

import argparse
import glob
import json
import os
import sys

import jsonschema
from fboss.lib.asic_config_v3.paths import AsicConfigPaths


def get_validation_targets(paths):
    # Each entry pairs a JSON schema with the config files it validates.
    return [
        {
            "description": "Vendor common configs",
            "schema": "vendor_common.schema.json",
            "files_glob": os.path.join(
                paths.asic_vendors_dir, "*", "*", "*_common.json"
            ),
        },
        {
            "description": "OCP SAI common config",
            "schema": "vendor_common.schema.json",
            "files_glob": os.path.join(
                paths.asic_vendors_dir, "common", "ocp_sai_common.json"
            ),
        },
        {
            "description": "Broadcom XGS ASIC configs",
            "schema": "broadcom_xgs_asic_config.schema.json",
            "files_glob": os.path.join(
                paths.asic_vendors_dir,
                "broadcom",
                "xgs",
                "asics",
                "tomahawk*.json",
            ),
        },
        {
            "description": "Platform configs",
            "schema": "platform_config.schema.json",
            "files_glob": os.path.join(
                paths.platforms_dir,
                "*",
                "*",
                "asic_config_v3",
                "asic_config.json",
            ),
        },
    ]


def validate(schema, config_path):
    with open(config_path) as f:
        config = json.load(f)
    jsonschema.validate(config, schema)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--fboss-root",
        type=str,
        required=True,
        help=(
            "Path to the fboss/ source directory itself, for example "
            "/path/to/fbcode/fboss."
        ),
    )
    args = parser.parse_args()
    paths = AsicConfigPaths.from_root(args.fboss_root)

    passed = 0
    failed = 0

    for target in get_validation_targets(paths):
        schema_path = os.path.join(paths.schemas_dir, target["schema"])
        with open(schema_path) as f:
            schema = json.load(f)

        files = sorted(glob.glob(target["files_glob"], recursive=True))

        print(f"{target['description']} ({target['schema']}):")

        if not files:
            print("  No files found.")
            continue

        for path in files:
            name = os.path.relpath(path, paths.configs_dir)
            try:
                validate(schema, path)
                print(f"  {name}: PASSED")
                passed += 1
            except jsonschema.ValidationError as e:
                print(f"  {name}: FAILED - {e.message}")
                failed += 1

        print()

    print(f"{passed} passed, {failed} failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
