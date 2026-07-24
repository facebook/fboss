#!/usr/bin/env python3
# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps

"""Unit tests for SaiBenchmarkSuite (SAI-specific benchmark behavior)."""

from unittest.mock import Mock

import pytest
from fboss_test_runner.frameworks import sai_benchmark_suite
from fboss_test_runner.frameworks.sai_benchmark_suite import SaiBenchmarkSuite


@pytest.fixture
def suite():
    return SaiBenchmarkSuite()


@pytest.fixture
def sai_args():
    args = Mock()
    args.agent_run_mode = "mono"
    args.num_npus = 1
    args.config = None
    args.mgmt_if = "eth0"
    args.platform_mapping_override_path = None
    args.fruid_path = None
    args.sai_logging = "WARN"
    return args


# ---- binary selection ----------------------------------------------------


def test_binary_mono(suite, sai_args):
    assert suite.binary_path(sai_args) == "/opt/fboss/bin/sai_all_benchmarks-sai_impl"


def test_binary_multi_switch(suite, sai_args):
    sai_args.agent_run_mode = "multi_switch"
    assert (
        suite.binary_path(sai_args)
        == "/opt/fboss/bin/sai_multi_switch_all_benchmarks-sai_impl"
    )


# ---- build_cmd -----------------------------------------------------------


def test_build_cmd_mono(suite, sai_args):
    cmd = suite.build_cmd("/bin/b", sai_args)
    assert "--flexports" in cmd
    assert "--enable_sai_log" in cmd
    assert "--multi_switch" not in cmd


def test_build_cmd_multi_switch(suite, sai_args):
    sai_args.agent_run_mode = "multi_switch"
    cmd = suite.build_cmd("/bin/b", sai_args)
    assert "--multi_switch" in cmd
    assert "--hw_agent_for_testing" in cmd
    assert "--flexports" not in cmd
    assert "--enable_sai_log" not in cmd


def test_build_cmd_multi_npu(suite, sai_args):
    sai_args.agent_run_mode = "multi_switch"
    sai_args.num_npus = 2
    assert "--multi_npu_platform_mapping" in suite.build_cmd("/bin/b", sai_args)


def test_build_cmd_bm_regex(suite, sai_args):
    cmd = suite.build_cmd("/bin/b", sai_args, benchmark_name="HwEcmpGroupShrink")
    assert "--bm_regex" in cmd
    assert "^HwEcmpGroupShrink$" in cmd


def test_build_cmd_with_config(suite, sai_args):
    sai_args.config = "/path/cfg"
    sai_args.mgmt_if = "eth1"
    cmd = suite.build_cmd("/bin/b", sai_args)
    assert "--config" in cmd and "/path/cfg" in cmd
    assert "--mgmt-if" in cmd and "eth1" in cmd


# ---- known_bad_keys ------------------------------------------------------


def test_known_bad_keys_strips_run_mode(suite):
    assert suite.known_bad_keys("brcm/x/y/tomahawk4/mono") == [
        "brcm/x/y/tomahawk4/mono",
        "brcm/x/y/tomahawk4",
    ]


def test_known_bad_keys_plain(suite):
    assert suite.known_bad_keys("brcm/x/y/tomahawk4") == ["brcm/x/y/tomahawk4"]


# ---- find_thresholds (dotted per-benchmark keys) -------------------------


@pytest.fixture
def thresholds():
    return {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwEcmpGroupShrink": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwEcmpGroupShrink", "upper_bound": 25}
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
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwStatsCollection": [
            {
                "test_config_regex": ".*/tomahawk4.*",
                "thresholds": [
                    {"metric_key_regex": "HwStatsCollection", "upper_bound": 1000}
                ],
            }
        ],
    }


def test_find_thresholds_mono(suite, sai_args, thresholds):
    res = suite.find_thresholds(
        "HwEcmpGroupShrink", "brcm/x/tomahawk4", thresholds, sai_args
    )
    assert res[0]["metric_key_regex"] == "HwEcmpGroupShrink"


def test_find_thresholds_multi_switch_prefers_multi(suite, sai_args, thresholds):
    sai_args.agent_run_mode = "multi_switch"
    res = suite.find_thresholds(
        "HwStatsCollection", "brcm/x/tomahawk4", thresholds, sai_args
    )
    assert res[0]["upper_bound"] == 9999


def test_find_thresholds_mono_prefers_mono(suite, sai_args, thresholds):
    res = suite.find_thresholds(
        "HwStatsCollection", "brcm/x/tomahawk4", thresholds, sai_args
    )
    assert res[0]["upper_bound"] == 1000


def test_find_thresholds_platform_mismatch(suite, sai_args, thresholds):
    assert (
        suite.find_thresholds(
            "HwEcmpGroupShrink", "brcm/x/tomahawk5", thresholds, sai_args
        )
        == []
    )


def test_find_thresholds_skips_warmboot(suite, sai_args):
    thr = {
        "fboss.agent.hw.sai.benchmarks.sai_bench_test.HwInitAndExit.warmboot": [
            {"test_config_regex": ".*", "thresholds": [{"metric_key_regex": "x"}]}
        ]
    }
    assert (
        suite.find_thresholds("HwInitAndExit", "brcm/x/tomahawk4", thr, sai_args) == []
    )


# ---- config path + services ----------------------------------------------


def test_config_path(suite):
    assert suite.config_path() == sai_benchmark_suite.SAI_BENCH_CONFIG


def test_setup_noop_for_mono(suite, sai_args, monkeypatch):
    mock_start = Mock()
    monkeypatch.setattr(
        sai_benchmark_suite, "setup_and_start_hw_agent_service", mock_start
    )
    suite.setup(sai_args)  # mono -> no service
    mock_start.assert_not_called()


def test_setup_starts_agent_for_multi_switch(suite, sai_args, monkeypatch):
    mock_start = Mock()
    monkeypatch.setattr(
        sai_benchmark_suite, "setup_and_start_hw_agent_service", mock_start
    )
    sai_args.agent_run_mode = "multi_switch"
    sai_args.config = "/cfg"
    suite.setup(sai_args)
    mock_start.assert_called_once()
