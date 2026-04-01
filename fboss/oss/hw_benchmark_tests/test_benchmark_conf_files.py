#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.

"""
Validates that T1, T2, and additional benchmark configuration files
do not have overlaps.
"""

import itertools
import pathlib
import sys

import pytest

# Add run_scripts to the path so run_test and its sibling modules (e.g.
# fboss_agent_utils) can be imported directly
sys.path.insert(
    0,
    str(pathlib.Path(__file__).resolve().parent.parent / "scripts" / "run_scripts"),
)
from run_test import _load_from_file

# Configuration files to check for overlaps
CONF_FILES = [
    "t1_benchmarks.conf",
    "t2_benchmarks.conf",
    "additional_benchmarks.conf",
]


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

    benchmarks1 = set(_load_from_file(conf_file1))
    benchmarks2 = set(_load_from_file(conf_file2))

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


def test_no_duplicates_within_files():
    """Test that no benchmark configuration file contains duplicate entries."""
    for filename in CONF_FILES:
        conf_file = get_conf_file_path(filename)
        benchmarks = _load_from_file(conf_file)
        assert len(set(benchmarks)) == len(benchmarks), (
            f"Found duplicates in {filename}: "
            f"{[b for b in benchmarks if benchmarks.count(b) > 1]}"
        )


def test_no_empty_files():
    """Test that all benchmark configuration files contain at least one benchmark."""
    for filename in CONF_FILES:
        conf_file = get_conf_file_path(filename)
        benchmarks = _load_from_file(conf_file)
        assert benchmarks, (
            f"Configuration file is empty or contains only comments: {filename}"
        )


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
