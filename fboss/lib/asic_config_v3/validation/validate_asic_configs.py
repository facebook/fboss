#!/usr/bin/env python3

"""
Validate ASIC config JSON files against their JSON schemas.

Usage:
    python3 validate_asic_configs.py
"""

import glob
import json
import os
import sys

import jsonschema

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODULE_DIR = os.path.dirname(SCRIPT_DIR)
SCHEMAS_DIR = os.path.join(MODULE_DIR, "schemas")
VENDORS_DIR = os.path.join(MODULE_DIR, "vendors")
PLATFORMS_DIR = os.path.join(MODULE_DIR, "platforms")

# Each entry pairs a JSON schema with the set of config files validated against it.
VALIDATION_TARGETS = [
    {
        "description": "Vendor common configs",
        "schema": "vendor_common.schema.json",
        "files_glob": os.path.join(VENDORS_DIR, "**", "*_common.json"),
    },
    {
        "description": "Broadcom XGS ASIC configs",
        "schema": "broadcom_xgs_asic_config.schema.json",
        "files_glob": os.path.join(
            VENDORS_DIR, "broadcom", "xgs", "asics", "tomahawk*.json"
        ),
    },
    {
        "description": "Platform configs",
        "schema": "platform_config.schema.json",
        "files_glob": os.path.join(PLATFORMS_DIR, "**", "asic_config.json"),
    },
]


def validate(schema, config_path):
    with open(config_path) as f:
        config = json.load(f)
    jsonschema.validate(config, schema)


def main():
    passed = 0
    failed = 0

    for target in VALIDATION_TARGETS:
        schema_path = os.path.join(SCHEMAS_DIR, target["schema"])
        with open(schema_path) as f:
            schema = json.load(f)

        files = sorted(glob.glob(target["files_glob"], recursive=True))

        print(f"{target['description']} ({target['schema']}):")

        if not files:
            print("  No files found.")
            continue

        for path in files:
            name = os.path.relpath(path, MODULE_DIR)
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
