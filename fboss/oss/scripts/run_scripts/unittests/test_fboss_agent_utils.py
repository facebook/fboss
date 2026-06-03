#!/usr/bin/env python3
# @noautodeps
# Copyright Meta Platforms, Inc. and affiliates.

"""Unit tests for fboss_agent_utils.py sw_agent and cold_boot_agents functions."""

import os
import sys
import unittest
from unittest.mock import MagicMock, mock_open, patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from fboss_agent_utils import (
    cleanup_sw_agent_service,
    cold_boot_agents,
    cold_boot_sw_agent,
    FBOSS_WARMBOOT_DIR,
    is_agent_running,
    setup_and_start_sw_agent_service,
)


def _mock_run(returncode: int = 0) -> MagicMock:
    result = MagicMock()
    result.returncode = returncode
    return result


class TestColdBootAgents(unittest.TestCase):
    @patch("fboss_agent_utils.subprocess.run")
    def test_stops_sw_then_hw_cleans_and_starts(self, mock_run):
        mock_run.return_value = _mock_run(0)

        cold_boot_agents([0])

        calls = [c[0][0] for c in mock_run.call_args_list]
        self.assertIn("systemctl stop fboss_sw_agent", calls[0])
        self.assertIn("systemctl stop fboss_hw_agent@0", calls[1])
        self.assertIn(f"rm -rf {FBOSS_WARMBOOT_DIR}", calls[2])
        self.assertIn(f"mkdir -p {FBOSS_WARMBOOT_DIR}", calls[3])
        self.assertIn(f"touch {FBOSS_WARMBOOT_DIR}/hw_cold_boot_once_0", calls[4])
        self.assertIn(f"touch {FBOSS_WARMBOOT_DIR}/sw_cold_boot_once", calls[5])
        self.assertIn("systemctl start fboss_hw_agent@0", calls[6])
        self.assertIn("systemctl start fboss_sw_agent", calls[7])

    @patch("fboss_agent_utils.subprocess.run")
    def test_multi_npu(self, mock_run):
        mock_run.return_value = _mock_run(0)

        cold_boot_agents([0, 1])

        calls = [c[0][0] for c in mock_run.call_args_list]
        hw_stop_calls = [c for c in calls if "systemctl stop fboss_hw_agent@" in c]
        self.assertEqual(len(hw_stop_calls), 2)
        self.assertIn("fboss_hw_agent@0", hw_stop_calls[0])
        self.assertIn("fboss_hw_agent@1", hw_stop_calls[1])

        hw_flag_calls = [c for c in calls if "hw_cold_boot_once_" in c]
        self.assertEqual(len(hw_flag_calls), 2)

        hw_start_calls = [c for c in calls if "systemctl start fboss_hw_agent@" in c]
        self.assertEqual(len(hw_start_calls), 2)

    @patch("fboss_agent_utils.subprocess.run")
    def test_raises_on_stop_failure(self, mock_run):
        def side_effect(cmd, **_kwargs):
            if "systemctl stop fboss_sw_agent" in cmd:
                return _mock_run(1)
            return _mock_run(0)

        mock_run.side_effect = side_effect

        with self.assertRaisesRegex(Exception, "stopping agents"):
            cold_boot_agents([0])
        calls = [c[0][0] for c in mock_run.call_args_list]
        self.assertFalse(any("rm -rf" in c for c in calls))
        self.assertFalse(any("systemctl start" in c for c in calls))

    def test_empty_switch_indexes_raises(self):
        with self.assertRaises(ValueError):
            cold_boot_agents([])


class TestIsAgentRunning(unittest.TestCase):
    @patch("fboss_agent_utils.subprocess.run")
    def test_all_active(self, mock_run):
        mock_run.return_value = _mock_run(0)

        result = is_agent_running([0])

        self.assertEqual(result, [True, True])
        self.assertEqual(mock_run.call_count, 2)

    @patch("fboss_agent_utils.subprocess.run")
    def test_sw_down(self, mock_run):
        mock_run.return_value = _mock_run(3)

        result = is_agent_running([0])

        self.assertEqual(result[0], False)

    @patch("fboss_agent_utils.subprocess.run")
    def test_hw_down(self, mock_run):
        def side_effect(cmd, **_kwargs):
            if "fboss_sw_agent" in cmd:
                return _mock_run(0)
            return _mock_run(3)

        mock_run.side_effect = side_effect

        result = is_agent_running([0])

        self.assertEqual(result, [True, False])

    @patch("fboss_agent_utils.subprocess.run")
    def test_multi_npu(self, mock_run):
        mock_run.return_value = _mock_run(0)

        result = is_agent_running([0, 1])

        self.assertEqual(result, [True, True, True])
        self.assertEqual(mock_run.call_count, 3)

    @patch("fboss_agent_utils.subprocess.run")
    def test_custom_service_names(self, mock_run):
        mock_run.return_value = _mock_run(0)

        is_agent_running(
            [0],
            hw_agent_service_name="custom_hw@",
            sw_agent_service_name="custom_sw",
        )

        calls = [c[0][0] for c in mock_run.call_args_list]
        self.assertIn("custom_sw", calls[0])
        self.assertIn("custom_hw@0", calls[1])


class TestCleanupSwAgentService(unittest.TestCase):
    @patch("fboss_agent_utils.subprocess.run")
    def test_stops_disables_and_cleans(self, mock_run):
        mock_run.return_value = _mock_run(0)

        cleanup_sw_agent_service()

        calls = [c[0][0] for c in mock_run.call_args_list]
        self.assertTrue(any("systemctl stop fboss_sw_agent" in c for c in calls))
        self.assertTrue(any("systemctl disable fboss_sw_agent" in c for c in calls))
        self.assertTrue(any("systemctl daemon-reload" in c for c in calls))
        self.assertTrue(any("pkill -f fboss_sw_agent" in c for c in calls))


class TestSetupAndStartSwAgentService(unittest.TestCase):
    @patch("fboss_agent_utils.subprocess.run")
    @patch("fboss_agent_utils.os.path.exists", return_value=True)
    @patch("builtins.open", mock_open())
    def test_sets_up_and_starts(self, mock_exists, mock_run):
        mock_run.return_value = _mock_run(0)

        setup_and_start_sw_agent_service(
            fboss_agent_config_path="/etc/coop/agent.conf",
        )

        calls = [c[0][0] for c in mock_run.call_args_list]
        self.assertTrue(any("systemctl enable" in c for c in calls))
        self.assertTrue(any("systemctl start fboss_sw_agent" in c for c in calls))

    @patch("fboss_agent_utils.os.path.exists", return_value=False)
    def test_raises_when_binary_missing(self, mock_exists):
        with self.assertRaisesRegex(Exception, "does not exist"):
            setup_and_start_sw_agent_service(
                fboss_agent_config_path="/etc/coop/agent.conf",
            )


class TestColdBootSwAgent(unittest.TestCase):
    @patch("fboss_agent_utils.subprocess.run")
    def test_stop_clean_start_sequence(self, mock_run):
        mock_run.return_value = _mock_run(0)

        result = cold_boot_sw_agent()

        calls = [c[0][0] for c in mock_run.call_args_list]
        self.assertIn("systemctl stop fboss_sw_agent", calls[0])
        self.assertIn("can_warm_boot", calls[1])
        self.assertIn("systemctl start fboss_sw_agent", calls[2])
        self.assertEqual(result, 0)


if __name__ == "__main__":
    unittest.main()
