# Copyright Meta Platforms, Inc. and affiliates.
# @noautodeps
"""Tests for LinkTestRunner.

Covers the mono vs multi_switch branching for binary selection and warmboot
file path, plus the service start ordering in _setup_coldboot_test which
must launch FSDB before QSFP before HW Agent."""

from unittest.mock import MagicMock, patch

import pytest
from fboss_test_runner.runners.link_test_runner import LinkTestRunner


@pytest.fixture
def link_runner():
    return LinkTestRunner()


def _make_args(**overrides):
    args = MagicMock()
    args.agent_run_mode = "multi_switch"
    args.num_npus = 1
    args.platform_mapping_override_path = None
    args.bsp_platform_mapping_override_path = None
    args.disable_fsdb = False
    args.fsdb_config = None
    args.qsfp_config = None
    args.config = None
    args.mgmt_if = "eth0"
    args.known_bad_tests_file = None
    for k, v in overrides.items():
        setattr(args, k, v)
    return args


class TestBinaryNameByMode:
    def test_mono_uses_mono_link_test_binary(self, link_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="mono")):
            assert link_runner._get_test_binary_name().endswith(
                "mono_link_test-sai_impl"
            )

    def test_multi_switch_uses_multi_link_test_binary(self, link_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="multi_switch")):
            assert link_runner._get_test_binary_name().endswith(
                "multi_link_test-sai_impl"
            )


class TestWarmbootCheckFileByMode:
    """mono uses _0 suffix, multi_switch uses no suffix.
    Wrong path silently skips warmboot."""

    def test_mono_uses_switch_index_zero(self, link_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="mono")):
            assert link_runner._get_warmboot_check_file().endswith("_0")

    def test_multi_switch_uses_no_switch_index(self, link_runner):
        with patch("run_test.args", new=_make_args(agent_run_mode="multi_switch")):
            assert not link_runner._get_warmboot_check_file().endswith("_0")


class TestSetupColdbootServiceOrder:
    """Production correctness: services must start FSDB → QSFP → HW Agent.
    Reordering breaks downstream subscriptions (QSFP needs FSDB up first).
    HW Agent only starts in multi_switch mode."""

    def test_multi_switch_starts_services_in_order(self, link_runner):
        call_order = []
        with (
            patch(
                "run_test.args",
                new=_make_args(agent_run_mode="multi_switch"),
            ),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_fsdb_service",
                side_effect=lambda **_: call_order.append("fsdb"),
            ),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_qsfp_service",
                side_effect=lambda **_: call_order.append("qsfp"),
            ),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_hw_agent_service",
                side_effect=lambda **_: call_order.append("hw_agent"),
            ),
        ):
            link_runner._setup_coldboot_test()
        assert call_order == ["fsdb", "qsfp", "hw_agent"]

    def test_mono_does_not_start_hw_agent_service(self, link_runner):
        # mono mode keeps the HW switch in-process; no separate HW Agent service.
        call_order = []
        with (
            patch("run_test.args", new=_make_args(agent_run_mode="mono")),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_fsdb_service",
                side_effect=lambda **_: call_order.append("fsdb"),
            ),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_qsfp_service",
                side_effect=lambda **_: call_order.append("qsfp"),
            ),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_hw_agent_service",
            ) as mock_hw_agent,
        ):
            link_runner._setup_coldboot_test()
        assert call_order == ["fsdb", "qsfp"]
        mock_hw_agent.assert_not_called()

    def test_disable_fsdb_skips_fsdb_startup(self, link_runner):
        with (
            patch(
                "run_test.args",
                new=_make_args(agent_run_mode="mono", disable_fsdb=True),
            ),
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_fsdb_service"
            ) as mock_fsdb,
            patch(
                "fboss_test_runner.runners.link_test_runner.setup_and_start_qsfp_service"
            ) as mock_qsfp,
        ):
            link_runner._setup_coldboot_test()
        mock_fsdb.assert_not_called()
        # Catches over-skipping: an early-return in _setup_coldboot_test that
        # accidentally drops qsfp too must fail this test, not pass silently.
        mock_qsfp.assert_called()
