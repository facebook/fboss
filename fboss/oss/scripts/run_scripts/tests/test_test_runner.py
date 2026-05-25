# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for TestRunner orchestration: _run_test, _run_tests, and related
end-to-end execution paths (gtest output post-processing, synthesize-OK
fallback for early-exit binaries), plus the _load_from_file helper consumed
by _get_tests_to_run via args.filter_file."""

import os
import tempfile
from unittest.mock import patch

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
