# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for TestRunner orchestration: _run_test, _run_tests, and related
end-to-end execution paths (gtest output post-processing, synthesize-OK
fallback for early-exit binaries, warmboot loop, timeout handling, config
backup, filter merging), plus the load_from_file helper consumed
by _get_tests_to_run via args.filter_file."""

import os
import subprocess
import tempfile
from unittest.mock import MagicMock, patch

from fboss_test_runner.result_types import GtestResult, GtestStatus, RunOutcome
from fboss_test_runner.runners.utils import load_from_file


class TestLoadFromFile:
    """load_from_file is a module-level helper used by _get_tests_to_run
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
            result = load_from_file(temp_file)
            assert result == ["entry1", "entry2", "entry3"]
        finally:
            os.unlink(temp_file)

    def test_nonexistent(self):
        """Test loading from a nonexistent file returns empty list"""
        result = load_from_file("/nonexistent/path.conf")
        assert result == []

    def test_empty(self):
        """Test loading from an empty file returns empty list"""
        with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".conf") as f:
            temp_file = f.name
        try:
            result = load_from_file(temp_file)
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
            result = load_from_file(temp_file, profile="p1")
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
            result = load_from_file(temp_file)
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
            result = load_from_file(temp_file)
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
        # _get_test_run_cmd reads args off the runner instance (self.args);
        # patch it on the runner for the duration of the call.
        with patch.object(runner, "args", new=mock_args):
            outcome = runner._run_test(
                conf_file="dummy.conf",
                test_prefix="cold_boot.",
                test_to_run="HwFooTest.Bar",
                setup_warmboot=False,
            )
        assert len(outcome.results) == 1
        result = outcome.results[0]
        assert result.status == GtestStatus.SKIPPED
        assert result.test_name == "cold_boot.HwFooTest.Bar"
        # Critical: the fallback must NOT have rewritten this to OK.
        assert result.status != GtestStatus.OK

    @patch("subprocess.check_output")
    def test_synthesize_ok_when_no_gtest_line(
        self, mock_check_output, runner, mock_args
    ):
        """Fallback path still works: empty output (e.g. --setup-for-warmboot early
        exit) should still synthesize an OK result so the test isn't lost from
        the summary."""
        mock_check_output.return_value = b""
        with patch.object(runner, "args", new=mock_args, create=True):
            outcome = runner._run_test(
                conf_file="dummy.conf",
                test_prefix="warm_boot.",
                test_to_run="HwFooTest.Bar",
                setup_warmboot=True,
            )
        assert len(outcome.results) == 1
        result = outcome.results[0]
        assert result.status == GtestStatus.OK
        assert result.test_name == "warm_boot.HwFooTest.Bar"


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
        mock_args.test_run_timeout = 300
        with patch.object(runner, "args", new=mock_args, create=True):
            outcome = runner._run_test(
                conf_file="dummy.conf",
                test_prefix="cold_boot.",
                test_to_run="HwSlowTest.Slow",
                setup_warmboot=False,
            )
        assert len(outcome.results) == 1
        result = outcome.results[0]
        # Critical: timeout must produce a TIMEOUT result, NOT be silently
        # dropped or rewritten as OK.
        assert result.status == GtestStatus.TIMEOUT
        assert result.test_name == "cold_boot.HwSlowTest.Slow"
        # Duration reported is timeout_in_second * 1000.
        assert result.duration_ms == 300000


class TestRunTestsWarmboot:
    """Tests for _run_tests cold→warm orchestration."""

    def _make_args(self, mock_args, coldboot_only):
        mock_args.coldboot_only = coldboot_only
        mock_args.sai_replayer_logging = None
        mock_args.simulator = None
        mock_args.sai_logging = "WARN"
        mock_args.fboss_logging = "WARN"
        mock_args.test_run_timeout = 300
        mock_args.num_warmboot_iterations = 1
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
                runner,
                "_run_test",
                return_value=RunOutcome(
                    "[ OK ] x.HwT.t (1 ms)", [GtestResult("x.HwT.t", GtestStatus.OK, 1)]
                ),
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
                runner,
                "_run_test",
                return_value=RunOutcome(
                    "[ OK ] x.HwT.t (1 ms)", [GtestResult("x.HwT.t", GtestStatus.OK, 1)]
                ),
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
                runner,
                "_run_test",
                return_value=RunOutcome(
                    "[ OK ] x.HwT.t (1 ms)", [GtestResult("x.HwT.t", GtestStatus.OK, 1)]
                ),
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


class TestSimulatorEnv:
    """_apply_simulator_env overlays simulator vars onto a per-instance env
    copy, so simulator runs never leak env across runner instances (the env
    used to live on a shared ClassVar)."""

    def test_xgs_overlay(self, runner):
        runner._apply_simulator_env("th4")
        assert runner.env_var["BCM_SIM_PATH"] == "1"
        assert runner.env_var["SOC_TARGET_SERVER"] == "127.0.0.1"
        assert runner.env_var["SOC_TARGET_PORT"] == "22222"

    def test_dnx_overlay(self, runner):
        runner._apply_simulator_env("j3")
        assert runner.env_var["ADAPTER_DEVID_0"] == "8860"
        assert runner.env_var["SOC_TARGET_SERVER"] == "localhost"
        assert runner.env_var["SAI_BOOT_FLAGS"] == "0x1020000"

    def test_no_simulator_leaves_env_unchanged(self, runner):
        before = dict(runner.env_var)
        runner._apply_simulator_env(None)
        assert runner.env_var == before

    def test_env_is_per_instance_no_leak(self, runner):
        # Applying simulator env to one runner must not leak onto a
        # separate instance.
        other = type(runner)()
        assert "SOC_TARGET_PORT" not in other.env_var
        runner._apply_simulator_env("th4")
        assert "SOC_TARGET_PORT" in runner.env_var
        assert "SOC_TARGET_PORT" not in other.env_var


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
            with patch.object(runner, "args", new=args, create=True):
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
        with patch.object(runner, "args", new=args, create=True):
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
                patch.object(runner, "args", new=args, create=True),
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
            patch.object(runner, "args", new=args, create=True),
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


class TestResolveTestsFile:
    """Tests for the shared TestRunner._resolve_tests_file helper used by every
    runner: prefer a caller-supplied override when it exists, else fall back to
    the default file when it exists, else return "" (no filtering). Per-runner
    tests only assert their default file wires through; the resolution logic is
    exercised here once."""

    _EXISTS = "fboss_test_runner.runners.test_runner.os.path.exists"
    _DEFAULT = "./share/known_bad_tests/default.materialized_JSON"

    def test_override_used_when_present(self, runner):
        with patch(self._EXISTS, return_value=True):
            assert (
                runner._resolve_tests_file(
                    "/tmp/override.json", self._DEFAULT, "known_bad"
                )
                == "/tmp/override.json"
            )

    def test_override_missing_falls_back_to_default(self, runner):
        # Override set but absent -> use the (present) default file.
        with patch(self._EXISTS, side_effect=lambda p: p == self._DEFAULT):
            assert (
                runner._resolve_tests_file(
                    "/tmp/missing.json", self._DEFAULT, "known_bad"
                )
                == self._DEFAULT
            )

    def test_override_and_default_missing_returns_empty(self, runner):
        with patch(self._EXISTS, return_value=False):
            assert (
                runner._resolve_tests_file(
                    "/tmp/missing.json", self._DEFAULT, "known_bad"
                )
                == ""
            )

    def test_default_used_when_present(self, runner):
        with patch(self._EXISTS, return_value=True):
            assert (
                runner._resolve_tests_file(None, self._DEFAULT, "unsupported")
                == self._DEFAULT
            )

    def test_default_missing_returns_empty(self, runner):
        with patch(self._EXISTS, return_value=False):
            assert runner._resolve_tests_file(None, self._DEFAULT, "unsupported") == ""

    def test_no_default_returns_empty(self, runner):
        # A runner with no default file (default_file="") and no override.
        assert runner._resolve_tests_file(None, "", "unsupported") == ""
