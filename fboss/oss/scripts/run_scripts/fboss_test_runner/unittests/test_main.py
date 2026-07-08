#!/usr/bin/env python3
# @noautodeps
# Copyright Meta Platforms, Inc. and affiliates.

"""Unit tests for main.py argument parsing + cross-argument validation."""

import os
import sys
import unittest
from argparse import Namespace
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

import fboss_test_runner.main as main_mod
from fboss_test_runner.main import _parse_args, main


class ParseArgsValidationTest(unittest.TestCase):
    def test_filter_and_filter_file_are_mutually_exclusive(self):
        with self.assertRaises(ValueError):
            _parse_args(["sai", "--filter", "X", "--filter_file", "f.conf"])

    def test_profile_requires_filter_file(self):
        with self.assertRaises(ValueError):
            _parse_args(["sai", "--profile", "p"])

    def test_filter_only_is_valid(self):
        args = _parse_args(["sai", "--filter", "X"])
        self.assertEqual(args.filter, "X")
        self.assertIsNone(args.filter_file)

    def test_profile_with_filter_file_is_valid(self):
        args = _parse_args(["sai", "--profile", "p", "--filter_file", "f.conf"])
        self.assertEqual(args.profile, "p")
        self.assertEqual(args.filter_file, "f.conf")


class LogBundleFlagTest(unittest.TestCase):
    def test_log_bundle_defaults_off(self):
        self.assertFalse(_parse_args(["sai"]).log_bundle)

    def test_log_bundle_enabled_when_passed_after_subcommand(self):
        self.assertTrue(_parse_args(["sai", "--log-bundle"]).log_bundle)

    def test_log_bundle_before_subcommand_is_rejected(self):
        # The flag lives on the subparsers, so it must follow the subcommand.
        with self.assertRaises(SystemExit):
            _parse_args(["--log-bundle", "sai"])


class MainLogBundleGatingTest(unittest.TestCase):
    """main() wraps the run in a LogCapture bundle only when --log-bundle is set;
    otherwise it dispatches the runner directly. (LogCapture's own behavior is
    covered in test_log_capture.)"""

    def _run_main(self, *, log_bundle: bool) -> SimpleNamespace:
        dispatched = MagicMock()
        with (
            patch.object(sys, "argv", ["run_test.py", "sai"]),
            patch.dict(
                os.environ,
                {"FBOSS_BIN": "/opt/fboss/bin", "FBOSS_LIB": "/opt/fboss/lib"},
            ),
            patch.object(main_mod.os, "chdir"),
            patch.object(main_mod, "LogCapture") as log_capture,
            patch.object(main_mod, "setup_fboss_env"),
            patch.object(
                main_mod,
                "_parse_args",
                return_value=Namespace(func=dispatched, log_bundle=log_bundle),
            ),
        ):
            main()
        return SimpleNamespace(log_capture=log_capture, dispatched=dispatched)

    def test_no_flag_dispatches_without_capture(self):
        m = self._run_main(log_bundle=False)
        m.log_capture.assert_not_called()
        m.dispatched.assert_called_once()

    def test_flag_wraps_run_in_log_capture(self):
        m = self._run_main(log_bundle=True)
        # main wraps the run in `with LogCapture(test_type):`.
        m.log_capture.assert_called_once_with("sai")
        m.log_capture.return_value.__enter__.assert_called_once()
        m.log_capture.return_value.__exit__.assert_called_once()
        m.dispatched.assert_called_once()


if __name__ == "__main__":
    unittest.main()
