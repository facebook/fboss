# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for gtest parsing, filtering, and test list management in TestRunner."""

import json
import os
import tempfile
from unittest.mock import MagicMock

import pytest


class TestParseListTestOutput:
    """Tests for _parse_list_test_output which parses --gtest_list_tests output."""

    def test_standard_two_classes(self, runner):
        output = (
            b"HwVlanTest.\n  VlanApplyConfig\n  VlanDelete\nHwAclTest.\n  AclApply\n"
        )
        result = runner._parse_list_test_output(output)
        assert result == [
            "HwVlanTest.VlanApplyConfig",
            "HwVlanTest.VlanDelete",
            "HwAclTest.AclApply",
        ]

    def test_templated_tests_with_comments(self, runner):
        output = (
            b"BcmMirrorTest/0.  # TypeParam = folly::IPAddressV4\n"
            b"  ResolvedSpanMirror\n"
            b"  UnresolvedSpanMirror\n"
        )
        result = runner._parse_list_test_output(output)
        assert result == [
            "BcmMirrorTest/0.ResolvedSpanMirror",
            "BcmMirrorTest/0.UnresolvedSpanMirror",
        ]

    def test_empty_output(self, runner):
        result = runner._parse_list_test_output(b"")
        assert result == []

    def test_malformed_no_class_line(self, runner):
        output = b"  SomeMethod\n"
        with pytest.raises(RuntimeError):
            runner._parse_list_test_output(output)

    def test_single_class_single_method(self, runner):
        output = b"HwPortTest.\n  PortUp\n"
        result = runner._parse_list_test_output(output)
        assert result == ["HwPortTest.PortUp"]


class TestParseGtestRunOutput:
    """Tests for _parse_gtest_run_output which extracts result lines."""

    def test_ok_and_failed_lines_extracted(self, runner):
        output = (
            b"[==========] Running 2 tests\n"
            b"[ RUN      ] HwVlanTest.VlanApplyConfig\n"
            b"[       OK ] HwVlanTest.VlanApplyConfig (150 ms)\n"
            b"[ RUN      ] HwVlanTest.VlanDelete\n"
            b"[  FAILED  ] HwVlanTest.VlanDelete (200 ms)\n"
            b"[==========] 2 tests ran\n"
        )
        result = runner._parse_gtest_run_output(output)
        assert len(result) == 2
        assert "OK" in result[0]
        assert "FAILED" in result[1]

    def test_no_matching_lines(self, runner):
        output = b"Some random output\nAnother line\n"
        result = runner._parse_gtest_run_output(output)
        assert result == []

    def test_skipped_and_timeout_extracted(self, runner):
        output = (
            b"[  SKIPPED ] HwTest.Skip (0 ms)\n[  TIMEOUT ] HwTest.Slow (1200000 ms)\n"
        )
        result = runner._parse_gtest_run_output(output)
        assert len(result) == 2

    def test_empty_output(self, runner):
        result = runner._parse_gtest_run_output(b"")
        assert result == []


class TestAddTestPrefixToGtestResult:
    """Tests for _add_test_prefix_to_gtest_result which injects boot type prefix."""

    def test_ok_result_gets_prefix(self, runner):
        output = b"[       OK ] HwVlanTest.Config (150 ms)"
        result = runner._add_test_prefix_to_gtest_result(output, "cold_boot.")
        decoded = result.decode("utf-8")
        assert "cold_boot.HwVlanTest.Config" in decoded
        assert "(150 ms)" in decoded

    def test_failed_result_gets_prefix(self, runner):
        output = b"[  FAILED  ] HwVlanTest.Config (200 ms)"
        result = runner._add_test_prefix_to_gtest_result(output, "warm_boot.")
        decoded = result.decode("utf-8")
        assert "warm_boot.HwVlanTest.Config" in decoded

    def test_no_gtest_line_returns_unchanged(self, runner):
        output = b"Some random output without gtest markers"
        result = runner._add_test_prefix_to_gtest_result(output, "cold_boot.")
        assert result == output


class TestRegexMatching:
    """Tests for regex-based test filtering."""

    def test_exact_match(self, runner):
        assert runner._test_matches_any_regex(
            "HwVlanTest.VlanDelete", ["HwVlanTest\\.VlanDelete"]
        )

    def test_wildcard_match(self, runner):
        assert runner._test_matches_any_regex("HwRouteTest.RouteAdd", ["HwRoute.*"])

    def test_no_match(self, runner):
        assert not runner._test_matches_any_regex(
            "HwPortTest.PortUp", ["HwVlan.*", "HwAcl.*"]
        )

    def test_empty_regex_list(self, runner):
        assert not runner._test_matches_any_regex("HwVlanTest.Config", [])

    def test_is_known_bad_delegates(self, runner):
        runner._known_bad_test_regexes = ["HwVlan.*"]
        assert runner._is_known_bad_test("HwVlanTest.Delete")
        assert not runner._is_known_bad_test("HwPortTest.Up")

    def test_is_unsupported_delegates(self, runner):
        runner._unsupported_test_regexes = ["HwMirror.*"]
        assert runner._is_unsupported_test("HwMirrorTest.Span")
        assert not runner._is_unsupported_test("HwVlanTest.Config")


class TestGetTestRegexesFromFile:
    """Tests for loading regex patterns from known-bad/unsupported JSON files."""

    def test_matching_key_returns_regexes(self, runner, known_bad_tests_json):
        result = runner._get_test_regexes_from_file(
            file_path=known_bad_tests_json,
            test_dict_key="known_bad_tests",
            keys_to_try=["brcm/8.2/8.2/tomahawk"],
        )
        assert "HwVlanTest\\.VlanDelete" in result
        assert "HwAcl.*" in result
        assert len(result) == 2

    def test_fallback_key_used(self, runner, known_bad_tests_json):
        result = runner._get_test_regexes_from_file(
            file_path=known_bad_tests_json,
            test_dict_key="known_bad_tests",
            keys_to_try=["nonexistent", "brcm/8.2/8.2/tomahawk/mono"],
        )
        assert "HwRouteTest\\.MonoOnly" in result

    def test_multiple_keys_union(self, runner, known_bad_tests_json):
        result = runner._get_test_regexes_from_file(
            file_path=known_bad_tests_json,
            test_dict_key="known_bad_tests",
            keys_to_try=["brcm/8.2/8.2/tomahawk", "brcm/8.2/8.2/tomahawk/mono"],
        )
        assert "HwVlanTest\\.VlanDelete" in result
        assert "HwAcl.*" in result
        assert "HwRouteTest\\.MonoOnly" in result

    def test_no_matching_keys_returns_empty(self, runner, known_bad_tests_json, capsys):
        result = runner._get_test_regexes_from_file(
            file_path=known_bad_tests_json,
            test_dict_key="known_bad_tests",
            keys_to_try=["nonexistent/key"],
        )
        assert result == []
        captured = capsys.readouterr()
        assert "Warning: Could not find tests" in captured.out

    def test_file_not_found_returns_empty(self, runner, capsys):
        result = runner._get_test_regexes_from_file(
            file_path="/nonexistent/file.json",
            test_dict_key="known_bad_tests",
            keys_to_try=["any_key"],
        )
        assert result == []
        captured = capsys.readouterr()
        assert "Warning: Test file" in captured.out


class TestInitializeTestLists:
    """Tests for _initialize_test_lists which loads known-bad + unsupported regexes."""

    def test_skip_not_set_both_lists_empty(self, runner, capsys):
        mock_args = MagicMock()
        mock_args.skip_known_bad_tests = None
        runner._initialize_test_lists(mock_args)
        assert runner._known_bad_test_regexes == []
        assert runner._unsupported_test_regexes == []
        captured = capsys.readouterr()
        assert "--skip-known-bad-tests option is not set" in captured.out

    def test_no_agent_run_mode_loads_base_key_only(self, runner):
        """Without agent_run_mode, only the exact key is used."""
        data = {
            "known_bad_tests": {
                "brcm/8.2/8.2/tomahawk": [
                    {"test_name_regex": "HwVlanTest\\.VlanDelete", "reason": "T1"},
                    {"test_name_regex": "HwAcl.*", "reason": "T2"},
                ],
                "brcm/8.2/8.2/tomahawk/mono": [
                    {"test_name_regex": "HwRouteTest\\.MonoOnly", "reason": "T3"},
                ],
            }
        }
        with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".json") as f:
            json.dump(data, f)
            kb_path = f.name

        try:
            mock_args = MagicMock(spec=[])
            mock_args.skip_known_bad_tests = "brcm/8.2/8.2/tomahawk"

            runner._get_known_bad_tests_file = lambda: kb_path
            runner._get_unsupported_tests_file = lambda: "/nonexistent"

            runner._initialize_test_lists(mock_args)

            assert len(runner._known_bad_test_regexes) == 2
            assert "HwVlanTest\\.VlanDelete" in runner._known_bad_test_regexes
            assert "HwAcl.*" in runner._known_bad_test_regexes
            assert "HwRouteTest\\.MonoOnly" not in runner._known_bad_test_regexes
        finally:
            os.unlink(kb_path)

    def test_key_with_run_mode_suffix_also_loads_base(self, runner):
        """With agent_run_mode matching the key suffix, both keys are tried."""
        data = {
            "known_bad_tests": {
                "brcm/8.2/8.2/tomahawk": [
                    {"test_name_regex": "HwVlanTest\\.VlanDelete", "reason": "T1"},
                ],
                "brcm/8.2/8.2/tomahawk/mono": [
                    {"test_name_regex": "HwRouteTest\\.MonoOnly", "reason": "T2"},
                ],
            }
        }
        with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".json") as f:
            json.dump(data, f)
            kb_path = f.name

        try:
            mock_args = MagicMock()
            mock_args.skip_known_bad_tests = "brcm/8.2/8.2/tomahawk/mono"
            mock_args.agent_run_mode = "mono"

            runner._get_known_bad_tests_file = lambda: kb_path
            runner._get_unsupported_tests_file = lambda: "/nonexistent"

            runner._initialize_test_lists(mock_args)

            assert "HwRouteTest\\.MonoOnly" in runner._known_bad_test_regexes
            assert "HwVlanTest\\.VlanDelete" in runner._known_bad_test_regexes
        finally:
            os.unlink(kb_path)

    def test_key_without_suffix_also_tries_with_run_mode(self, runner):
        """With agent_run_mode, appended suffixed key is also tried."""
        data = {
            "known_bad_tests": {
                "brcm/8.2/8.2/tomahawk": [
                    {"test_name_regex": "HwVlanTest\\.VlanDelete", "reason": "T1"},
                ],
                "brcm/8.2/8.2/tomahawk/multi_switch": [
                    {"test_name_regex": "HwMultiTest\\.Only", "reason": "T2"},
                ],
            }
        }
        with tempfile.NamedTemporaryFile(mode="w", delete=False, suffix=".json") as f:
            json.dump(data, f)
            kb_path = f.name

        try:
            mock_args = MagicMock()
            mock_args.skip_known_bad_tests = "brcm/8.2/8.2/tomahawk"
            mock_args.agent_run_mode = "multi_switch"

            runner._get_known_bad_tests_file = lambda: kb_path
            runner._get_unsupported_tests_file = lambda: "/nonexistent"

            runner._initialize_test_lists(mock_args)

            assert "HwVlanTest\\.VlanDelete" in runner._known_bad_test_regexes
            assert "HwMultiTest\\.Only" in runner._known_bad_test_regexes
        finally:
            os.unlink(kb_path)

    def test_loads_both_known_bad_and_unsupported(
        self, runner, known_bad_tests_json, unsupported_tests_json
    ):
        mock_args = MagicMock(spec=[])
        mock_args.skip_known_bad_tests = "brcm/8.2/8.2/tomahawk"

        runner._get_known_bad_tests_file = lambda: known_bad_tests_json
        runner._get_unsupported_tests_file = lambda: unsupported_tests_json

        runner._initialize_test_lists(mock_args)

        assert len(runner._known_bad_test_regexes) == 2
        assert "HwVlanTest\\.VlanDelete" in runner._known_bad_test_regexes
        assert "HwAcl.*" in runner._known_bad_test_regexes

        assert len(runner._unsupported_test_regexes) == 1
        assert "HwMirrorTest\\..*" in runner._unsupported_test_regexes
