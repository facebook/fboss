#!/usr/bin/env python3
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Unit tests for ArtifactStore class."""

import os
import shutil
import tempfile
import time
import unittest
from pathlib import Path

from distro_cli.lib.artifact import ArtifactStore, find_artifact_in_dir
from distro_cli.lib.exceptions import ArtifactError


class TestArtifactStore(unittest.TestCase):
    """Test ArtifactStore data/metadata separation."""

    def setUp(self):
        """Set up test fixtures."""
        self.temp_dir = Path(tempfile.mkdtemp())
        # Override class attribute for testing
        self.original_store_dir = ArtifactStore.ARTIFACT_STORE_DIR
        ArtifactStore.ARTIFACT_STORE_DIR = self.temp_dir / "store"
        self.store = ArtifactStore()

    def tearDown(self):
        """Clean up test directory."""
        # Restore original ARTIFACT_STORE_DIR
        ArtifactStore.ARTIFACT_STORE_DIR = self.original_store_dir

        if self.temp_dir.exists():
            shutil.rmtree(self.temp_dir)

    def test_store_data_only(self):
        """Test storing data files without metadata."""
        data_file = Path(self.temp_dir) / "data.txt"
        data_file.write_text("test data")

        stored_data, stored_meta = self.store.store("test-key", [data_file], [])

        self.assertEqual(len(stored_data), 1)
        self.assertEqual(len(stored_meta), 0)
        self.assertTrue(stored_data[0].exists())
        self.assertTrue(stored_data[0].read_text() == "test data")

    def test_store_data_and_metadata(self):
        """Test storing both data and metadata files."""
        data_file = Path(self.temp_dir) / "data.txt"
        data_file.write_text("test data")
        meta_file = Path(self.temp_dir) / "meta.json"
        meta_file.write_text('{"etag": "abc123"}')

        stored_data, stored_meta = self.store.store(
            "test-key", [data_file], [meta_file]
        )

        self.assertEqual(len(stored_data), 1)
        self.assertEqual(len(stored_meta), 1)
        self.assertTrue(stored_data[0].exists())
        self.assertTrue(stored_meta[0].exists())
        self.assertTrue(stored_meta[0].read_text() == '{"etag": "abc123"}')

    def test_get_store_miss(self):
        """Test get() with store miss."""
        data_file = Path(self.temp_dir) / "data.txt"
        data_file.write_text("fetched data")

        def fetch_fn(stored_data, stored_meta):
            self.assertEqual(len(stored_data), 0)
            self.assertEqual(len(stored_meta), 0)
            return (False, [data_file], [])

        result_data, result_meta = self.store.get("new-key", fetch_fn)

        self.assertEqual(len(result_data), 1)
        self.assertEqual(len(result_meta), 0)
        self.assertTrue(result_data[0].read_text() == "fetched data")

    def test_get_store_hit(self):
        """Test get() with store hit."""
        data_file = Path(self.temp_dir) / "data.txt"
        data_file.write_text("original data")
        self.store.store("hit-key", [data_file], [])

        def fetch_fn(stored_data, stored_meta):
            self.assertEqual(len(stored_data), 1)
            return (True, stored_data, stored_meta)

        result_data, _ = self.store.get("hit-key", fetch_fn)

        self.assertEqual(len(result_data), 1)
        self.assertTrue(result_data[0].read_text() == "original data")

    def test_data_metadata_separation(self):
        """Test that data and metadata are stored in separate subdirectories."""
        data_file = self.temp_dir / "data.txt"
        data_file.write_text("data")
        meta_file = self.temp_dir / "meta.json"
        meta_file.write_text("metadata")

        stored_data, stored_meta = self.store.store("sep-key", [data_file], [meta_file])

        self.assertTrue("data" in str(stored_data[0]))
        self.assertTrue("metadata" in str(stored_meta[0]))
        self.assertNotEqual(stored_data[0].parent, stored_meta[0].parent)

    def test_multiple_data_files(self):
        """Test storing multiple data files."""
        data1 = self.temp_dir / "file1.txt"
        data1.write_text("data1")
        data2 = self.temp_dir / "file2.txt"
        data2.write_text("data2")

        stored_data, stored_meta = self.store.store("multi-key", [data1, data2], [])

        self.assertEqual(len(stored_data), 2)
        self.assertEqual(len(stored_meta), 0)
        self.assertTrue(all(f.exists() for f in stored_data))

    def test_fetch_fn_receives_stored_files(self):
        """Test that fetch function receives stored files on subsequent calls."""
        data_file = self.temp_dir / "data.txt"
        data_file.write_text("stored")
        meta_file = self.temp_dir / "meta.json"
        meta_file.write_text('{"stored": true}')

        self.store.store("fetch-key", [data_file], [meta_file])

        received_data = []
        received_meta = []

        def fetch_fn(stored_data, stored_meta):
            received_data.extend(stored_data)
            received_meta.extend(stored_meta)
            return (True, stored_data, stored_meta)

        self.store.get("fetch-key", fetch_fn)

        self.assertEqual(len(received_data), 1)
        self.assertEqual(len(received_meta), 1)
        self.assertTrue(received_data[0].exists())
        self.assertTrue(received_meta[0].exists())

    def test_fetch_fn_store_miss_updates_store(self):
        """Test that fetch function on store miss updates the store."""
        new_data = self.temp_dir / "new.txt"
        new_data.write_text("new content")

        def fetch_fn(_stored_data, _stored_meta):
            return (False, [new_data], [])

        _, _ = self.store.get("update-key", fetch_fn)

        # Verify store was updated
        def verify_fn(stored_data, stored_meta):
            self.assertEqual(len(stored_data), 1)
            return (True, stored_data, stored_meta)

        verify_data, _ = self.store.get("update-key", verify_fn)
        self.assertEqual(len(verify_data), 1)

    def test_different_keys_for_compressed_and_uncompressed(self):
        """Test that compressed and uncompressed artifacts use different store keys."""
        # Store uncompressed artifact
        uncompressed_data = self.temp_dir / "artifact.tar"
        uncompressed_data.write_text("uncompressed content")
        self.store.store("component-build-abc123-uncompressed", [uncompressed_data], [])

        # Store compressed artifact with different key
        compressed_data = self.temp_dir / "artifact.tar.zst"
        compressed_data.write_text("compressed content")
        self.store.store("component-build-abc123-compressed", [compressed_data], [])

        # Verify both are stored separately
        def fetch_uncompressed(stored_data, stored_meta):
            self.assertEqual(len(stored_data), 1)
            self.assertIn("artifact.tar", stored_data[0].name)
            return (True, stored_data, stored_meta)

        def fetch_compressed(stored_data, stored_meta):
            self.assertEqual(len(stored_data), 1)
            self.assertIn("artifact.tar.zst", stored_data[0].name)
            return (True, stored_data, stored_meta)

        uncompressed_result, _ = self.store.get(
            "component-build-abc123-uncompressed", fetch_uncompressed
        )
        compressed_result, _ = self.store.get(
            "component-build-abc123-compressed", fetch_compressed
        )

        # Verify they are different files
        self.assertNotEqual(uncompressed_result[0], compressed_result[0])
        self.assertIn("artifact.tar", uncompressed_result[0].name)
        self.assertIn("artifact.tar.zst", compressed_result[0].name)

    def test_artifact_key_salt_creates_different_cache_entries(self):
        """Test that different artifact_key_salt values create separate cache entries."""
        # Simulate building the same component with different salts
        artifact1 = self.temp_dir / "build1.tar"
        artifact1.write_text("build with salt=uncompressed")

        artifact2 = self.temp_dir / "build2.tar"
        artifact2.write_text("build with salt=compressed")

        # Store with different salts (simulating what ComponentBuilder does)
        key_base = "kernel-build-12345678"
        self.store.store(f"{key_base}-salt=uncompressed", [artifact1], [])
        self.store.store(f"{key_base}-salt=compressed", [artifact2], [])

        # Retrieve and verify they're different
        def fetch_fn1(stored_data, stored_meta):
            return (True, stored_data, stored_meta)

        def fetch_fn2(stored_data, stored_meta):
            return (True, stored_data, stored_meta)

        result1, _ = self.store.get(f"{key_base}-salt=uncompressed", fetch_fn1)
        result2, _ = self.store.get(f"{key_base}-salt=compressed", fetch_fn2)

        # Verify different content
        self.assertEqual(result1[0].read_text(), "build with salt=uncompressed")
        self.assertEqual(result2[0].read_text(), "build with salt=compressed")


class TestFindArtifactInDir(unittest.TestCase):
    """Test find_artifact_in_dir function with compression variant support."""

    def setUp(self):
        """Set up test fixtures."""
        self.temp_dir = Path(tempfile.mkdtemp())

    def tearDown(self):
        """Clean up test directory."""
        if self.temp_dir.exists():
            shutil.rmtree(self.temp_dir)

    def test_finds_uncompressed_tar(self):
        """Test finding uncompressed .tar file."""
        artifact = self.temp_dir / "kernel-6.11.1.rpms.tar"
        artifact.write_text("uncompressed tar")

        result = find_artifact_in_dir(self.temp_dir, "kernel-*.rpms.tar", "kernel")

        self.assertEqual(result, artifact)

    def test_finds_zstd_compressed(self):
        """Test finding zstd-compressed .tar.zst file."""
        artifact = self.temp_dir / "sai-1.0.tar.zst"
        artifact.write_text("zstd compressed")

        result = find_artifact_in_dir(self.temp_dir, "sai-*.tar", "sai")

        self.assertEqual(result, artifact)

    def test_uses_most_recent_when_both_exist(self):
        """Test that most recent file is used when both .tar and .tar.zst exist."""
        # Test 1: Uncompressed is newer
        zstd = self.temp_dir / "kernel-6.11.1.rpms.tar.zst"
        zstd.write_text("zstd")
        old_time = time.time() - 10  # 10 seconds ago
        os.utime(zstd, (old_time, old_time))

        uncompressed = self.temp_dir / "kernel-6.11.1.rpms.tar"
        uncompressed.write_text("uncompressed")

        result = find_artifact_in_dir(self.temp_dir, "kernel-*.rpms.tar", "kernel")
        self.assertEqual(result, uncompressed, "Should use newer uncompressed file")

        # Test 2: Compressed is newer
        new_time = time.time()
        os.utime(zstd, (new_time, new_time))

        result = find_artifact_in_dir(self.temp_dir, "kernel-*.rpms.tar", "kernel")
        self.assertEqual(result, zstd, "Should use newer compressed file")

    def test_falls_back_to_zstd_if_no_uncompressed(self):
        """Test falling back to .tar.zst when .tar doesn't exist."""
        zstd = self.temp_dir / "sai-1.0.tar.zst"
        zstd.write_text("zstd only")

        result = find_artifact_in_dir(self.temp_dir, "sai-*.tar", "sai")

        self.assertEqual(result, zstd)

    def test_raises_error_when_no_artifacts_found(self):
        """Test that ArtifactError is raised when no artifacts found."""
        with self.assertRaises(ArtifactError) as cm:
            find_artifact_in_dir(self.temp_dir, "nonexistent-*.tar.zst", "nonexistent")

        error_msg = str(cm.exception)
        self.assertIn("nonexistent", error_msg)
        self.assertIn("nonexistent-*.tar", error_msg)
        self.assertIn("nonexistent-*.tar.zst", error_msg)

    def test_handles_multiple_matching_files(self):
        """Test handling of multiple files matching the same pattern."""
        # Create multiple matching files
        artifact1 = self.temp_dir / "kernel-6.11.1.rpms.tar"
        artifact1.write_text("first")
        artifact2 = self.temp_dir / "kernel-6.12.0.rpms.tar"
        artifact2.write_text("second")

        # Should return one of them (first in glob order)
        result = find_artifact_in_dir(self.temp_dir, "kernel-*.rpms.tar", "kernel")

        self.assertIn(result, [artifact1, artifact2])

    def test_zst_pattern_input(self):
        """Test when input pattern already ends with .tar.zst."""
        artifact = self.temp_dir / "component-1.0.tar.zst"
        artifact.write_text("zstd")

        result = find_artifact_in_dir(self.temp_dir, "component-*.tar.zst", "component")

        self.assertEqual(result, artifact)

    def test_pattern_without_extension(self):
        """Test pattern that doesn't end with .tar extension."""
        # Create a file matching the pattern
        artifact = self.temp_dir / "custom-pattern.tar"
        artifact.write_text("custom")

        result = find_artifact_in_dir(self.temp_dir, "custom-pattern.tar", "custom")

        self.assertEqual(result, artifact)


if __name__ == "__main__":
    unittest.main()
