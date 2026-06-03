# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for TestRunner orchestration: _run_test, _run_tests, and related
end-to-end execution paths (gtest output post-processing, synthesize-OK
fallback for early-exit binaries, warmboot loop, timeout handling, config
backup, filter merging), plus the _load_from_file helper consumed
by _get_tests_to_run via args.filter_file."""

import os
import subprocess
import tempfile
from unittest.mock import MagicMock, patch

from run_test import _load_from_file


class TestLoadFromFile:
    """_load_from_file is a module-level helper used by _get_tests_to_run
    (args.filter_file path) and BenchmarkTestRunner. Tests live here next to
    the consumer."""

    def test_basic(self):
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

    def test_nonexistent(self):
        """Test loading from a nonexistent file returns empty list"""
        result = _load_from_file("/nonexistent/path.conf")
        assert result == []

    def test_empty(self):
        """Test loading from an empty file returns empty list"""
        with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
            temp_file = f.name
        try:
            result = _load_from_file(temp_file)
            assert result == []
        finally:
            os.unlink(temp_file)

    def test_with_profile(self):
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

    def test_no_profile_includes_untagged_and_t(self):
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

    def test_comments_and_whitespace(self):
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


class TestRunTestGtestFallback:
    """Tests for _run_test gtest output post-processing (prefix injection,
    synthesize-OK fallback for empty output)."""

    @patch("subprocess.check_output")
    def test_preserves_skipped_does_not_synthesize_ok(
        self, mock_check_output, runner, mock_args
    ):
        """End-to-end: a SKIPPED gtest result must not be rewritten as OK."""
        mock_check_output.return_value = b"[  SKIPPED ] HwFooTest.Bar (5 ms)\n"
        # _get_test_run_cmd reads the module-level `args` global, which is only
        # bound under `if __name__ == "__main__":` — use create=True to inject it.
        with patch("run_test.args", new=mock_args, create=True):
            result = runner._run_test(
                conf_file="dummy.conf",
                test_prefix="cold_boot.",
                test_to_run="HwFooTest.Bar",
                setup_warmboot=False,
                sai_logging="WARN",
                fboss_logging="WARN",
            )
        decoded = result.decode("utf-8")
        assert "SKIPPED" in decoded
        assert "cold_boot.HwFooTest.Bar" in decoded
        # Critical: the fallback must NOT have rewritten this to "[       OK ]".
        assert "[       OK ]" not in decoded

    @patch("subprocess.check_output")
    def test_synthesize_ok_when_no_gtest_line(
        self, mock_check_output, runner, mock_args
    ):
        """Fallback path still works: empty output (e.g. --setup-for-warmboot early
        exit) should still synthesize an OK result so the test isn't lost from
        the summary."""
        mock_check_output.return_value = b""
        with patch("run_test.args", new=mock_args, create=True):
            result = runner._run_test(
                conf_file="dummy.conf",
                test_prefix="warm_boot.",
                test_to_run="HwFooTest.Bar",
                setup_warmboot=True,
                sai_logging="WARN",
                fboss_logging="WARN",
            )
        decoded = result.decode("utf-8")
        assert "[       OK ] warm_boot.HwFooTest.Bar" in decoded


class TestRunTestTimeout:
    """Test that subprocess.TimeoutExpired is converted to a [ TIMEOUT ]
    result line so the test surfaces in the summary."""

    @patch("subprocess.check_output")
    def test_timeout_synthesizes_timeout_line(
        self, mock_check_output, runner, mock_args
    ):
        mock_check_output.side_effect = subprocess.TimeoutExpired(
            cmd=["dummy"], timeout=300
        )
        with patch("run_test.args", new=mock_args, create=True):
            result = runner._run_test(
                conf_file="dummy.conf",
                test_prefix="cold_boot.",
                test_to_run="HwSlowTest.Slow",
                setup_warmboot=False,
                sai_logging="WARN",
                fboss_logging="WARN",
                test_run_timeout_in_second=300,
            )
        decoded = result.decode("utf-8")
        # Critical: timeout must produce a TIMEOUT line, NOT be silently
        # dropped or rewritten as OK.
        assert "[  TIMEOUT ]" in decoded
        assert "cold_boot.HwSlowTest.Slow" in decoded
        # Duration reported is timeout_in_second * 1000.
        assert "(300000 ms)" in decoded


class TestRunTestsWarmboot:
    """Tests for _run_tests cold→warm orchestration."""

    def _make_args(self, mock_args, coldboot_only):
        mock_args.coldboot_only = coldboot_only
        mock_args.sai_replayer_logging = None
        mock_args.simulator = None
        mock_args.sai_logging = "WARN"
        mock_args.fboss_logging = "WARN"
        mock_args.test_run_timeout = 300
        return mock_args

    def _make_conf(self, tmp_path):
        # _run_tests early-returns when conf_file does not exist on disk.
        conf = tmp_path / "test.conf"
        conf.write_text("")
        return str(conf)

    def test_warmboot_runs_both_phases_when_check_file_present(
        self, runner, mock_args, tmp_path
    ):
        check_file = tmp_path / "can_warm_boot"
        check_file.write_text("")
        runner._get_warmboot_check_file = lambda: str(check_file)
        args = self._make_args(mock_args, coldboot_only=False)
        conf = self._make_conf(tmp_path)

        with (
            patch.object(
                runner, "_run_test", return_value=b"[       OK ] x.HwT.t (1 ms)"
            ) as mock_run,
            patch.object(runner, "_setup_coldboot_test"),
            patch.object(runner, "_setup_warmboot_test"),
            patch.object(runner, "_end_run"),
        ):
            outputs = runner._run_tests(["HwT.t"], conf, args)

        # 1 cold + 1 warm = 2 invocations, 2 outputs.
        assert mock_run.call_count == 2
        assert len(outputs) == 2
        prefixes = [call.args[1] for call in mock_run.call_args_list]
        assert prefixes == [runner.COLDBOOT_PREFIX, runner.WARMBOOT_PREFIX]

    def test_coldboot_only_skips_warmboot(self, runner, mock_args, tmp_path):
        # Even when the check file exists, --coldboot_only must skip warmboot.
        check_file = tmp_path / "can_warm_boot"
        check_file.write_text("")
        runner._get_warmboot_check_file = lambda: str(check_file)
        args = self._make_args(mock_args, coldboot_only=True)
        conf = self._make_conf(tmp_path)

        with (
            patch.object(
                runner, "_run_test", return_value=b"[       OK ] x.HwT.t (1 ms)"
            ) as mock_run,
            patch.object(runner, "_setup_coldboot_test"),
            patch.object(runner, "_setup_warmboot_test") as mock_warm_setup,
            patch.object(runner, "_end_run"),
        ):
            outputs = runner._run_tests(["HwT.t"], conf, args)

        assert mock_run.call_count == 1
        assert len(outputs) == 1
        mock_warm_setup.assert_not_called()

    def test_warmboot_skipped_when_check_file_missing(
        self, runner, mock_args, tmp_path
    ):
        # Regression for the "warmboot file path mismatch" failure mode:
        # if _get_warmboot_check_file points at a path that doesn't exist
        # (e.g. wrong switch_index for the run mode), the warmboot pass is
        # silently skipped — no warmboot result will appear in the summary.
        missing = tmp_path / "does_not_exist"
        runner._get_warmboot_check_file = lambda: str(missing)
        args = self._make_args(mock_args, coldboot_only=False)
        conf = self._make_conf(tmp_path)

        with (
            patch.object(
                runner, "_run_test", return_value=b"[       OK ] x.HwT.t (1 ms)"
            ) as mock_run,
            patch.object(runner, "_setup_coldboot_test"),
            patch.object(runner, "_setup_warmboot_test") as mock_warm_setup,
            patch.object(runner, "_end_run"),
        ):
            outputs = runner._run_tests(["HwT.t"], conf, args)

        # Warmboot was requested (coldboot_only=False) but check file is
        # missing → only the coldboot pass runs.
        assert mock_run.call_count == 1
        assert len(outputs) == 1
        mock_warm_setup.assert_not_called()


class TestBackupAndModifyConfig:
    """Test that _backup_and_modify_config copies the config to /tmp and
    rewrites AUTOLOAD_BOARD_SETTINGS when --run-on-reference-board is set.
    Exercises _string_in_file + _replace_string_in_file transitively."""

    def test_reference_board_creates_modified_copy(self, runner, tmp_path):
        conf = tmp_path / "platform.materialized_JSON"
        conf.write_text("AUTOLOAD_BOARD_SETTINGS: 0\nother: value\n")

        modified_path = f"/tmp/modified-{conf.name}"
        try:
            args = MagicMock()
            args.run_on_reference_board = True
            with patch("run_test.args", new=args, create=True):
                returned = runner._backup_and_modify_config(str(conf))

            assert returned == modified_path
            assert os.path.isfile(modified_path)
            with open(modified_path) as f:
                new_contents = f.read()
            assert "AUTOLOAD_BOARD_SETTINGS: 1" in new_contents
            assert "AUTOLOAD_BOARD_SETTINGS: 0" not in new_contents
            # Original must be untouched.
            assert "AUTOLOAD_BOARD_SETTINGS: 0" in conf.read_text()
        finally:
            if os.path.isfile(modified_path):
                os.unlink(modified_path)

    def test_no_reference_board_returns_original_unmodified(self, runner, tmp_path):
        conf = tmp_path / "platform.materialized_JSON"
        conf.write_text("AUTOLOAD_BOARD_SETTINGS: 0\n")
        args = MagicMock()
        args.run_on_reference_board = False
        with patch("run_test.args", new=args, create=True):
            returned = runner._backup_and_modify_config(str(conf))
        # Should return original path, unchanged contents.
        assert returned == str(conf)
        assert "AUTOLOAD_BOARD_SETTINGS: 0" in conf.read_text()


class TestGetTestsToRunFilter:
    def _make_args(self, **overrides):
        args = MagicMock()
        args.filter = None
        args.filter_file = None
        args.profile = None
        for k, v in overrides.items():
            setattr(args, k, v)
        return args

    def test_filter_file_overrides_filter_and_joins_with_colon(self, runner):
        with tempfile.NamedTemporaryFile(mode="w", suffix=".conf", delete=False) as f:
            f.write("HwVlan* default\n")
            f.write("HwAcl* default\n")
            filter_file_path = f.name

        try:
            args = self._make_args(
                filter="ignored_when_filter_file_set",
                filter_file=filter_file_path,
                profile="default",
            )
            with (
                patch("run_test.args", new=args, create=True),
                patch.object(
                    runner, "_list_tests_to_run", return_value=["HwVlanTest.A"]
                ) as mock_list,
                patch.object(runner, "_is_known_bad_test", return_value=False),
                patch.object(runner, "_is_unsupported_test", return_value=False),
            ):
                runner._get_tests_to_run()

            first_filter = mock_list.call_args_list[0].args[0]
            assert ":" in first_filter
            assert "HwVlan*" in first_filter
            assert "HwAcl*" in first_filter
            assert "ignored_when_filter_file_set" not in first_filter
        finally:
            os.unlink(filter_file_path)

    def test_known_bad_tests_excluded_from_final_filter(self, runner):
        listed = ["HwVlanTest.Delete", "HwRouteTest.Add"]

        def fake_is_known_bad(test):
            return test == "HwVlanTest.Delete"

        args = self._make_args()
        with (
            patch("run_test.args", new=args, create=True),
            patch.object(
                runner, "_list_tests_to_run", side_effect=[listed, ["HwRouteTest.Add"]]
            ) as mock_list,
            patch.object(runner, "_is_known_bad_test", side_effect=fake_is_known_bad),
            patch.object(runner, "_is_unsupported_test", return_value=False),
        ):
            result = runner._get_tests_to_run()

        assert mock_list.call_args_list[0].args[0] == "*"
        second_filter = mock_list.call_args_list[1].args[0]
        assert "HwRouteTest.Add" in second_filter
        assert "HwVlanTest.Delete" not in second_filter
        assert result == ["HwRouteTest.Add"]
