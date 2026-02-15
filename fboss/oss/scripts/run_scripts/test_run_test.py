#!/usr/bin/env python3
# Copyright Meta Platforms, Inc. and affiliates.

"""
Unit tests for run_test.py using pytest framework.

Provides comprehensive coverage of the BenchmarkTestRunner class,
including benchmark loading, parsing, execution, and error handling.
"""

import os
import sys
import tempfile
import subprocess
from unittest.mock import Mock, patch, MagicMock, call

import pytest

from run_test import BenchmarkTestRunner


# Fixtures


@pytest.fixture
def runner():
    """Create a BenchmarkTestRunner instance for testing"""
    return BenchmarkTestRunner()


@pytest.fixture
def mock_args():
    """Create a mock args object with common attributes"""
    args = Mock()
    args.filter_file = None
    args.list_tests = False
    args.config = None
    args.mgmt_if = "eth0"
    args.platform_mapping_override_path = None
    args.fruid_path = None
    args.sai_logging = "WARN"
    args.fboss_logging = "WARN"
    args.test_run_timeout = 300
    return args


@pytest.fixture
def temp_benchmark_file():
    """Create a temporary benchmark configuration file"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        f.write("# Comment line\n")
        f.write("benchmark1\n")
        f.write("\n")
        f.write("benchmark2\n")
        f.write("# Another comment\n")
        f.write("benchmark3\n")
        temp_file = f.name

    yield temp_file

    # Cleanup
    if os.path.exists(temp_file):
        os.unlink(temp_file)


# Tests for _get_benchmarks_to_run


def test_get_benchmarks_with_filter_file(runner, temp_benchmark_file):
    """Test loading benchmarks from a custom filter file"""
    benchmarks = runner._get_benchmarks_to_run(temp_benchmark_file)
    assert benchmarks == ["benchmark1", "benchmark2", "benchmark3"]


def test_get_benchmarks_with_nonexistent_file(runner, capsys):
    """Test handling of nonexistent filter file"""
    benchmarks = runner._get_benchmarks_to_run("/nonexistent/file.conf")
    assert benchmarks is None

    captured = capsys.readouterr()
    assert "Error: Benchmark configuration file not found" in captured.out


def test_get_benchmarks_with_empty_file(runner, capsys):
    """Test handling of empty benchmark file"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        temp_file = f.name

    try:
        benchmarks = runner._get_benchmarks_to_run(temp_file)
        assert benchmarks is None

        captured = capsys.readouterr()
        assert "Error: No benchmarks found" in captured.out
    finally:
        os.unlink(temp_file)


@patch("os.path.exists")
@patch("run_test._load_from_file")
def test_get_benchmarks_default_configs(mock_load, mock_exists, runner):
    """Test loading benchmarks from default T1, T2, and additional configs"""
    # Mock that all config files exist
    mock_exists.return_value = True

    # Mock different benchmarks from each file
    mock_load.side_effect = [
        ["t1_bench1", "t1_bench2"],
        ["t2_bench1", "t2_bench2"],
        ["additional_bench1"],
    ]

    benchmarks = runner._get_benchmarks_to_run(None)

    # Should return all benchmarks combined (as a list from a set, order may vary)
    assert set(benchmarks) == {
        "t1_bench1",
        "t1_bench2",
        "t2_bench1",
        "t2_bench2",
        "additional_bench1",
    }
    assert len(benchmarks) == 5


@patch("os.path.exists")
@patch("run_test._load_from_file")
def test_get_benchmarks_missing_default_configs(mock_load, mock_exists, runner, capsys):
    """Test handling when some default config files are missing"""
    # Mock that only T1 config exists
    mock_exists.side_effect = [True, False, False]
    mock_load.return_value = ["t1_bench1", "t1_bench2"]

    benchmarks = runner._get_benchmarks_to_run(None)

    assert set(benchmarks) == {"t1_bench1", "t1_bench2"}

    captured = capsys.readouterr()
    assert "Warning: Configuration file not found" in captured.out


@patch("os.path.exists")
def test_get_benchmarks_all_default_configs_missing(mock_exists, runner, capsys):
    """Test handling when all default config files are missing"""
    mock_exists.return_value = False

    benchmarks = runner._get_benchmarks_to_run(None)

    assert benchmarks is None
    captured = capsys.readouterr()
    assert "Error: No benchmarks found" in captured.out


# Tests for _parse_benchmark_output


def test_parse_benchmark_output_success(runner):
    """Test parsing successful benchmark output with all metrics"""
    stdout = """
RibResolutionBenchmark                                       1.46s   684.78m
{"cpu_time_usec": 1460000, "max_rss": 123456}
"""

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_binary_name"] == "test_binary"
    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["test_status"] == "OK"
    assert result["relative_time_per_iter"] == "1.46s"
    assert result["iters_per_sec"] == "684.78m"
    assert result["cpu_time_usec"] == "1460000"
    assert result["max_rss"] == "123456"


def test_parse_benchmark_output_missing_json(runner):
    """Test parsing benchmark output with missing JSON metrics"""
    stdout = """
RibResolutionBenchmark                                       1.46s   684.78m
"""

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_binary_name"] == "test_binary"
    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["test_status"] == "FAILED"  # Missing JSON means FAILED
    assert result["relative_time_per_iter"] == "1.46s"
    assert result["iters_per_sec"] == "684.78m"
    assert result["cpu_time_usec"] == ""
    assert result["max_rss"] == ""


def test_parse_benchmark_output_missing_benchmark_line(runner):
    """Test parsing benchmark output with missing benchmark line"""
    stdout = """
{"cpu_time_usec": 1460000, "max_rss": 123456}
"""

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_binary_name"] == "test_binary"
    assert result["benchmark_test_name"] == ""
    assert result["test_status"] == "FAILED"  # Missing benchmark line means FAILED
    assert result["relative_time_per_iter"] == ""
    assert result["iters_per_sec"] == ""
    assert result["cpu_time_usec"] == "1460000"
    assert result["max_rss"] == "123456"


def test_parse_benchmark_output_empty(runner):
    """Test parsing empty benchmark output"""
    result = runner._parse_benchmark_output("test_binary", "")

    assert result["benchmark_binary_name"] == "test_binary"
    assert result["benchmark_test_name"] == ""
    assert result["test_status"] == "FAILED"
    assert result["relative_time_per_iter"] == ""
    assert result["iters_per_sec"] == ""
    assert result["cpu_time_usec"] == ""
    assert result["max_rss"] == ""


def test_parse_benchmark_output_with_units(runner):
    """Test parsing benchmark output with different time units"""
    stdout = """
FastBenchmark                                                123ms   8.13
{"cpu_time_usec": 123000, "max_rss": 98765}
"""

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "FastBenchmark"
    assert result["relative_time_per_iter"] == "123ms"
    assert result["iters_per_sec"] == "8.13"


# Tests for _run_benchmark_binary


@patch("subprocess.run")
def test_run_benchmark_binary_success(mock_run, runner, mock_args):
    """Test successful benchmark binary execution"""
    mock_result = Mock()
    mock_result.returncode = 0
    mock_result.stdout = """
TestBenchmark                                                2.5s    400m
{"cpu_time_usec": 2500000, "max_rss": 200000}
"""
    mock_result.stderr = ""
    mock_run.return_value = mock_result

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "TestBenchmark"
    mock_run.assert_called_once()


@patch("subprocess.run")
def test_run_benchmark_binary_timeout(mock_run, runner, mock_args):
    """Test benchmark binary timeout handling"""
    mock_run.side_effect = subprocess.TimeoutExpired("cmd", 300)

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "TIMEOUT"
    assert result["benchmark_binary_name"] == "/opt/fboss/bin/test_bench"
    assert result["benchmark_test_name"] == ""


@patch("subprocess.run")
def test_run_benchmark_binary_failure(mock_run, runner, mock_args):
    """Test benchmark binary execution failure"""
    mock_result = Mock()
    mock_result.returncode = 1
    mock_result.stdout = "Some error output"
    mock_result.stderr = "Error message"
    mock_run.return_value = mock_result

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "FAILED"
    mock_run.assert_called_once()


@patch("subprocess.run")
def test_run_benchmark_binary_exception(mock_run, runner, mock_args):
    """Test benchmark binary execution with exception"""
    mock_run.side_effect = Exception("Unexpected error")

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "FAILED"
    assert result["benchmark_binary_name"] == "/opt/fboss/bin/test_bench"


@patch("subprocess.run")
def test_run_benchmark_binary_with_config(mock_run, runner, mock_args):
    """Test benchmark binary execution with config arguments"""
    mock_args.config = "/path/to/config.conf"
    mock_args.mgmt_if = "eth1"

    mock_result = Mock()
    mock_result.returncode = 0
    mock_result.stdout = """
TestBenchmark                                                1.0s    1000m
{"cpu_time_usec": 1000000, "max_rss": 100000}
"""
    mock_result.stderr = ""
    mock_run.return_value = mock_result

    runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    # Verify config arguments were passed
    call_args = mock_run.call_args[0][0]
    assert "--config" in call_args
    assert "/path/to/config.conf" in call_args
    assert "--mgmt-if" in call_args
    assert "eth1" in call_args


# Tests for run_test method


def test_run_test_with_nonexistent_filter_file(runner, mock_args, capsys):
    """Test run_test with nonexistent filter file"""
    mock_args.filter_file = "/nonexistent/file.conf"

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Error: Benchmark configuration file not found" in captured.out


def test_run_test_list_tests_mode(runner, mock_args, temp_benchmark_file, capsys):
    """Test run_test in list_tests mode"""
    mock_args.filter_file = temp_benchmark_file
    mock_args.list_tests = True

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "benchmark1" in captured.out
    assert "benchmark2" in captured.out
    assert "benchmark3" in captured.out


@patch("os.path.exists")
@patch("os.path.isfile")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch("builtins.open", create=True)
def test_run_test_execution(
    mock_open,
    mock_run_binary,
    mock_isfile,
    mock_exists,
    runner,
    mock_args,
    temp_benchmark_file,
):
    """Test run_test executes benchmarks and writes CSV"""
    mock_args.filter_file = temp_benchmark_file

    # Mock that all benchmark binaries exist
    mock_exists.return_value = True
    mock_isfile.return_value = True

    # Mock benchmark execution results
    mock_run_binary.side_effect = [
        {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark1",
            "benchmark_test_name": "Bench1",
            "test_status": "OK",
            "relative_time_per_iter": "1.0s",
            "iters_per_sec": "1000m",
            "cpu_time_usec": "1000000",
            "max_rss": "100000",
        },
        {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark2",
            "benchmark_test_name": "Bench2",
            "test_status": "OK",
            "relative_time_per_iter": "2.0s",
            "iters_per_sec": "500m",
            "cpu_time_usec": "2000000",
            "max_rss": "200000",
        },
        {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark3",
            "benchmark_test_name": "Bench3",
            "test_status": "OK",
            "relative_time_per_iter": "3.0s",
            "iters_per_sec": "333m",
            "cpu_time_usec": "3000000",
            "max_rss": "300000",
        },
    ]

    # Mock CSV file writing
    mock_file = MagicMock()
    mock_open.return_value.__enter__.return_value = mock_file

    runner.run_test(mock_args)

    # Verify all benchmarks were executed
    assert mock_run_binary.call_count == 3

    # Verify CSV file was opened for writing
    mock_open.assert_called_once()
    assert ".csv" in str(mock_open.call_args)


@patch("os.path.exists")
@patch("os.path.isfile")
def test_run_test_no_existing_binaries(
    mock_isfile, mock_exists, runner, mock_args, temp_benchmark_file, capsys
):
    """Test run_test when no benchmark binaries exist"""
    mock_args.filter_file = temp_benchmark_file

    # Mock that benchmark binaries don't exist
    mock_exists.return_value = False
    mock_isfile.return_value = False

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Error: No benchmark binaries found" in captured.out


@patch("os.path.exists")
@patch("os.path.isfile")
def test_run_test_some_missing_binaries(
    mock_isfile, mock_exists, runner, mock_args, temp_benchmark_file, capsys
):
    """Test run_test when some benchmark binaries are missing"""
    mock_args.filter_file = temp_benchmark_file

    # Mock that only benchmark1 exists
    def exists_side_effect(path):
        return "benchmark1" in path

    def isfile_side_effect(path):
        return "benchmark1" in path

    mock_exists.side_effect = exists_side_effect
    mock_isfile.side_effect = isfile_side_effect

    # Mock benchmark execution
    with patch.object(BenchmarkTestRunner, "_run_benchmark_binary") as mock_run:
        with patch("builtins.open", create=True):
            mock_run.return_value = {
                "benchmark_binary_name": "/opt/fboss/bin/benchmark1",
                "benchmark_test_name": "Bench1",
                "test_status": "OK",
                "relative_time_per_iter": "1.0s",
                "iters_per_sec": "1000m",
                "cpu_time_usec": "1000000",
                "max_rss": "100000",
            }

            runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Warning:" in captured.out
    assert "benchmark binaries not found" in captured.out


# Integration tests


@patch("os.path.exists")
@patch("run_test._load_from_file")
def test_integration_default_configs_to_list_tests(
    mock_load, mock_exists, runner, mock_args, capsys
):
    """Integration test: load from default configs and list tests"""
    # Mock default config files exist
    mock_exists.return_value = True
    mock_load.side_effect = [["t1_bench"], ["t2_bench"], ["additional_bench"]]

    mock_args.list_tests = True

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "t1_bench" in captured.out
    assert "t2_bench" in captured.out
    assert "additional_bench" in captured.out
