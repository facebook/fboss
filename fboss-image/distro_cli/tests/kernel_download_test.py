#!/usr/bin/env python3
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Test kernel component download functionality with caching."""

import hashlib
import http.server
import json
import shutil
import socketserver
import tempfile
import threading
import unittest
from pathlib import Path

from distro_cli.builder.component import ComponentBuilder
from distro_cli.lib.artifact import ArtifactStore
from distro_cli.lib.manifest import ImageManifest


class SimpleHTTPServer:
    """Simple HTTP server for testing downloads."""

    def __init__(self, directory, port=0):
        """Initialize HTTP server.

        Args:
            directory: Directory to serve files from
            port: Port to listen on (default: 0 = let OS assign random port)
        """
        self.directory = directory
        self.port = port
        self.server = None
        self.server_thread = None
        self._ready = threading.Event()

    def start(self):
        """Start the HTTP server in a background thread."""
        # Enable SO_REUSEADDR to avoid "Address already in use" errors
        socketserver.TCPServer.allow_reuse_address = True

        self.server = socketserver.TCPServer(
            ("127.0.0.1", self.port),
            lambda *args, **kwargs: http.server.SimpleHTTPRequestHandler(
                *args, directory=str(self.directory), **kwargs
            ),
        )

        # Get the actual port assigned by the OS (important when port=0)
        self.port = self.server.server_address[1]

        def serve_with_ready_signal():
            """Serve forever and signal when ready."""
            self._ready.set()
            self.server.serve_forever()

        self.server_thread = threading.Thread(
            target=serve_with_ready_signal, daemon=True
        )
        self.server_thread.start()

        # Wait for server to be ready (with timeout)
        if not self._ready.wait(timeout=180):  # 3 minutes max
            raise RuntimeError("HTTP server failed to start within 3 minutes")

    def stop(self):
        """Stop the HTTP server."""
        if self.server:
            self.server.shutdown()
            self.server.server_close()

    def get_url(self, filename):
        """Get URL for a file served by this server.

        Args:
            filename: Name of file to get URL for

        Returns:
            Full HTTP URL to the file
        """
        return f"http://127.0.0.1:{self.port}/{filename}"


class KernelDownloadTestHelper:
    """Helper class with common test logic for kernel download tests.

    This is NOT a TestCase - it's a helper that test classes use via delegation.
    """

    def __init__(
        self,
        test_case,
        manifest_path,
        store_dir,
        source_file=None,
        temp_source_dir=None,
    ):
        """Initialize helper.

        Args:
            test_case: The unittest.TestCase instance (for assertions)
            manifest_path: Path to manifest file
            store_dir: Path to store directory
            source_file: Original source file to copy from (for file:// tests)
            temp_source_dir: Temp directory where source file is staged (for file:// tests)
        """
        self.test_case = test_case
        self.manifest_path = manifest_path
        self.store_dir = store_dir
        self.source_file = source_file
        self.temp_source_dir = temp_source_dir

    def build_kernel(self):
        """Build kernel with caching.

        For file:// tests, ensures source file exists by copying from original if needed.

        Returns:
            Path to built/cached artifact
        """
        # For file:// tests, ensure source file exists
        # (it may have been moved to artifact store on previous build)
        if self.source_file and self.temp_source_dir:
            temp_source_file = self.temp_source_dir / self.source_file.name
            if not temp_source_file.exists():
                shutil.copy2(self.source_file, temp_source_file)

        manifest = ImageManifest(self.manifest_path)
        # Override class attribute for testing
        ArtifactStore.ARTIFACT_STORE_DIR = self.store_dir
        store = ArtifactStore()

        # Get kernel component data
        kernel_data = manifest.get_component("kernel")

        kernel_builder = ComponentBuilder(
            component_name="kernel",
            component_data=kernel_data,
            manifest_dir=manifest.manifest_dir,
            store=store,
            artifact_pattern="kernel-*.rpms.tar",
        )
        return kernel_builder.build()

    def get_store_key(self):
        """Get store key for kernel component in manifest.

        Returns:
            Store key string
        """
        manifest = ImageManifest(self.manifest_path)
        component_data = manifest.get_component("kernel")
        url = component_data["download"]
        return f"kernel-download-{hashlib.sha256(url.encode()).hexdigest()}"

    def test_download_and_cache(self):
        """Test that kernel download works and caching is effective."""
        # First build - should download and cache
        artifact_path_1 = self.build_kernel()

        # Verify artifact exists and is in cache
        self.test_case.assertTrue(
            artifact_path_1.exists(), f"Artifact not found: {artifact_path_1}"
        )
        self.test_case.assertTrue(
            str(artifact_path_1).startswith(str(self.store_dir)),
            f"Artifact not in store dir: {artifact_path_1}",
        )

        # Second build - should hit cache (no download)
        artifact_path_2 = self.build_kernel()

        # Verify we got the same cached artifact
        self.test_case.assertEqual(
            artifact_path_1,
            artifact_path_2,
            "Second build should return same cached artifact",
        )
        self.test_case.assertTrue(
            artifact_path_2.exists(), f"Cached artifact not found: {artifact_path_2}"
        )

    def test_cache_invalidation(self):
        """Test that cache invalidation works."""
        # Build and cache artifact
        artifact_path = self.build_kernel()
        self.test_case.assertTrue(artifact_path.exists())

        # Get store and invalidate
        ArtifactStore.ARTIFACT_STORE_DIR = self.store_dir
        store = ArtifactStore()
        store_key = self.get_store_key()
        store.invalidate(store_key)

        # Verify artifact is gone
        self.test_case.assertFalse(
            artifact_path.exists(), "Artifact should be removed after invalidation"
        )

    def test_cache_clear(self):
        """Test that clearing cache removes all artifacts."""
        # Build and cache artifact
        artifact_path = self.build_kernel()
        self.test_case.assertTrue(artifact_path.exists())

        # Clear the store
        ArtifactStore.ARTIFACT_STORE_DIR = self.store_dir
        store = ArtifactStore()
        store.clear()

        # Verify store directory is empty
        store_contents = list(self.store_dir.glob("*"))
        self.test_case.assertEqual(
            len(store_contents),
            0,
            f"Store should be empty after clear, but contains: {store_contents}",
        )


class TestKernelDownloadFile(unittest.TestCase):
    """Test kernel component download from file:// URL."""

    def setUp(self):
        """Set up test fixtures."""
        self.test_dir = Path(__file__).parent
        self.store_dir = Path(tempfile.mkdtemp(prefix="test-store-"))

        # Create temp directory for source files (on same filesystem as artifact store)
        # This ensures the file can be moved efficiently during download
        self.temp_source_dir = Path(tempfile.mkdtemp(prefix="test-source-"))

        # Copy test data file to temp source directory
        # This way the original test data file is preserved when download moves it
        # Downloads should use compressed files to save bandwidth
        self.source_file = self.test_dir / "data" / "kernel-test.tar.zst"
        temp_source_file = self.temp_source_dir / "kernel-test.tar.zst"
        shutil.copy2(self.source_file, temp_source_file)

        # Create test manifest that points to temp source file
        self.manifest_path = self.temp_source_dir / "test-manifest.json"
        manifest_data = {
            "kernel": {"download": f"file:{temp_source_file}"},
            "distribution_formats": {"usb": "output/test-download.iso"},
        }
        with open(self.manifest_path, "w") as f:
            json.dump(manifest_data, f, indent=2)

        # Create helper with delegation, passing source file info
        self.helper = KernelDownloadTestHelper(
            self,
            self.manifest_path,
            self.store_dir,
            source_file=self.source_file,
            temp_source_dir=self.temp_source_dir,
        )

    def tearDown(self):
        """Clean up test fixtures."""
        if self.store_dir.exists():
            shutil.rmtree(self.store_dir)
        if self.temp_source_dir.exists():
            shutil.rmtree(self.temp_source_dir)

    def test_download_and_cache(self):
        """Test that kernel download works and caching is effective."""
        self.helper.test_download_and_cache()

    def test_cache_invalidation(self):
        """Test that cache invalidation works."""
        self.helper.test_cache_invalidation()

    def test_cache_clear(self):
        """Test that clearing cache removes all artifacts."""
        self.helper.test_cache_clear()


class TestKernelDownloadHTTP(unittest.TestCase):
    """Test kernel component download from HTTP server."""

    @classmethod
    def setUpClass(cls):
        """Start HTTP server for all tests."""
        cls.test_dir = Path(__file__).parent
        cls.data_dir = cls.test_dir / "data"

        # Start HTTP server using helper (port=0 lets OS assign random available port)
        cls.http_server = SimpleHTTPServer(cls.data_dir)
        cls.http_server.start()

    @classmethod
    def tearDownClass(cls):
        """Stop HTTP server."""
        cls.http_server.stop()

    def setUp(self):
        """Set up test fixtures."""
        self.temp_dir = Path(tempfile.mkdtemp(prefix="test-http-"))
        self.store_dir = Path(tempfile.mkdtemp(prefix="test-store-"))

        # Create test manifest with HTTP URL
        self.manifest_path = self.temp_dir / "test-http-manifest.json"
        manifest_data = {
            "name": "test-http-download",
            "version": "1.0.0",
            "kernel": {"download": self.http_server.get_url("kernel-test.tar.zst")},
            "distribution_formats": {"usb": "output/test.iso"},
        }

        with open(self.manifest_path, "w") as f:
            json.dump(manifest_data, f, indent=2)

        # Create helper with delegation (HTTP tests don't need source file restoration)
        self.helper = KernelDownloadTestHelper(self, self.manifest_path, self.store_dir)

    def tearDown(self):
        """Clean up test fixtures."""
        if self.temp_dir.exists():
            shutil.rmtree(self.temp_dir)
        if self.store_dir.exists():
            shutil.rmtree(self.store_dir)

    def test_download_and_cache(self):
        """Test that kernel download works and caching is effective."""
        self.helper.test_download_and_cache()

    def test_cache_invalidation(self):
        """Test that cache invalidation works."""
        self.helper.test_cache_invalidation()

    def test_cache_clear(self):
        """Test that clearing cache removes all artifacts."""
        self.helper.test_cache_clear()


if __name__ == "__main__":
    unittest.main()
