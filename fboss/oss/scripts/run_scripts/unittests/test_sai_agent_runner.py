# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for SaiAgentTestRunner.

Covers the mono vs multi_switch branching that drives binary, warmboot file,
SAI replayer flags, and platform-mapping override placement, plus the
production-feature filtering implemented in _filter_tests."""

import json
from unittest.mock import MagicMock, patch

import pytest
from run_test import SaiAgentTestRunner


@pytest.fixture
def sai_agent_runner():
    return SaiAgentTestRunner()


def _make_args(**overrides):
    args = MagicMock()
    args.agent_run_mode = "multi_switch"
    args.num_npus = 1
    args.platform_mapping_override_path = None
    args.mgmt_if = "eth0"
    args.enable_production_features = None
    args.list_tests_for_features = None
    args.production_features = None
    args.known_bad_tests_file = None
    args.unsupported_tests_file = None
    for k, v in overrides.items():
        setattr(args, k, v)
    return args


class TestWarmbootCheckFile:
    """Mono uses switch_index=0 (can_warm_boot_0); multi_switch uses
    switch_index=None (can_warm_boot). Mismatched paths silently skip the
    warmboot."""

    def test_mono_uses_switch_index_zero(self, sai_agent_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="mono")):
            assert sai_agent_runner._get_warmboot_check_file().endswith("_0")

    def test_multi_switch_uses_no_switch_index(self, sai_agent_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="multi_switch")):
            # multi_switch path must NOT have the _<index> suffix.
            path = sai_agent_runner._get_warmboot_check_file()
            assert not path.endswith("_0")


class TestSaiReplayerLoggingFlagsByMode:
    """In multi_switch the SAI replayer is configured via the systemd unit
    file (hw agent runs as a service), so the test binary must NOT receive
    --enable-replayer flags. In mono the flags go on the binary."""

    def test_multi_switch_returns_empty_when_path_set(self, sai_agent_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="multi_switch")):
            assert sai_agent_runner._get_sai_replayer_logging_flags("/tmp/replay") == []

    def test_mono_returns_replayer_flags_when_path_set(self, sai_agent_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="mono")):
            flags = sai_agent_runner._get_sai_replayer_logging_flags("/tmp/replay")
            assert "--enable-replayer" in flags
            assert "--sai-log" in flags
            assert "/tmp/replay" in flags

    def test_returns_empty_when_path_none(self, sai_agent_runner):
        # Regardless of mode, no path → no flags.
        with patch("run_test.args", new=_make_args(agent_run_mode="mono")):
            assert sai_agent_runner._get_sai_replayer_logging_flags(None) == []


class TestTestRunArgsByMode:
    """--platform_mapping_override_path is added to the binary args ONLY in
    mono mode — multi_switch passes it via the systemd unit file instead."""

    def test_mono_includes_platform_mapping_override(self, sai_agent_runner):
        with patch(
            "run_test.args",
            new=_make_args(
                agent_run_mode="mono",
                platform_mapping_override_path="/tmp/pm.json",
            ),
        ):
            args_list = sai_agent_runner._get_test_run_args("dummy.conf")
            assert "--platform_mapping_override_path" in args_list
            assert "/tmp/pm.json" in args_list

    def test_multi_switch_omits_platform_mapping_override(self, sai_agent_runner):
        with patch(
            "run_test.args",
            new=_make_args(
                agent_run_mode="multi_switch",
                platform_mapping_override_path="/tmp/pm.json",
            ),
        ):
            args_list = sai_agent_runner._get_test_run_args("dummy.conf")
            # Override must NOT appear; multi_switch routes it via systemd unit.
            assert "--platform_mapping_override_path" not in args_list
            assert "/tmp/pm.json" not in args_list


class TestFilterTestsProductionFeatures:
    """--enable-production-features ASIC: load asicToFeatureNames from the
    JSON, then per-test invoke the binary with --list_production_feature and
    keep tests whose feature set is a subset of the ASIC's features."""

    def _features_json(self, tmp_path, mapping):
        path = tmp_path / "features.json"
        path.write_text(json.dumps({"asicToFeatureNames": mapping}))
        return str(path)

    def test_keeps_tests_whose_features_are_subset_of_asic(
        self, sai_agent_runner, tmp_path
    ):
        path = self._features_json(
            tmp_path, {"tomahawk5": ["ACL_COUNTER", "DLB", "SINGLE_ACL_TABLE"]}
        )

        # First test reports features that are a subset → keep.
        # Second test reports a feature NOT in the ASIC → drop.
        def fake_run(cmd, **_):
            test = next(
                arg.split("=", 1)[1] for arg in cmd if arg.startswith("--gtest_filter=")
            )
            if test == "AgentAclTest.OK":
                stdout = "Feature List: ACL_COUNTER\n"
            else:
                stdout = "Feature List: NOT_SUPPORTED_FEATURE\n"
            return MagicMock(stdout=stdout)

        with (
            patch(
                "run_test.args",
                new=_make_args(
                    agent_run_mode="multi_switch",
                    enable_production_features="tomahawk5",
                    production_features=path,
                ),
            ),
            patch("run_test.subprocess.run", side_effect=fake_run),
        ):
            kept = sai_agent_runner._filter_tests(
                ["AgentAclTest.OK", "AgentExoticTest.Bad"]
            )
        assert kept == ["AgentAclTest.OK"]

    def test_no_filter_when_neither_flag_set(self, sai_agent_runner):
        with patch(
            "run_test.args",
            new=_make_args(agent_run_mode="multi_switch"),
        ):
            tests = ["AgentA.t", "AgentB.t"]
            assert sai_agent_runner._filter_tests(tests) == tests
