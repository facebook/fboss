#!/usr/bin/env python3
# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps

"""Unit tests for QsfpBenchmarkSuite (QSFP-specific benchmark behavior)."""

from unittest.mock import Mock, patch

import pytest
from fboss_test_runner.frameworks import qsfp_benchmark_suite
from fboss_test_runner.frameworks.qsfp_benchmark_suite import (
    QSFP_BENCH_TARGET,
    QsfpBenchmarkSuite,
)


@pytest.fixture
def suite():
    return QsfpBenchmarkSuite()


@pytest.fixture
def qsfp_args():
    args = Mock()
    args.qsfp = True
    args.qsfp_config = "/share/qsfp_test_configs/montblanc.materialized_JSON"
    args.force_5pim_fuji = False
    args.port_manager_mode = False
    args.fruid_path = None
    return args


# ---- binary + config -----------------------------------------------------


def test_binary_path(suite, qsfp_args):
    assert suite.binary_path(qsfp_args) == "/opt/fboss/bin/qsfp_hw_test_benchmark"


def test_config_path(suite):
    assert suite.config_path() == qsfp_benchmark_suite.QSFP_BENCH_CONFIG


def test_known_bad_keys_no_run_mode(suite):
    # QSFP keys are "<platform>/<phy_sdk>" with no run-mode variants.
    assert suite.known_bad_keys("fuji_5x16Q/barchetta2-5.2") == [
        "fuji_5x16Q/barchetta2-5.2"
    ]


# ---- build_cmd -----------------------------------------------------------


def test_build_cmd_basics(suite, qsfp_args):
    cmd = suite.build_cmd("/bin/b", qsfp_args, benchmark_name="PhyInitAllColdBoot")
    assert cmd[0] == "/bin/b"
    assert "--json" in cmd
    assert "--bm_regex" in cmd and "^PhyInitAllColdBoot$" in cmd
    assert "--qsfp-config" in cmd
    assert qsfp_args.qsfp_config in cmd
    assert "--logging" in cmd
    # No agent / SAI flags for QSFP.
    assert "--flexports" not in cmd
    assert "--enable_sai_log" not in cmd
    assert "--multi_switch" not in cmd
    assert "--config" not in cmd


def test_build_cmd_force_5pim_fuji(suite, qsfp_args):
    qsfp_args.force_5pim_fuji = True
    assert "--force-5pim-fuji" in suite.build_cmd("/bin/b", qsfp_args)


def test_build_cmd_port_manager_mode(suite, qsfp_args):
    qsfp_args.port_manager_mode = True
    assert "--port-manager-mode" in suite.build_cmd("/bin/b", qsfp_args)


def test_build_cmd_omits_optional_flags_by_default(suite, qsfp_args):
    cmd = suite.build_cmd("/bin/b", qsfp_args)
    assert "--force-5pim-fuji" not in cmd
    assert "--port-manager-mode" not in cmd


def test_build_cmd_fruid(suite, qsfp_args):
    qsfp_args.fruid_path = "/var/facebook/fboss/fruid.json"
    assert "--fruid_filepath=/var/facebook/fboss/fruid.json" in suite.build_cmd(
        "/bin/b", qsfp_args
    )


# ---- find_thresholds (buck-target keyed) ---------------------------------


@pytest.fixture
def thresholds():
    return {
        QSFP_BENCH_TARGET: [
            {
                "test_config_regex": ".*minipack_7x16Q_1x16O/*",
                "thresholds": [
                    {"metric_key_regex": "PhyInitAllColdBoot", "upper_bound": 54}
                ],
            },
            {
                "test_config_regex": ".*fuji_5x16Q/*",
                "thresholds": [
                    {"metric_key_regex": "PhyInitAllColdBoot", "upper_bound": 80}
                ],
            },
            {
                "test_config_regex": ".*fuji_5x16Q/*",
                "thresholds": [{"metric_key_regex": "max_rss", "upper_bound": 750}],
            },
        ]
    }


def test_find_thresholds_matches_platform(suite, qsfp_args, thresholds):
    # For fuji_5x16Q, both the PhyInit and max_rss entries apply (flattened).
    res = suite.find_thresholds(
        "PhyInitAllColdBoot", "fuji_5x16Q/barchetta2-5.2", thresholds, qsfp_args
    )
    keys = sorted(t["metric_key_regex"] for t in res)
    assert keys == ["PhyInitAllColdBoot", "max_rss"]


def test_find_thresholds_other_platform(suite, qsfp_args, thresholds):
    res = suite.find_thresholds(
        "PhyInitAllColdBoot", "minipack_7x16Q_1x16O/millenio-5.5", thresholds, qsfp_args
    )
    assert len(res) == 1
    assert res[0]["upper_bound"] == 54


def test_find_thresholds_no_platform_match(suite, qsfp_args, thresholds):
    assert (
        suite.find_thresholds("PhyInitAllColdBoot", "wedge400/x", thresholds, qsfp_args)
        == []
    )


def test_find_thresholds_no_target_key(suite, qsfp_args):
    assert (
        suite.find_thresholds("PhyInitAllColdBoot", "fuji_5x16Q/x", {}, qsfp_args) == []
    )


def test_find_thresholds_ungated_benchmark_returns_empty(suite, qsfp_args, thresholds):
    # A benchmark whose own timing metric has no threshold must return [] (-> the
    # framework reports NO_THRESHOLD) even though the platform defines shared
    # rusage bounds -- otherwise the ungated benchmark would falsely PASS.
    assert (
        suite.find_thresholds(
            "RefreshTransceiver_LR4_2x400G_10KM",
            "fuji_5x16Q/barchetta2-5.2",
            thresholds,
            qsfp_args,
        )
        == []
    )


# ---- setup / teardown are non-agent no-ops (no services) -----------------


def test_setup_is_noop(suite, qsfp_args):
    # QSFP benchmarks self-init in-process and force their own coldboot, so the
    # suite starts no services and spawns no subprocesses.
    with patch("subprocess.run") as mock_run, patch("subprocess.Popen") as mock_popen:
        suite.setup(qsfp_args)
    mock_run.assert_not_called()
    mock_popen.assert_not_called()


def test_teardown_is_noop(suite, qsfp_args):
    # Base class no-op; should not raise or start/stop anything.
    suite.teardown(qsfp_args)
