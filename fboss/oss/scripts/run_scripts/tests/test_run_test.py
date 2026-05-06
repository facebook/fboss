#!/usr/bin/env python3
# ruff: noqa: I001
# Copyright Meta Platforms, Inc. and affiliates.

"""
Unit tests for run_test.py using pytest framework.

Provides comprehensive coverage of the BenchmarkTestRunner class,
including benchmark loading, parsing, execution, and error handling.
"""

import io
import json
import os
import subprocess
import tempfile
from unittest.mock import MagicMock, Mock, patch

import pytest
from run_test import _load_from_file, BenchmarkTestRunner

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
    args.skip_known_bad_tests = None
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
    """Test parsing successful --json benchmark output"""
    stdout = '{"RibResolutionBenchmark": 1460000000000}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_binary_name"] == "test_binary"
    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["test_status"] == "OK"
    assert result["cpu_time_usec"] == "1460000"
    assert result["max_rss"] == "123456"
    assert result["metrics"]["RibResolutionBenchmark"] == 1460000000000


def test_parse_benchmark_output_with_rx_pps(runner):
    """Test parsing output with cpu_rx_pps metric"""
    stdout = '{"RxSlowPathBenchmark": 1460000000000}\n{"cpu_rx_pps": 200000}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "RxSlowPathBenchmark"
    assert result["cpu_rx_pps"] == "200000"
    assert result["metrics"]["cpu_rx_pps"] == 200000
    assert result["metrics"]["RxSlowPathBenchmark"] == 1460000000000


def test_parse_benchmark_output_missing_rusage(runner):
    """Test parsing output with missing rusage JSON"""
    stdout = '{"RibResolutionBenchmark": 1460000000000}\n'

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["test_status"] == "FAILED"
    assert result["cpu_time_usec"] == ""


def test_parse_benchmark_output_missing_benchmark(runner):
    """Test parsing output with only rusage JSON (no benchmark name)"""
    stdout = '{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_test_name"] == ""
    assert result["test_status"] == "FAILED"
    assert result["cpu_time_usec"] == "1460000"


def test_parse_benchmark_output_empty(runner):
    """Test parsing empty benchmark output"""
    result = runner._parse_benchmark_output("test_binary", "")

    assert result["benchmark_test_name"] == ""
    assert result["test_status"] == "FAILED"
    assert result["metrics"] == {}


def test_parse_benchmark_output_with_noise(runner):
    """Test parsing output with non-JSON text interspersed"""
    stdout = 'DMA pool size: 16777216\n{"HwEcmpGroupShrink": 1460000000000}\nSome debug output\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"
    assert result["metrics"]["HwEcmpGroupShrink"] == 1460000000000


# Tests for _run_benchmark_binary


def _mock_popen(stdout_text, returncode=0):
    """Create a mock Popen that simulates streaming output line by line."""
    mock_proc = Mock()
    mock_proc.stdout = io.StringIO(stdout_text)
    mock_proc.stderr = io.StringIO("")
    mock_proc.returncode = returncode
    mock_proc.wait = Mock(return_value=returncode)
    mock_proc.kill = Mock()
    return mock_proc


@patch("subprocess.Popen")
def test_run_benchmark_binary_success(mock_popen_cls, runner, mock_args):
    """Test successful benchmark binary execution"""
    mock_popen_cls.return_value = _mock_popen(
        '{"TestBenchmark": 2500000000000}\n{"cpu_time_usec": 2500000, "max_rss": 200000}\n'
    )

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "TestBenchmark"
    mock_popen_cls.assert_called_once()


@patch("subprocess.Popen")
def test_run_benchmark_binary_timeout(mock_popen_cls, runner, mock_args):
    """Test benchmark binary timeout handling"""
    mock_proc = _mock_popen("")
    mock_proc.wait.side_effect = subprocess.TimeoutExpired("cmd", 300)
    mock_popen_cls.return_value = mock_proc

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "TIMEOUT"
    assert result["benchmark_binary_name"] == "/opt/fboss/bin/test_bench"
    assert result["benchmark_test_name"] == ""
    mock_proc.kill.assert_called_once()


@patch("subprocess.Popen")
def test_run_benchmark_binary_failure(mock_popen_cls, runner, mock_args):
    """Test benchmark binary execution failure"""
    mock_popen_cls.return_value = _mock_popen("Some error output\n", returncode=1)

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "FAILED"
    mock_popen_cls.assert_called_once()


@patch("subprocess.Popen")
def test_run_benchmark_binary_exception(mock_popen_cls, runner, mock_args):
    """Test benchmark binary execution with exception"""
    mock_popen_cls.side_effect = Exception("Unexpected error")

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "FAILED"
    assert result["benchmark_binary_name"] == "/opt/fboss/bin/test_bench"


@patch("subprocess.Popen")
def test_run_benchmark_binary_with_config(mock_popen_cls, runner, mock_args):
    """Test benchmark binary execution with config arguments"""
    mock_args.config = "/path/to/config.conf"
    mock_args.mgmt_if = "eth1"

    mock_popen_cls.return_value = _mock_popen(
        '{"TestBenchmark": 1000000000000}\n{"cpu_time_usec": 1000000, "max_rss": 100000}\n'
    )

    runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    call_args = mock_popen_cls.call_args[0][0]
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


@patch("run_test._load_from_file")
@patch("os.path.exists")
@patch("os.path.isfile")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch("builtins.open", create=True)
def test_run_test_execution(
    mock_open,
    mock_run_binary,
    mock_isfile,
    mock_exists,
    mock_load,
    runner,
    mock_args,
    temp_benchmark_file,
):
    """Test run_test executes benchmarks and writes CSV"""
    mock_args.filter_file = temp_benchmark_file

    # Mock that all benchmark binaries exist
    mock_exists.return_value = True
    mock_isfile.return_value = True
    mock_load.return_value = ["benchmark1", "benchmark2", "benchmark3"]

    # Mock benchmark execution results
    mock_run_binary.side_effect = [
        {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark1",
            "benchmark_test_name": "Bench1",
            "test_status": "OK",
            "cpu_time_usec": "1000000",
            "max_rss": "100000",
            "cpu_rx_pps": "",
            "cpu_tx_pps": "",
            "metrics": {"Bench1": 1000000000000},
        },
        {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark2",
            "benchmark_test_name": "Bench2",
            "test_status": "OK",
            "cpu_time_usec": "2000000",
            "max_rss": "200000",
            "cpu_rx_pps": "",
            "cpu_tx_pps": "",
            "metrics": {"Bench2": 2000000000000},
        },
        {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark3",
            "benchmark_test_name": "Bench3",
            "test_status": "OK",
            "cpu_time_usec": "3000000",
            "max_rss": "300000",
            "cpu_rx_pps": "",
            "cpu_tx_pps": "",
            "metrics": {"Bench3": 3000000000000},
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


@patch("run_test._load_from_file")
@patch("os.path.exists")
@patch("os.path.isfile")
def test_run_test_no_existing_binaries(
    mock_isfile, mock_exists, mock_load, runner, mock_args, temp_benchmark_file, capsys
):
    """Test run_test when no benchmark binaries exist"""
    mock_args.filter_file = temp_benchmark_file

    # Mock loading benchmarks from filter file
    mock_load.return_value = ["benchmark1", "benchmark2", "benchmark3"]
    # Mock that the filter file exists but benchmark binaries don't
    mock_exists.side_effect = lambda path: path == temp_benchmark_file
    mock_isfile.return_value = False

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Error: No benchmark binaries found" in captured.out


@patch("run_test._load_from_file")
@patch("os.path.exists")
@patch("os.path.isfile")
def test_run_test_some_missing_binaries(
    mock_isfile, mock_exists, mock_load, runner, mock_args, temp_benchmark_file, capsys
):
    """Test run_test when some benchmark binaries are missing"""
    mock_args.filter_file = temp_benchmark_file

    mock_load.return_value = ["benchmark1", "benchmark2", "benchmark3"]

    # Mock that filter file exists and only benchmark1 binary exists
    def exists_side_effect(path):
        return path == temp_benchmark_file or "benchmark1" in path

    def isfile_side_effect(path):
        return "benchmark1" in path

    mock_exists.side_effect = exists_side_effect
    mock_isfile.side_effect = isfile_side_effect

    # Mock benchmark execution
    with (
        patch.object(BenchmarkTestRunner, "_run_benchmark_binary") as mock_run,
        patch("builtins.open", create=True),
    ):
        mock_run.return_value = {
            "benchmark_binary_name": "/opt/fboss/bin/benchmark1",
            "benchmark_test_name": "Bench1",
            "test_status": "OK",
            "cpu_time_usec": "1000000",
            "max_rss": "100000",
            "cpu_rx_pps": "",
            "cpu_tx_pps": "",
            "metrics": {"Bench1": 1000000000000},
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


# Tests for _load_from_file


def test_load_from_file_basic():
    """Test loading entries from a file with comments and blank lines"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        f.write("# Comment line\n")
        f.write("entry1\n")
        f.write("\n")
        f.write("entry2\n")
        f.write("# Another comment\n")
        f.write("entry3\n")
        temp_file = f.name
    try:
        result = _load_from_file(temp_file)
        assert result == ["entry1", "entry2", "entry3"]
    finally:
        os.unlink(temp_file)


def test_load_from_file_nonexistent():
    """Test loading from a nonexistent file returns empty list"""
    result = _load_from_file("/nonexistent/path.conf")
    assert result == []


def test_load_from_file_empty():
    """Test loading from an empty file returns empty list"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        temp_file = f.name
    try:
        result = _load_from_file(temp_file)
        assert result == []
    finally:
        os.unlink(temp_file)


def test_load_from_file_with_profile():
    """Test loading with profile filters to matching tagged lines"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        f.write("entry_untagged\n")
        f.write("entry_tagged_p1 p1\n")
        f.write("entry_tagged_p2 p2\n")
        f.write("entry_tagged_t t\n")
        temp_file = f.name
    try:
        result = _load_from_file(temp_file, profile="p1")
        assert result == ["entry_tagged_p1"]
    finally:
        os.unlink(temp_file)


def test_load_from_file_no_profile_includes_untagged_and_t():
    """Test that without profile, untagged and t-tagged lines are included"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        f.write("entry_untagged\n")
        f.write("entry_tagged_p1 p1\n")
        f.write("entry_tagged_t t\n")
        temp_file = f.name
    try:
        result = _load_from_file(temp_file)
        assert result == ["entry_untagged", "entry_tagged_t"]
    finally:
        os.unlink(temp_file)


def test_load_from_file_comments_and_whitespace():
    """Test that comments and whitespace-only lines are skipped"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        f.write("# full comment\n")
        f.write("   \n")
        f.write("\n")
        f.write("  # indented comment\n")
        f.write("entry1\n")
        temp_file = f.name
    try:
        result = _load_from_file(temp_file)
        assert result == ["entry1"]
    finally:
        os.unlink(temp_file)


# Tests for _find_jsons_in_str


def test_find_jsons_in_str_multiple(runner):
    """Test extracting multiple JSON objects from mixed output"""
    output = 'noise\n{"a": 1}\nmore noise\n{"b": 2, "c": 3}\n'
    result = runner._find_jsons_in_str(output)
    assert len(result) == 2
    assert result[0] == {"a": 1}
    assert result[1] == {"b": 2, "c": 3}


def test_find_jsons_in_str_empty(runner):
    """Test extracting from empty string"""
    assert runner._find_jsons_in_str("") == []


def test_find_jsons_in_str_no_json(runner):
    """Test extracting from string with no JSON"""
    assert runner._find_jsons_in_str("just plain text") == []


# Tests for _load_known_bad_test_regexes


@pytest.fixture
def temp_sai_bench_config():
    """Create a temporary sai_bench config JSON file"""
    config = {
        "team": "sai_bench",
        "known_bad_tests": {
            "brcm/10.2.0.0_odp/tomahawk4": [
                {"test_name_regex": ".*Voq.*", "reason": "VOQ not applicable"},
                {
                    "test_name_regex": "HwInitAndExitFabricBenchmark",
                    "reason": "Fabric not applicable",
                },
            ],
            "brcm/10.2.0.0_odp/tomahawk4/mono": [
                {"test_name_regex": "ExtraMonoTest", "reason": "Mono specific"},
            ],
        },
        "buck_rule_or_test_id_to_benchmark_thres": {
            "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwEcmpGroupShrink": [
                {
                    "test_config_regex": ".*/tomahawk4.*",
                    "thresholds": [
                        {
                            "metric_key_regex": "HwEcmpGroupShrink",
                            "upper_bound": 25000000000000,  # picoseconds
                        }
                    ],
                }
            ],
            "fboss.agent.hw.sai.benchmarks.multi_switch.RxSlowPathBenchmark": [
                {
                    "test_config_regex": ".*/tomahawk4.*",
                    "thresholds": [
                        {"metric_key_regex": "cpu_rx_pps", "lower_bound": 138800}
                    ],
                }
            ],
        },
    }

    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".json") as f:
        json.dump(config, f)
        temp_file = f.name

    yield temp_file

    if os.path.exists(temp_file):
        os.unlink(temp_file)


@patch("run_test.SAI_BENCH_CONFIG", new="nonexistent_file.json")
def test_load_known_bad_test_regexes_missing_file(runner):
    """Test loading known bad tests when config file doesn't exist"""
    result = runner._load_known_bad_test_regexes("brcm/10.2.0.0_odp/tomahawk4")
    assert result == []


def test_load_known_bad_test_regexes_exact_key(runner, temp_sai_bench_config):
    """Test loading known bad tests with exact key match"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        regexes = runner._load_known_bad_test_regexes("brcm/10.2.0.0_odp/tomahawk4")
    # Should get regexes from exact key only (no suffix appending)
    assert ".*Voq.*" in regexes
    assert "HwInitAndExitFabricBenchmark" in regexes


def test_load_known_bad_test_regexes_no_match(runner, temp_sai_bench_config):
    """Test loading known bad tests with no matching key"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        regexes = runner._load_known_bad_test_regexes("brcm/10.2.0.0_odp/tomahawk5")
    assert regexes == []


# Tests for _find_thresholds_for_benchmark


def test_find_thresholds_mono_match(runner, temp_sai_bench_config):
    """Test finding thresholds for a mono benchmark by metric key in output"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    metrics = {"HwEcmpGroupShrink": 10000000000000, "cpu_time_usec": 123}
    thresholds = runner._find_thresholds_for_benchmark(
        metrics,
        is_multi_switch=False,
        platform_key="brcm/10.2.0.0_odp/tomahawk4",
        all_thresholds=all_thresholds,
    )
    assert len(thresholds) == 1
    assert thresholds[0]["metric_key_regex"] == "HwEcmpGroupShrink"
    assert thresholds[0]["upper_bound"] == 25000000000000


def test_find_thresholds_multi_switch_match(runner, temp_sai_bench_config):
    """Test finding thresholds for a multi-switch benchmark by metric key"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    metrics = {"RxSlowPathBenchmark": 1460000000000, "cpu_rx_pps": 200000}
    thresholds = runner._find_thresholds_for_benchmark(
        metrics,
        is_multi_switch=True,
        platform_key="brcm/10.2.0.0_odp/tomahawk4",
        all_thresholds=all_thresholds,
    )
    assert len(thresholds) == 1
    assert thresholds[0]["metric_key_regex"] == "cpu_rx_pps"
    assert thresholds[0]["lower_bound"] == 138800


def test_find_thresholds_no_match(runner, temp_sai_bench_config):
    """Test finding thresholds when no output metric matches any config key"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    metrics = {"NonExistentBenchmark": 100, "cpu_time_usec": 123}
    thresholds = runner._find_thresholds_for_benchmark(
        metrics,
        is_multi_switch=False,
        platform_key="brcm/10.2.0.0_odp/tomahawk4",
        all_thresholds=all_thresholds,
    )
    assert thresholds == []


def test_find_thresholds_platform_mismatch(runner, temp_sai_bench_config):
    """Test that platform regex mismatch returns empty"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    metrics = {"HwEcmpGroupShrink": 10000000000000}
    thresholds = runner._find_thresholds_for_benchmark(
        metrics,
        is_multi_switch=False,
        platform_key="brcm/10.2.0.0_odp/tomahawk5",
        all_thresholds=all_thresholds,
    )
    assert thresholds == []


# Tests for _check_thresholds


def test_check_thresholds_pass(runner):
    """Test threshold check when metric is within bounds"""
    result = {
        "metrics": {"HwEcmpGroupShrink": 10000000000000},  # 10s in picoseconds
    }
    thresholds = [
        {
            "metric_key_regex": "HwEcmpGroupShrink",
            "upper_bound": 25000000000000,  # 25s in picoseconds
        }
    ]
    violations = runner._check_thresholds(result, thresholds)
    assert violations == []


def test_check_thresholds_upper_exceeded(runner):
    """Test threshold check when metric exceeds upper bound"""
    result = {
        "metrics": {"HwEcmpGroupShrink": 30000000000000},  # 30s in picoseconds
    }
    thresholds = [
        {
            "metric_key_regex": "HwEcmpGroupShrink",
            "upper_bound": 25000000000000,  # 25s in picoseconds
        }
    ]
    violations = runner._check_thresholds(result, thresholds)
    assert len(violations) == 1
    assert "upper_bound" in violations[0]


def test_check_thresholds_lower_violated(runner):
    """Test threshold check when metric is below lower bound"""
    result = {
        "metrics": {"cpu_rx_pps": 100000},
    }
    thresholds = [{"metric_key_regex": "cpu_rx_pps", "lower_bound": 138800}]
    violations = runner._check_thresholds(result, thresholds)
    assert len(violations) == 1
    assert "lower_bound" in violations[0]


def test_check_thresholds_missing_metric_no_violation(runner):
    """Test that missing metric does not cause violation"""
    result = {
        "metrics": {"SomeOtherBenchmark": 10000000000000},
    }
    thresholds = [
        {
            "metric_key_regex": "HwEcmpGroupShrink",
            "upper_bound": 25000000000000,
        }
    ]
    violations = runner._check_thresholds(result, thresholds)
    assert violations == []


def test_parse_benchmark_output_folly_json_format(runner):
    """Test parsing folly --json output format (JSON object with name: picoseconds)"""
    stdout = """{
  "RibResolutionBenchmark": 1460000000000
}
{"cpu_time_usec": 1460000, "max_rss": 123456}
"""
    result = runner._parse_benchmark_output("test_binary", stdout)
    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["metrics"]["RibResolutionBenchmark"] == 1460000000000
    assert result["cpu_time_usec"] == "1460000"
    assert result["max_rss"] == "123456"


def test_parse_benchmark_output_folly_json_with_pps(runner):
    """Test parsing folly --json output with cpu_rx_pps metric"""
    stdout = """{
  "RxSlowPathBenchmark": 500000000
}
{"cpu_rx_pps": 200000}
{"cpu_time_usec": 500000, "max_rss": 98765}
"""
    result = runner._parse_benchmark_output("sai_rx_slow_path_rate-sai_impl", stdout)
    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "RxSlowPathBenchmark"
    assert result["metrics"]["RxSlowPathBenchmark"] == 500000000
    assert result["cpu_rx_pps"] == "200000"
