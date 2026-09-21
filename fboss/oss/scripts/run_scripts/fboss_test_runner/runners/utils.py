#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import json
import os
import re
import subprocess


def test_matches_any_regex(test: str, regex_list: list[str]) -> bool:
    """Return True if ``test`` matches any regex in ``regex_list``."""
    return any(re.match(regex_pattern, test) for regex_pattern in regex_list)


def get_test_regexes_from_file(
    file_path: str,
    test_dict_key: str,
    keys_to_try: list[str],
) -> list[str]:
    """Extract test-name regexes from a benchmark/test config JSON file.

    Tries each key in ``keys_to_try`` against ``test_dict_key`` and merges the
    ``test_name_regex`` values from all matching keys.

    Args:
        file_path: Path to the JSON file containing test configurations
        test_dict_key: Top-level key holding the test dict (e.g.
            "known_bad_tests", "unsupported_tests")
        keys_to_try: Candidate keys to look up within the test dict

    Returns:
        List of test-name regex strings from all matching keys (may be empty)
    """
    if not os.path.exists(file_path):
        print(f"Warning: Test file {file_path} does not exist")
        return []

    with open(file_path) as f:
        test_json = json.load(f)
        test_dict = test_json.get(test_dict_key, {})

        test_regexes = set()
        for key in keys_to_try:
            for test_struct in test_dict.get(key, []):
                test_regexes.add(test_struct["test_name_regex"])

        # Warn only when the section has entries for other keys but none matched
        # -- that suggests a possibly mistyped key. An empty section (nothing
        # defined, e.g. a config with no known_bad_tests) is expected, not noise.
        if test_dict and not test_regexes:
            print(
                f"Warning: Could not find tests for key '{keys_to_try[0]}'. "
                f"Available keys: {list(test_dict.keys())}"
            )

        return list(test_regexes)


def load_from_file(file_path: str, profile: str | None = None) -> list[str]:
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


def run_script(script_file: str) -> None:
    if not os.path.exists(script_file):
        raise Exception(f"Script file {script_file} does not exist")
    if not os.access(script_file, os.X_OK):
        raise Exception(f"Script file {script_file} is not executable")
    subprocess.run(script_file, check=False, shell=True)
