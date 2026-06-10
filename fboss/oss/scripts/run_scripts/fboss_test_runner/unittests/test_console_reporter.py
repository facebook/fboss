# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for TestRunner._print_output_summary (console reporter).

Summary remaps gtest OK/FAILED/SKIPPED to PASSED/FAILED/SKIPPED.
Counts only reflect tests that actually ran — filtered tests are dropped.
"""

from unittest.mock import patch

import pytest
from fboss_test_runner.result_types import GtestResult, GtestStatus


class TestPrintOutputSummary:
    @pytest.fixture(autouse=True)
    def _isolate_cwd(self, monkeypatch, tmp_path):
        """_print_output_summary calls _write_results_to_csv which writes
        hwtest_results_<timestamp>.csv into the current working directory.
        Sandbox each test in tmp_path so droppings don't leak into the repo."""
        monkeypatch.chdir(tmp_path)

    def test_ok_rendered_as_passed(self, runner, capsys):
        runner._print_output_summary(
            [GtestResult("cold_boot.HwFooTest.Bar", GtestStatus.OK, 100)]
        )
        out = capsys.readouterr().out
        assert "[ PASSED ] cold_boot.HwFooTest.Bar (100 ms)" in out
        assert "PASSED : 1" in out
        assert "OK :" not in out
        assert "[       OK ]" not in out

    def test_failed_skipped_timeout_preserved(self, runner, capsys):
        runner._print_output_summary(
            [
                GtestResult("cold_boot.HwFailTest.A", GtestStatus.FAILED, 200),
                GtestResult("cold_boot.HwSkipTest.B", GtestStatus.SKIPPED, 5),
                GtestResult("cold_boot.HwSlowTest.C", GtestStatus.TIMEOUT, 1200000),
            ]
        )
        out = capsys.readouterr().out
        assert "FAILED : 1" in out
        assert "SKIPPED : 1" in out
        assert "TIMEOUT : 1" in out
        assert "PASSED : 0" in out

    def test_mixed_outputs_counted_correctly(self, runner, capsys):
        runner._print_output_summary(
            [
                GtestResult("cold_boot.HwA.t", GtestStatus.OK, 10),
                GtestResult("warm_boot.HwA.t", GtestStatus.OK, 8),
                GtestResult("cold_boot.HwB.t", GtestStatus.FAILED, 20),
                GtestResult("cold_boot.HwC.t", GtestStatus.SKIPPED, 1),
            ]
        )
        out = capsys.readouterr().out
        assert "PASSED : 2" in out
        assert "FAILED : 1" in out
        assert "SKIPPED : 1" in out
        assert "TIMEOUT : 0" in out

    def test_filtered_tests_not_counted(self, runner, mock_args, capsys):
        runner._known_bad_test_regexes = ["HwBad.*"]
        runner._unsupported_test_regexes = []
        with (
            patch.object(
                runner,
                "_list_tests_to_run",
                side_effect=[
                    ["HwBadTest.A", "HwGoodTest.B"],
                    ["HwGoodTest.B"],
                ],
            ),
            patch("run_test.args", new=mock_args, create=True),
        ):
            mock_args.filter = None
            mock_args.filter_file = None
            runner._get_tests_to_run()
        runner._print_output_summary(
            [GtestResult("cold_boot.HwGoodTest.B", GtestStatus.OK, 100)]
        )
        out = capsys.readouterr().out
        assert "PASSED : 1" in out
        assert "SKIPPED : 0" in out

    def test_csv_uses_mapped_status(self, runner, tmp_path):
        runner._print_output_summary(
            [
                GtestResult("cold_boot.HwA.t", GtestStatus.OK, 10),
                GtestResult("cold_boot.HwB.t", GtestStatus.FAILED, 20),
                GtestResult("cold_boot.HwC.t", GtestStatus.SKIPPED, 1),
                GtestResult("cold_boot.HwD.t", GtestStatus.TIMEOUT, 1200000),
            ]
        )
        csv_files = list(tmp_path.glob("hwtest_results_*.csv"))
        assert len(csv_files) == 1
        contents = csv_files[0].read_text()
        assert "Test Name,Result" in contents
        assert "cold_boot.HwA.t,PASSED" in contents
        assert "cold_boot.HwB.t,FAILED" in contents
        assert "cold_boot.HwC.t,SKIPPED" in contents
        assert "cold_boot.HwD.t,TIMEOUT" in contents
        assert ",OK" not in contents
