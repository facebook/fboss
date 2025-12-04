# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

import subprocess
import unittest
from unittest.mock import MagicMock, patch

from ..conveyor_api import get_release_status


class GetReleaseStatusTest(unittest.TestCase):
    """Tests for get_release_status function."""

    def test_get_release_status_with_valid_response(self) -> None:
        """Test get_release_status successfully parses valid conveyor CLI output."""
        # Setup: Mock subprocess.run to return valid JSON
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = """Warning: some warning
[{
    "runs": {},
    "conveyor_fbid": 123,
    "release_id": 456,
    "release_instance_number": 1,
    "release_number": 6655,
    "release_instance_id": "R6655.1"
}]"""
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify we get a list with release data
            self.assertIsInstance(releases, list)
            self.assertEqual(len(releases), 1)
            self.assertIsInstance(releases[0], dict)
            self.assertEqual(releases[0]["release_instance_id"], "R6655.1")

    def test_get_release_status_constructs_correct_command(self) -> None:
        """Test get_release_status constructs correct conveyor CLI command."""
        # Setup: Mock subprocess.run
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "[]"
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ) as mock_run:
            # Execute: Call get_release_status
            get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify correct command was constructed
            cmd = mock_run.call_args[0][0]
            self.assertEqual(cmd[0], "conveyor")
            self.assertEqual(cmd[1], "release")
            self.assertEqual(cmd[2], "status")
            self.assertIn("--conveyor-id", cmd)
            self.assertIn("fboss/wedge_agent", cmd)
            self.assertIn("--release-instance", cmd)
            self.assertIn("R6655.1", cmd)
            self.assertIn("--json", cmd)

    def test_get_release_status_handles_no_json_output(self) -> None:
        """Test get_release_status returns empty list when no JSON array found."""
        # Setup: Mock subprocess.run to return output without JSON array
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "No JSON here"
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify empty list is returned
            self.assertEqual(releases, [])

    def test_get_release_status_parses_json_after_warnings(self) -> None:
        """Test get_release_status correctly parses JSON that appears after warning messages."""
        # Setup: Mock subprocess.run to return warnings before JSON
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = """Warning: config not found
Warning: using default settings
[{"release_instance_id": "R6655.1", "runs": {}}]"""
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify JSON was parsed correctly despite warnings
            self.assertIsInstance(releases, list)
            self.assertEqual(len(releases), 1)
            self.assertEqual(releases[0]["release_instance_id"], "R6655.1")

    def test_get_release_status_handles_subprocess_error(self) -> None:
        """Test get_release_status handles subprocess failures gracefully."""
        # Setup: Mock subprocess.run to raise CalledProcessError
        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            side_effect=subprocess.CalledProcessError(
                1, ["conveyor"], stderr="Connection failed"
            ),
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify empty list is returned on error
            self.assertEqual(releases, [])

    def test_get_release_status_handles_json_decode_error(self) -> None:
        """Test get_release_status handles invalid JSON gracefully."""
        # Setup: Mock subprocess.run to return invalid JSON
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "[invalid json"
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify empty list is returned on JSON parse error
            self.assertEqual(releases, [])

    def test_get_release_status_handles_empty_json_array(self) -> None:
        """Test get_release_status handles empty JSON array correctly."""
        # Setup: Mock subprocess.run to return empty array
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "[]"
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify empty list is returned
            self.assertEqual(releases, [])

    def test_get_release_status_handles_multiple_releases_in_response(self) -> None:
        """Test get_release_status handles multiple release objects in JSON response."""
        # Setup: Mock subprocess.run to return multiple releases
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = """[
            {"release_instance_id": "R6655.1", "runs": {}},
            {"release_instance_id": "R6655.2", "runs": {}}
        ]"""
        mock_result.stderr = ""

        with patch(
            "fboss.scripts.conveyor_analyzer.conveyor_api.subprocess.run",
            return_value=mock_result,
        ):
            # Execute: Call get_release_status
            releases = get_release_status("fboss/wedge_agent", "R6655.1")

            # Assert: Verify both releases are returned
            self.assertEqual(len(releases), 2)
            self.assertEqual(releases[0]["release_instance_id"], "R6655.1")
            self.assertEqual(releases[1]["release_instance_id"], "R6655.2")
