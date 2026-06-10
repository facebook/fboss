#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import os
import subprocess


def load_from_file(file_path, profile=None):
    """Load list from a configuration file, skipping comment lines.

    Args:
        file_path: Path to the configuration file
        profile: Optional tag to filter the input file

    Returns:
        List of strings in the file
    """
    file_lines = []
    if os.path.exists(file_path):
        with open(file_path) as file:
            file_lines = []
            for line in file:
                stripped_line = line.strip()
                if not stripped_line or stripped_line.startswith("#"):
                    continue
                parts = stripped_line.split()
                pattern = parts[0]
                tags = parts[1:] if len(parts) > 1 else []
                if profile:
                    if profile not in tags:
                        continue
                # no --profile: include untagged lines and t-tagged lines
                elif tags and "t" not in tags:
                    continue
                file_lines.append(pattern)
    return file_lines


def run_script(script_file: str):
    if not os.path.exists(script_file):
        raise Exception(f"Script file {script_file} does not exist")
    if not os.access(script_file, os.X_OK):
        raise Exception(f"Script file {script_file} is not executable")
    subprocess.run(script_file, check=False, shell=True)
