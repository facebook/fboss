#!/usr/bin/env python3
# Copyright Meta Platforms, Inc. and affiliates.

"""
Unit tests for the BenchmarkTestRunner in run_test.py.

Covers benchmark loading, parsing, execution, threshold validation,
and error handling.
"""

import io
import json
import os
import subprocess
import tempfile
from unittest.mock import Mock, patch

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
    args.filter = None
    args.list_tests = False
    args.config = None
    args.mgmt_if = "eth0"
    args.platform_mapping_override_path = None
    args.fruid_path = None
    args.sai_logging = "WARN"
    args.fboss_logging = "WARN"
    args.test_run_timeout = 300
    args.skip_known_bad_tests = None
    args.agent_run_mode = "mono"
    args.num_npus = 1
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

    if os.path.exists(temp_file):
        os.unlink(temp_file)


# Tests for _load_requested_benchmarks


def test_load_requested_with_filter_file(runner, temp_benchmark_file):
    """Test loading benchmarks from a filter file"""
    benchmarks = runner._load_requested_benchmarks(temp_benchmark_file)
    assert benchmarks == ["benchmark1", "benchmark2", "benchmark3"]


def test_load_requested_with_nonexistent_file(runner, capsys):
    """Test handling of nonexistent filter file"""
    benchmarks = runner._load_requested_benchmarks("/nonexistent/file.conf")
    assert benchmarks is None

    captured = capsys.readouterr()
    assert "Error: Configuration file not found" in captured.out


def test_load_requested_with_empty_file(runner, capsys):
    """Test handling of empty filter file"""
    with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
        temp_file = f.name

    try:
        benchmarks = runner._load_requested_benchmarks(temp_file)
        assert benchmarks is None

        captured = capsys.readouterr()
        assert "Error: No benchmarks found" in captured.out
    finally:
        os.unlink(temp_file)


# Tests for _get_benchmark_binary


def test_get_benchmark_binary_mono(runner, mock_args):
    """Default agent_run_mode (mono) returns the mono binary"""
    mock_args.agent_run_mode = "mono"
    with (
        patch("os.path.exists", return_value=True),
        patch("os.path.isfile", return_value=True),
    ):
        path = runner._get_benchmark_binary(mock_args)
    assert path == "/opt/fboss/bin/sai_all_benchmarks-sai_impl"


def test_get_benchmark_binary_multi_switch(runner, mock_args):
    """agent_run_mode=multi_switch returns the multi-switch binary"""
    mock_args.agent_run_mode = "multi_switch"
    with (
        patch("os.path.exists", return_value=True),
        patch("os.path.isfile", return_value=True),
    ):
        path = runner._get_benchmark_binary(mock_args)
    assert path == "/opt/fboss/bin/sai_multi_switch_all_benchmarks-sai_impl"


def test_get_benchmark_binary_missing(runner, mock_args):
    """Returns None when binary doesn't exist"""
    mock_args.agent_run_mode = "mono"
    with patch("os.path.exists", return_value=False):
        assert runner._get_benchmark_binary(mock_args) is None


def test_get_benchmark_binary_defaults_to_mono_when_attr_missing(runner):
    """Callers without agent_run_mode (e.g., older Namespaces) get the mono binary."""

    class _NoAgentRunMode:
        pass

    args = _NoAgentRunMode()
    with (
        patch("os.path.exists", return_value=True),
        patch("os.path.isfile", return_value=True),
    ):
        path = runner._get_benchmark_binary(args)
    assert path == "/opt/fboss/bin/sai_all_benchmarks-sai_impl"


# Tests for _list_benchmarks


def test_list_benchmarks_success(runner):
    """Test discovering benchmarks from --bm_list output"""
    bm_list_output = (
        "This test program does NOT link in any test case.\n"
        "[==========] Running 0 tests from 0 test suites.\n"
        "[==========] 0 tests from 0 test suites ran. (0 ms total)\n"
        "[  PASSED  ] 0 tests.\n"
        "HwFswScaleRouteAddBenchmark\n"
        "HwEcmpGroupShrink\n"
        "RibResolutionBenchmark\n"
        '{\n  "cpu_time_usec": 911,\n  "max_rss": 47872\n}\n'
    )
    with patch("subprocess.check_output", return_value=bm_list_output):
        result = runner._list_benchmarks("/opt/fboss/bin/sai_all_benchmarks-sai_impl")

    assert result == [
        "HwFswScaleRouteAddBenchmark",
        "HwEcmpGroupShrink",
        "RibResolutionBenchmark",
    ]


def test_list_benchmarks_filters_json_and_noise(runner):
    """Test that JSON properties, gtest output, and other noise are filtered"""
    bm_list_output = (
        "This test program does NOT link in any test case.\n"
        "[==========] Running 0 tests from 0 test suites.\n"
        "[  PASSED  ] 0 tests.\n"
        "HwEcmpGroupShrink\n"
        "{\n"
        '  "cpu_time_usec": 911,\n'
        '  "max_rss": 47872\n'
        "}\n"
        "Some random debug line with spaces\n"
        "runTxSlowPathBenchmark\n"
    )
    with patch("subprocess.check_output", return_value=bm_list_output):
        result = runner._list_benchmarks("/opt/fboss/bin/sai_all_benchmarks-sai_impl")

    assert result == ["HwEcmpGroupShrink", "runTxSlowPathBenchmark"]


def test_list_benchmarks_failure(runner, capsys):
    """Test handling of --bm_list failure"""
    with patch(
        "subprocess.check_output",
        side_effect=subprocess.CalledProcessError(1, "cmd"),
    ):
        result = runner._list_benchmarks("/opt/fboss/bin/sai_all_benchmarks-sai_impl")

    assert result is None
    captured = capsys.readouterr()
    assert "Warning: Failed to list benchmarks" in captured.out


def test_list_benchmarks_timeout(runner):
    """Test handling of --bm_list timeout"""
    with patch(
        "subprocess.check_output",
        side_effect=subprocess.TimeoutExpired("cmd", 30),
    ):
        result = runner._list_benchmarks("/opt/fboss/bin/sai_all_benchmarks-sai_impl")

    assert result is None


# Tests for _parse_benchmark_output


def test_parse_benchmark_output_with_name(runner):
    """Test parsing with benchmark_name extracts timing directly"""
    stdout = '{"RibResolutionBenchmark": 1460000000000}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="RibResolutionBenchmark"
    )

    assert result["benchmark_binary_name"] == "test_binary"
    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["benchmark_time_ps"] == "1460000000000"
    assert result["cpu_time_usec"] == "1460000"
    assert result["max_rss"] == "123456"
    assert result["metrics"]["RibResolutionBenchmark"] == 1460000000000


def test_parse_benchmark_output_with_rx_pps(runner):
    """Test parsing output with cpu_rx_pps metric"""
    stdout = '{"RxSlowPathBenchmark": 1460000000000}\n{"cpu_rx_pps": 200000}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="RxSlowPathBenchmark"
    )

    assert result["benchmark_test_name"] == "RxSlowPathBenchmark"
    assert result["benchmark_time_ps"] == "1460000000000"
    assert result["cpu_rx_pps"] == "200000"
    assert result["metrics"]["cpu_rx_pps"] == 200000


def test_parse_benchmark_output_name_not_in_json(runner):
    """Test that benchmark_name is preserved even when not in JSON output"""
    stdout = '{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="HwEcmpGroupShrink"
    )

    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"
    assert result["benchmark_time_ps"] == ""
    assert result["cpu_time_usec"] == "1460000"


def test_parse_benchmark_output_no_name(runner):
    """Test parsing without benchmark_name leaves test_name empty"""
    stdout = '{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output("test_binary", stdout)

    assert result["benchmark_test_name"] == ""
    assert result["cpu_time_usec"] == "1460000"


def test_parse_benchmark_output_empty(runner):
    """Test parsing empty benchmark output"""
    result = runner._parse_benchmark_output("test_binary", "")

    assert result["benchmark_test_name"] == ""
    assert result["metrics"] == {}


def test_parse_benchmark_output_with_noise(runner):
    """Test parsing output with non-JSON text interspersed"""
    stdout = 'DMA pool size: 16777216\n{"HwEcmpGroupShrink": 1460000000000}\nSome debug output\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="HwEcmpGroupShrink"
    )

    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"
    assert result["benchmark_time_ps"] == "1460000000000"


def test_parse_benchmark_output_extra_metrics_dont_confuse(runner):
    """Test that extra numeric metrics don't affect name/timing when benchmark_name is provided"""
    stdout = '{"max_mp_group_size": 42}\n{"HwEcmpGroupShrink": 1460000000000}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'

    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="HwEcmpGroupShrink"
    )

    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"
    assert result["benchmark_time_ps"] == "1460000000000"
    assert result["metrics"]["max_mp_group_size"] == 42


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
    """Test successful benchmark binary execution with benchmark_name"""
    mock_popen_cls.return_value = _mock_popen(
        '{"TestBenchmark": 2500000000000}\n{"cpu_time_usec": 2500000, "max_rss": 200000}\n'
    )

    result = runner._run_benchmark_binary(
        "/opt/fboss/bin/test_bench", mock_args, benchmark_name="TestBenchmark"
    )

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "TestBenchmark"
    assert result["benchmark_time_ps"] == "2500000000000"
    mock_popen_cls.assert_called_once()


@patch("subprocess.Popen")
def test_run_benchmark_binary_with_bm_regex(mock_popen_cls, runner, mock_args):
    """Test benchmark execution with --bm_regex for specific test selection"""
    mock_popen_cls.return_value = _mock_popen(
        '{"HwEcmpGroupShrink": 2500000000000}\n{"cpu_time_usec": 2500000, "max_rss": 200000}\n'
    )

    result = runner._run_benchmark_binary(
        "/opt/fboss/bin/sai_all_benchmarks-sai_impl",
        mock_args,
        benchmark_name="HwEcmpGroupShrink",
    )

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"

    call_args = mock_popen_cls.call_args[0][0]
    assert "--bm_regex" in call_args
    assert "^HwEcmpGroupShrink$" in call_args


@patch("subprocess.Popen")
def test_run_benchmark_binary_timeout(mock_popen_cls, runner, mock_args):
    """Test benchmark binary timeout handling"""
    mock_proc = _mock_popen("")
    mock_proc.wait.side_effect = subprocess.TimeoutExpired("cmd", 300)
    mock_popen_cls.return_value = mock_proc

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "TIMEOUT"
    assert result["benchmark_test_name"] == ""
    mock_proc.kill.assert_called_once()


@patch("subprocess.Popen")
def test_run_benchmark_binary_timeout_preserves_name(mock_popen_cls, runner, mock_args):
    """Test that timeout preserves benchmark_name when provided"""
    mock_proc = _mock_popen("")
    mock_proc.wait.side_effect = subprocess.TimeoutExpired("cmd", 300)
    mock_popen_cls.return_value = mock_proc

    result = runner._run_benchmark_binary(
        "/opt/fboss/bin/sai_all_benchmarks-sai_impl",
        mock_args,
        benchmark_name="HwEcmpGroupShrink",
    )

    assert result["test_status"] == "TIMEOUT"
    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"


@patch("subprocess.Popen")
def test_run_benchmark_binary_failure(mock_popen_cls, runner, mock_args):
    """Test benchmark binary execution failure"""
    mock_popen_cls.return_value = _mock_popen("Some error output\n", returncode=1)

    result = runner._run_benchmark_binary("/opt/fboss/bin/test_bench", mock_args)

    assert result["test_status"] == "FAILED"


@patch("subprocess.Popen")
def test_run_benchmark_binary_exception_preserves_name(
    mock_popen_cls, runner, mock_args
):
    """Test that exception preserves benchmark_name when provided"""
    mock_popen_cls.side_effect = Exception("Unexpected error")

    result = runner._run_benchmark_binary(
        "/opt/fboss/bin/sai_all_benchmarks-sai_impl",
        mock_args,
        benchmark_name="HwEcmpGroupShrink",
    )

    assert result["test_status"] == "FAILED"
    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"


@patch("subprocess.Popen")
def test_run_benchmark_binary_exit_code_determines_status(
    mock_popen_cls, runner, mock_args
):
    """Test that exit code 0 means OK even without JSON benchmark output"""
    mock_popen_cls.return_value = _mock_popen(
        '{"cpu_time_usec": 244392642, "max_rss": 4581460}\n', returncode=0
    )

    result = runner._run_benchmark_binary(
        "/opt/fboss/bin/sai_all_benchmarks-sai_impl",
        mock_args,
        benchmark_name="HwEcmpGroupShrink",
    )

    assert result["test_status"] == "OK"
    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"


@patch("subprocess.Popen")
def test_run_benchmark_binary_nonzero_exit_is_failed(mock_popen_cls, runner, mock_args):
    """Test that non-zero exit code means FAILED even with valid JSON output"""
    mock_popen_cls.return_value = _mock_popen(
        '{"HwEcmpGroupShrink": 1460000000000}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n',
        returncode=1,
    )

    result = runner._run_benchmark_binary(
        "/opt/fboss/bin/test_bench", mock_args, benchmark_name="HwEcmpGroupShrink"
    )

    assert result["test_status"] == "FAILED"
    assert result["benchmark_test_name"] == "HwEcmpGroupShrink"


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


# Tests for _build_benchmark_cmd


def test_build_benchmark_cmd_mono(runner, mock_args):
    """Mono mode includes --flexports and --enable_sai_log, no --multi_switch"""
    cmd = runner._build_benchmark_cmd("/opt/fboss/bin/bench", mock_args)
    assert "--flexports" in cmd
    assert "--enable_sai_log" in cmd
    assert "--multi_switch" not in cmd
    assert "--hw_agent_for_testing" not in cmd


def test_build_benchmark_cmd_multi_switch(runner, mock_args):
    """Multi-switch mode includes --multi_switch and --hw_agent_for_testing"""
    mock_args.agent_run_mode = "multi_switch"
    mock_args.num_npus = 1
    cmd = runner._build_benchmark_cmd("/opt/fboss/bin/bench", mock_args)
    assert "--multi_switch" in cmd
    assert "--hw_agent_for_testing" in cmd
    assert "--flexports" not in cmd
    assert "--enable_sai_log" not in cmd
    assert "--multi_npu_platform_mapping" not in cmd


def test_build_benchmark_cmd_multi_switch_multi_npu(runner, mock_args):
    """Multi-switch with num_npus > 1 includes --multi_npu_platform_mapping"""
    mock_args.agent_run_mode = "multi_switch"
    mock_args.num_npus = 2
    cmd = runner._build_benchmark_cmd("/opt/fboss/bin/bench", mock_args)
    assert "--multi_switch" in cmd
    assert "--multi_npu_platform_mapping" in cmd


def test_build_benchmark_cmd_with_bm_regex(runner, mock_args):
    """Benchmark name adds --bm_regex with escaped name"""
    cmd = runner._build_benchmark_cmd(
        "/opt/fboss/bin/bench", mock_args, benchmark_name="HwEcmpGroupShrink"
    )
    assert "--bm_regex" in cmd
    assert "^HwEcmpGroupShrink$" in cmd


# Tests for run_test


def _make_ok_result(name):
    return {
        "benchmark_binary_name": "/opt/fboss/bin/sai_all_benchmarks-sai_impl",
        "benchmark_test_name": name,
        "benchmark_time_ps": "1000",
        "test_status": "OK",
        "cpu_time_usec": "100",
        "max_rss": "200",
        "cpu_rx_pps": "",
        "cpu_tx_pps": "",
        "metrics": {name: 1000},
    }


@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_no_binary(mock_get_bin, runner, mock_args, capsys):
    """Test run_test when binary not found"""
    mock_get_bin.return_value = None

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Could not find benchmark binary" in captured.out


@patch.object(BenchmarkTestRunner, "_write_results_and_summary")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_basic_flow(
    mock_get_bin, mock_list, mock_run, mock_write, runner, mock_args
):
    """Test run_test runs all discovered benchmarks by default"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "BenchB", "BenchC"]
    mock_run.return_value = _make_ok_result("BenchA")

    runner.run_test(mock_args)

    assert mock_run.call_count == 3
    for call in mock_run.call_args_list:
        assert call[1].get("benchmark_name") is not None
    mock_write.assert_called_once()


@patch.object(BenchmarkTestRunner, "_write_results_and_summary")
@patch.object(BenchmarkTestRunner, "_apply_threshold_check")
@patch.object(BenchmarkTestRunner, "_load_benchmark_thresholds")
@patch.object(BenchmarkTestRunner, "_load_known_bad_test_regexes")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_passes_is_multi_switch_to_threshold_check(
    mock_get_bin,
    mock_list,
    mock_run,
    mock_known_bad,
    mock_load_thresh,
    mock_apply,
    mock_write,
    runner,
    mock_args,
):
    """args.agent_run_mode='multi_switch' propagates as is_multi_switch=True
    to _apply_threshold_check; default mono propagates as False."""
    mock_get_bin.return_value = (
        "/opt/fboss/bin/sai_multi_switch_all_benchmarks-sai_impl"
    )
    mock_list.return_value = ["BenchA"]
    mock_run.return_value = _make_ok_result("BenchA")
    mock_known_bad.return_value = []
    mock_load_thresh.return_value = {"some.key": [{"thresholds": []}]}
    mock_args.skip_known_bad_tests = "brcm/10.2.0.0_odp/tomahawk4"

    mock_args.agent_run_mode = "multi_switch"
    runner.run_test(mock_args)
    assert mock_apply.call_args[0][1] is True

    mock_apply.reset_mock()
    mock_args.agent_run_mode = "mono"
    runner.run_test(mock_args)
    assert mock_apply.call_args[0][1] is False
    assert mock_write.call_count == 2


@patch.object(BenchmarkTestRunner, "_write_results_and_summary")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch.object(BenchmarkTestRunner, "_load_requested_benchmarks")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_with_filter_file(
    mock_get_bin, mock_list, mock_load, mock_run, mock_write, runner, mock_args
):
    """Test --filter_file narrows benchmarks to those in the file"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "BenchB", "BenchC"]
    mock_load.return_value = ["BenchA", "BenchB"]
    mock_args.filter_file = "/some/file.conf"
    mock_run.return_value = _make_ok_result("BenchA")

    runner.run_test(mock_args)

    assert mock_run.call_count == 2
    mock_write.assert_called_once()


@patch.object(BenchmarkTestRunner, "_write_results_and_summary")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_pre_filters_known_bad(
    mock_get_bin, mock_list, mock_run, mock_write, runner, mock_args, capsys
):
    """Test that known-bad tests are skipped BEFORE running"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "HwVoqTest", "BenchC"]
    mock_run.return_value = _make_ok_result("BenchA")
    mock_args.skip_known_bad_tests = "brcm/10.2.0.0_odp/tomahawk4"

    with (
        patch.object(
            BenchmarkTestRunner,
            "_load_known_bad_test_regexes",
            return_value=[".*Voq.*"],
        ),
        patch.object(
            BenchmarkTestRunner, "_load_benchmark_thresholds", return_value={}
        ),
    ):
        runner.run_test(mock_args)

    assert mock_run.call_count == 2
    captured = capsys.readouterr()
    assert "SKIPPING (known bad): HwVoqTest" in captured.out
    mock_write.assert_called_once()
    assert mock_write.call_args[0][1] == 1


@patch.object(BenchmarkTestRunner, "_write_results_and_summary")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_filter_exact_match(
    mock_get_bin, mock_list, mock_run, _mock_write, runner, mock_args
):
    """Test --filter with exact name matches one benchmark"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "BenchB", "HwEcmpGroupShrink"]
    mock_args.filter = "HwEcmpGroupShrink"
    mock_run.return_value = _make_ok_result("HwEcmpGroupShrink")

    runner.run_test(mock_args)

    assert mock_run.call_count == 1
    assert mock_run.call_args[1]["benchmark_name"] == "HwEcmpGroupShrink"


@patch.object(BenchmarkTestRunner, "_write_results_and_summary")
@patch.object(BenchmarkTestRunner, "_run_benchmark_binary")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_filter_regex_match(
    mock_get_bin, mock_list, mock_run, _mock_write, runner, mock_args
):
    """Test --filter with regex pattern matches multiple benchmarks"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = [
        "HwFswScaleRouteAddBenchmark",
        "HwFswScaleRouteDelBenchmark",
        "HwEcmpGroupShrink",
    ]
    mock_args.filter = ".*Route.*"
    mock_run.return_value = _make_ok_result("HwFswScaleRouteAddBenchmark")

    runner.run_test(mock_args)

    assert mock_run.call_count == 2


@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_filter_no_match(mock_get_bin, mock_list, runner, mock_args, capsys):
    """Test --filter with no matches prints error and returns"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "BenchB"]
    mock_args.filter = "NonExistent"

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "No benchmarks matching" in captured.out


@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_filter_invalid_regex(
    mock_get_bin, mock_list, runner, mock_args, capsys
):
    """Test --filter with invalid regex prints error and returns"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "BenchB"]
    mock_args.filter = "[invalid"

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Invalid --filter regex" in captured.out


@patch.object(BenchmarkTestRunner, "_load_requested_benchmarks")
@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_filter_file_not_found_in_binary(
    mock_get_bin, mock_list, mock_load, runner, mock_args, capsys
):
    """Test warning when filter_file entries are not found in binary"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA"]
    mock_load.return_value = ["BenchA", "NonExistentBench"]
    mock_args.filter_file = "/some/file.conf"

    with (
        patch.object(BenchmarkTestRunner, "_run_benchmark_binary") as mock_run,
        patch.object(BenchmarkTestRunner, "_write_results_and_summary"),
    ):
        mock_run.return_value = _make_ok_result("BenchA")
        runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "NonExistentBench" in captured.out
    assert "not found in binary" in captured.out


@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_list_tests(mock_get_bin, mock_list, runner, mock_args, capsys):
    """Test --list_tests prints all discovered benchmarks"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = ["BenchA", "BenchB"]
    mock_args.list_tests = True

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "BenchA" in captured.out
    assert "BenchB" in captured.out


@patch.object(BenchmarkTestRunner, "_list_benchmarks")
@patch.object(BenchmarkTestRunner, "_get_benchmark_binary")
def test_run_test_discovery_failure(mock_get_bin, mock_list, runner, mock_args, capsys):
    """Test run_test when benchmark discovery fails"""
    mock_get_bin.return_value = "/opt/fboss/bin/sai_all_benchmarks-sai_impl"
    mock_list.return_value = None

    runner.run_test(mock_args)

    captured = capsys.readouterr()
    assert "Could not discover benchmarks" in captured.out


# Tests for _write_results_and_summary


def test_write_results_and_summary_csv_and_counts(runner, capsys, tmp_path):
    """Test CSV output and summary counts"""
    os.chdir(tmp_path)
    results = [
        _make_ok_result("BenchA"),
        _make_ok_result("BenchB"),
    ]
    results[0]["threshold_status"] = "PASS"
    results[0]["threshold_details"] = ""
    results[1]["threshold_status"] = "NO_THRESHOLD"
    results[1]["threshold_details"] = ""

    runner._write_results_and_summary(results, skipped_count=1)

    captured = capsys.readouterr()
    assert "OK: 2" in captured.out
    assert "Skipped (known bad, pre-filtered): 1" in captured.out
    assert "BENCHMARK RESULTS SUMMARY" in captured.out
    assert "BenchA: OK [THRESHOLD PASS]" in captured.out
    assert "BenchB: OK [NO_THRESHOLD]" in captured.out
    assert "Threshold: 1 pass, 0 exceeded, 1 not configured" in captured.out

    csv_files = list(tmp_path.glob("benchmark_results_*.csv"))
    assert len(csv_files) == 1
    content = csv_files[0].read_text()
    assert "benchmark_time_ps" in content
    assert "BenchA" in content


def test_write_results_and_summary_exits_on_failure(runner, tmp_path):
    """Test that sys.exit(1) is called when benchmarks fail"""
    os.chdir(tmp_path)
    results = [_make_ok_result("BenchA")]
    results[0]["test_status"] = "FAILED"
    results[0]["threshold_status"] = "N/A"
    results[0]["threshold_details"] = ""

    with pytest.raises(SystemExit) as exc_info:
        runner._write_results_and_summary(results)

    assert exc_info.value.code == 1


# Tests for _apply_threshold_check


def test_apply_threshold_check_pass(runner):
    """Test threshold check marks result as PASS"""
    result = _make_ok_result("HwEcmpGroupShrink")
    result["metrics"]["HwEcmpGroupShrink"] = 10000000000000
    thresholds = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwEcmpGroupShrink": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {
                        "metric_key_regex": "HwEcmpGroupShrink",
                        "upper_bound": 25000000000000,
                    }
                ],
            }
        ]
    }

    runner._apply_threshold_check(
        result,
        False,
        "brcm/10.2.0.0_odp/tomahawk4",
        thresholds,
    )

    assert result["threshold_status"] == "PASS"


def test_apply_threshold_check_exceeded(runner):
    """Test threshold check marks result as EXCEEDED"""
    result = _make_ok_result("HwEcmpGroupShrink")
    result["metrics"]["HwEcmpGroupShrink"] = 30000000000000
    thresholds = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwEcmpGroupShrink": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {
                        "metric_key_regex": "HwEcmpGroupShrink",
                        "upper_bound": 25000000000000,
                    }
                ],
            }
        ]
    }

    runner._apply_threshold_check(
        result,
        False,
        "brcm/10.2.0.0_odp/tomahawk4",
        thresholds,
    )

    assert result["threshold_status"] == "EXCEEDED"


def test_apply_threshold_check_no_platform_key(runner):
    """Test threshold check is N/A when no platform key"""
    result = _make_ok_result("BenchA")

    runner._apply_threshold_check(result, False, None, {})

    assert result["threshold_status"] == "N/A"


def test_apply_threshold_check_failed_status(runner):
    """Test threshold check is N/A when test status is not OK"""
    result = _make_ok_result("BenchA")
    result["test_status"] = "FAILED"

    runner._apply_threshold_check(
        result,
        False,
        "brcm/10.2.0.0_odp/tomahawk4",
        {"some": "thresholds"},
    )

    assert result["threshold_status"] == "N/A"


def test_apply_threshold_check_no_threshold_warns(runner, capsys):
    """When no threshold matches, marks NO_THRESHOLD and warns operator."""
    result = _make_ok_result("UnconfiguredBenchmark")
    thresholds = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwEcmpGroupShrink": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwEcmpGroupShrink", "upper_bound": 1}
                ],
            }
        ]
    }

    runner._apply_threshold_check(
        result,
        False,
        "brcm/10.2.0.0_odp/tomahawk4",
        thresholds,
    )

    assert result["threshold_status"] == "NO_THRESHOLD"
    captured = capsys.readouterr()
    assert "WILL ALWAYS PASS" in captured.out
    assert "UnconfiguredBenchmark" in captured.out


def test_apply_threshold_check_multi_switch_picks_multi_threshold(runner):
    """is_multi_switch=True selects the multi_switch-keyed threshold over the mono one."""
    result = _make_ok_result("HwStatsCollection")
    result["metrics"]["HwStatsCollection"] = 5000
    thresholds = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwStatsCollection": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwStatsCollection", "upper_bound": 1000}
                ],
            }
        ],
        "fboss.agent.hw.sai.benchmarks.multi_switch.HwStatsCollection": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwStatsCollection", "upper_bound": 9999}
                ],
            }
        ],
    }

    runner._apply_threshold_check(
        result, True, "brcm/10.2.0.0_odp/tomahawk4", thresholds
    )

    # 5000 is over the mono bound (1000) but under the multi_switch bound (9999).
    # PASS proves the multi_switch threshold was picked.
    assert result["threshold_status"] == "PASS"


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
                            "upper_bound": 25000000000000,
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
    assert ".*Voq.*" in regexes
    assert "HwInitAndExitFabricBenchmark" in regexes


def test_load_known_bad_test_regexes_no_match(runner, temp_sai_bench_config):
    """Test loading known bad tests with no matching key"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        regexes = runner._load_known_bad_test_regexes("brcm/10.2.0.0_odp/tomahawk5")
    assert regexes == []


# Tests for _find_thresholds_for_benchmark


def test_find_thresholds_mono_match(runner, temp_sai_bench_config):
    """Mono binary picks the mono-keyed threshold by benchmark name"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    thresholds = runner._find_thresholds_for_benchmark(
        "HwEcmpGroupShrink", False, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert len(thresholds) == 1
    assert thresholds[0]["metric_key_regex"] == "HwEcmpGroupShrink"


def test_find_thresholds_multi_switch_match(runner, temp_sai_bench_config):
    """Multi-switch binary picks the multi_switch-keyed threshold"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    thresholds = runner._find_thresholds_for_benchmark(
        "RxSlowPathBenchmark", True, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert len(thresholds) == 1
    assert thresholds[0]["metric_key_regex"] == "cpu_rx_pps"


def test_find_thresholds_mono_falls_back_to_multi_switch(runner, temp_sai_bench_config):
    """If only a multi-switch threshold exists, mono falls back to it."""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    thresholds = runner._find_thresholds_for_benchmark(
        "RxSlowPathBenchmark", False, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert len(thresholds) == 1
    assert thresholds[0]["metric_key_regex"] == "cpu_rx_pps"


def test_find_thresholds_multi_switch_prefers_multi_over_mono(runner):
    """When both mono and multi_switch entries exist, the matching mode wins."""
    all_thresholds = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwStatsCollection": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwStatsCollection", "upper_bound": 1000}
                ],
            }
        ],
        "fboss.agent.hw.sai.benchmarks.multi_switch.HwStatsCollection": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwStatsCollection", "upper_bound": 9999}
                ],
            }
        ],
    }
    thresholds = runner._find_thresholds_for_benchmark(
        "HwStatsCollection", True, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert thresholds[0]["upper_bound"] == 9999

    thresholds = runner._find_thresholds_for_benchmark(
        "HwStatsCollection", False, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert thresholds[0]["upper_bound"] == 1000


def test_find_thresholds_no_match(runner, temp_sai_bench_config):
    """Unknown benchmark name returns no thresholds"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    thresholds = runner._find_thresholds_for_benchmark(
        "NonExistentBenchmark", False, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert thresholds == []


def test_find_thresholds_platform_mismatch(runner, temp_sai_bench_config):
    """Benchmark name matches but platform regex doesn't"""
    with patch("run_test.SAI_BENCH_CONFIG", new=temp_sai_bench_config):
        all_thresholds = runner._load_benchmark_thresholds()
    thresholds = runner._find_thresholds_for_benchmark(
        "HwEcmpGroupShrink", False, "brcm/10.2.0.0_odp/tomahawk5", all_thresholds
    )
    assert thresholds == []


def test_find_thresholds_skips_warmboot(runner):
    """Warmboot threshold entries are ignored"""
    all_thresholds = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwInitAndExit.warmboot": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [{"metric_key_regex": "HwInitAndExit", "upper_bound": 1}],
            }
        ],
    }
    thresholds = runner._find_thresholds_for_benchmark(
        "HwInitAndExit", False, "brcm/10.2.0.0_odp/tomahawk4", all_thresholds
    )
    assert thresholds == []


# Tests for _check_thresholds


def test_check_thresholds_pass(runner):
    """Test threshold check when metric is within bounds"""
    result = {"metrics": {"HwEcmpGroupShrink": 10000000000000}}
    thresholds = [
        {"metric_key_regex": "HwEcmpGroupShrink", "upper_bound": 25000000000000}
    ]
    assert runner._check_thresholds(result, thresholds) == []


def test_check_thresholds_upper_exceeded(runner):
    """Test threshold check when metric exceeds upper bound"""
    result = {"metrics": {"HwEcmpGroupShrink": 30000000000000}}
    thresholds = [
        {"metric_key_regex": "HwEcmpGroupShrink", "upper_bound": 25000000000000}
    ]
    violations = runner._check_thresholds(result, thresholds)
    assert len(violations) == 1
    assert "upper_bound" in violations[0]


def test_check_thresholds_lower_violated(runner):
    """Test threshold check when metric is below lower bound"""
    result = {"metrics": {"cpu_rx_pps": 100000}}
    thresholds = [{"metric_key_regex": "cpu_rx_pps", "lower_bound": 138800}]
    violations = runner._check_thresholds(result, thresholds)
    assert len(violations) == 1
    assert "lower_bound" in violations[0]


def test_check_thresholds_missing_metric_no_violation(runner):
    """Test that missing metric does not cause violation"""
    result = {"metrics": {"SomeOtherBenchmark": 10000000000000}}
    thresholds = [
        {"metric_key_regex": "HwEcmpGroupShrink", "upper_bound": 25000000000000}
    ]
    assert runner._check_thresholds(result, thresholds) == []


def test_parse_benchmark_output_folly_json_format(runner):
    """Test parsing folly --json output format with benchmark_name"""
    stdout = '{\n  "RibResolutionBenchmark": 1460000000000\n}\n{"cpu_time_usec": 1460000, "max_rss": 123456}\n'
    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="RibResolutionBenchmark"
    )
    assert result["benchmark_test_name"] == "RibResolutionBenchmark"
    assert result["benchmark_time_ps"] == "1460000000000"
    assert result["metrics"]["RibResolutionBenchmark"] == 1460000000000


def test_parse_benchmark_output_folly_json_with_pps(runner):
    """Test parsing folly --json output with cpu_rx_pps metric"""
    stdout = '{\n  "RxSlowPathBenchmark": 500000000\n}\n{"cpu_rx_pps": 200000}\n{"cpu_time_usec": 500000, "max_rss": 98765}\n'
    result = runner._parse_benchmark_output(
        "test_binary", stdout, benchmark_name="RxSlowPathBenchmark"
    )
    assert result["benchmark_test_name"] == "RxSlowPathBenchmark"
    assert result["benchmark_time_ps"] == "500000000"
    assert result["cpu_rx_pps"] == "200000"
