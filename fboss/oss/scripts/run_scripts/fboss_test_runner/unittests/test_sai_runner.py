# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for SaiTestRunner."""

from unittest.mock import MagicMock, patch

import pytest
from fboss_test_runner.runners.sai_test_runner import (
    SAI_HW_KNOWN_BAD_TESTS,
    SAI_UNSUPPORTED_TESTS,
    SaiTestRunner,
)


@pytest.fixture
def sai_runner():
    return SaiTestRunner()


def _make_args(**overrides):
    args = MagicMock()
    args.mgmt_if = "eth0"
    args.platform_mapping_override_path = None
    args.known_bad_tests_file = None
    args.unsupported_tests_file = None
    for k, v in overrides.items():
        setattr(args, k, v)
    return args


class TestSaiReplayerLoggingFlags:
    def test_path_set_emits_five_flags_in_order(self, sai_runner):
        flags = sai_runner._get_sai_replayer_logging_flags("/tmp/replay.log")
        assert flags == [
            "--enable-replayer",
            "--enable_get_attr_log",
            "--enable_packet_log",
            "--sai-log",
            "/tmp/replay.log",
        ]

    def test_path_none_returns_empty(self, sai_runner):
        assert sai_runner._get_sai_replayer_logging_flags(None) == []


class TestRunArgsPlatformMappingOverride:
    def test_omits_override_when_unset(self, sai_runner):
        with patch.object(sai_runner, "args", new=_make_args()):
            args_list = sai_runner._get_test_run_args("conf.yaml")
        assert args_list == ["--config", "conf.yaml", "--mgmt-if", "eth0"]

    def test_appends_override_when_set(self, sai_runner):
        with patch.object(
            sai_runner,
            "args",
            new=_make_args(platform_mapping_override_path="/tmp/pm.json"),
        ):
            args_list = sai_runner._get_test_run_args("conf.yaml")
        idx = args_list.index("--platform_mapping_override_path")
        assert args_list[idx + 1] == "/tmp/pm.json"


class TestDefaultTestsFiles:
    # Only assert the default files wire through; the override / fallback /
    # missing-file resolution logic is covered by TestResolveTestsFile in
    # test_test_runner. Mock os.path.exists so the default resolves.
    _EXISTS = "fboss_test_runner.runners.test_runner.os.path.exists"

    def test_known_bad_default_file(self, sai_runner):
        with (
            patch.object(sai_runner, "args", new=_make_args()),
            patch(self._EXISTS, return_value=True),
        ):
            assert sai_runner._get_known_bad_tests_file() == SAI_HW_KNOWN_BAD_TESTS

    def test_unsupported_default_file(self, sai_runner):
        with (
            patch.object(sai_runner, "args", new=_make_args()),
            patch(self._EXISTS, return_value=True),
        ):
            assert sai_runner._get_unsupported_tests_file() == SAI_UNSUPPORTED_TESTS
