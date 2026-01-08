#!/usr/bin/env python3
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Unit tests for ArtifactStore class."""

import shutil
import tempfile
import unittest
from pathlib import Path

from distro_cli.lib.artifact import ArtifactStore


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


if __name__ == "__main__":
    unittest.main()
