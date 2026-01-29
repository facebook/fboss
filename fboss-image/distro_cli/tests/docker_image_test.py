"""Tests for Docker image management utilities."""

import os
import unittest
from pathlib import Path
from unittest.mock import patch

from distro_cli.lib.docker.image import _should_build_image


class TestShouldBuildImage(unittest.TestCase):
    """Tests for _should_build_image function."""

    def setUp(self):
        """Set up test fixtures."""
        self.root_dir = Path("/fake/root")

    @patch("distro_cli.lib.docker.image._compute_dependency_checksum")
    @patch("distro_cli.lib.docker.image._get_image_build_timestamp")
    def test_image_not_found(self, mock_get_timestamp, mock_compute_checksum):
        """Test when image doesn't exist."""
        mock_compute_checksum.return_value = "abc123"
        mock_get_timestamp.return_value = None

        should_build, checksum, reason = _should_build_image(self.root_dir)

        self.assertTrue(should_build)
        self.assertEqual(checksum, "abc123")
        self.assertEqual(reason, "not found")

    @patch("distro_cli.lib.docker.image._compute_dependency_checksum")
    @patch("distro_cli.lib.docker.image._get_image_build_timestamp")
    @patch("time.time")
    def test_image_expired(self, mock_time, mock_get_timestamp, mock_compute_checksum):
        """Test when image exists but is expired."""
        mock_compute_checksum.return_value = "abc123"
        # Image was built at timestamp 1000
        mock_get_timestamp.return_value = 1000
        # Current time is 1000 + 25 hours (more than 24 hour default expiration)
        mock_time.return_value = 1000 + (25 * 60 * 60)

        # Default expiration is 24 hours
        should_build, checksum, reason = _should_build_image(self.root_dir)

        self.assertTrue(should_build)
        self.assertEqual(checksum, "abc123")
        self.assertIn("expired", reason)
        self.assertIn("24", reason)

    @patch("distro_cli.lib.docker.image._compute_dependency_checksum")
    @patch("distro_cli.lib.docker.image._get_image_build_timestamp")
    @patch("time.time")
    def test_image_exists_and_not_expired(
        self, mock_time, mock_get_timestamp, mock_compute_checksum
    ):
        """Test when image exists and is not expired."""
        mock_compute_checksum.return_value = "abc123"
        # Image was built at timestamp 1000
        mock_get_timestamp.return_value = 1000
        # Current time is 1000 + 1 hour (less than 24 hour default expiration)
        mock_time.return_value = 1000 + (1 * 60 * 60)

        should_build, checksum, reason = _should_build_image(self.root_dir)

        self.assertFalse(should_build)
        self.assertEqual(checksum, "abc123")
        self.assertEqual(reason, "exists and is not expired")

    @patch("distro_cli.lib.docker.image._compute_dependency_checksum")
    @patch("distro_cli.lib.docker.image._get_image_build_timestamp")
    @patch("time.time")
    def test_custom_expiration_time(
        self, mock_time, mock_get_timestamp, mock_compute_checksum
    ):
        """Test with custom expiration time from environment variable."""
        mock_compute_checksum.return_value = "abc123"
        # Image was built at timestamp 1000
        mock_get_timestamp.return_value = 1000
        # Current time is 1000 + 50 hours (more than 48 hour custom expiration)
        mock_time.return_value = 1000 + (50 * 60 * 60)

        # Set custom expiration to 48 hours
        with patch.dict(os.environ, {"FBOSS_BUILDER_CACHE_EXPIRATION_HOURS": "48"}):
            should_build, checksum, reason = _should_build_image(self.root_dir)

        self.assertTrue(should_build)
        self.assertEqual(checksum, "abc123")
        self.assertIn("expired", reason)
        self.assertIn("48", reason)


if __name__ == "__main__":
    unittest.main()
