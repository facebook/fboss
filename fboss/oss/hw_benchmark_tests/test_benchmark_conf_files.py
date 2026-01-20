#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.

"""
Validates that T1, T2, and additional benchmark configuration files
do not have overlaps.
"""

import itertools
import os
import pathlib
import pytest

# Configuration files to check for overlaps
CONF_FILES = [
    "t1_benchmarks.conf",
    "t2_benchmarks.conf",
    "additional_benchmarks.conf",
]


def load_benchmarks_from_file(filepath):
    """Load benchmark names from a configuration file, ignoring comments and empty lines."""
    benchmarks = []
    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            # Skip empty lines and comments
            if line and not line.startswith("#"):
                benchmarks.append(line)
    return set(benchmarks)


def get_conf_file_path(filename):
    """Get the path to a benchmark configuration file."""
    # Try to find the file relative to this test file
    test_dir = pathlib.Path(__file__).parent
    conf_file = test_dir / filename

    if conf_file.exists():
        return conf_file

    # Fallback: try relative to current working directory
    conf_file = pathlib.Path(filename)
    if conf_file.exists():
        return conf_file

    raise FileNotFoundError(f"Configuration file not found: {filename}")


@pytest.mark.parametrize(
    "file1,file2",
    list(itertools.combinations(CONF_FILES, 2)),
)
def test_no_overlap(file1, file2):
    """Test that benchmark files don't have overlaps."""
    conf_file1 = get_conf_file_path(file1)
    conf_file2 = get_conf_file_path(file2)

    benchmarks1 = load_benchmarks_from_file(conf_file1)
    benchmarks2 = load_benchmarks_from_file(conf_file2)

    # Find overlaps
    overlaps = benchmarks1 & benchmarks2

    assert not overlaps, (
        f"Found overlaps between {file1} and {file2}: {overlaps}\n"
        f"Each benchmark should only appear in one configuration file."
    )


def test_all_files_exist():
    """Test that all required benchmark configuration files exist."""
    for filename in CONF_FILES:
        conf_file = get_conf_file_path(filename)
        assert conf_file.exists(), f"Required configuration file not found: {filename}"


def test_no_empty_files():
    """Test that all benchmark configuration files contain at least one benchmark."""
    for filename in CONF_FILES:
        conf_file = get_conf_file_path(filename)
        benchmarks = load_benchmarks_from_file(conf_file)
        assert (
            benchmarks
        ), f"Configuration file is empty or contains only comments: {filename}"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
