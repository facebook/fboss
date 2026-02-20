#!/usr/bin/env python3
"""Tests for compression functionality in ImageBuilder."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from distro_cli.builder.image_builder import ImageBuilder


class TestCompression(unittest.TestCase):
    """Test compression functionality."""

    def test_compress_artifact_skip_when_disabled(self):
        """Test that compression is skipped when compress_artifacts=False."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)

            # Create a dummy .tar file
            test_tar = tmpdir_path / "test-artifact.tar"
            test_tar.write_text("dummy content for testing")

            # Create ImageBuilder with compression disabled
            mock_manifest = Mock()
            mock_manifest.manifest_dir = tmpdir_path
            builder = ImageBuilder(mock_manifest)
            builder.compress_artifacts = False

            # Try to compress (should be no-op)
            result_path = builder._compress_artifact(test_tar, "test-component")

            # Verify original file is returned unchanged
            self.assertEqual(result_path, test_tar)
            self.assertTrue(test_tar.exists())

            # Verify no .zst file was created
            compressed_path = test_tar.with_suffix(".tar.zst")
            self.assertFalse(compressed_path.exists())

    def test_compress_artifact_when_enabled(self):
        """Test that compression works when compress_artifacts=True."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)

            # Create a dummy .tar file
            test_tar = tmpdir_path / "test-artifact.tar"
            test_tar.write_text("dummy content for testing")

            # Create ImageBuilder with compression enabled
            mock_manifest = Mock()
            mock_manifest.manifest_dir = tmpdir_path
            builder = ImageBuilder(mock_manifest)
            builder.compress_artifacts = True

            # Compress the artifact
            result_path = builder._compress_artifact(test_tar, "test-component")

            # Verify compressed file was created and returned
            self.assertTrue(result_path.exists())
            self.assertTrue(result_path.name.endswith(".tar.zst"))

            # Verify original was removed
            self.assertFalse(test_tar.exists())


if __name__ == "__main__":
    unittest.main()
