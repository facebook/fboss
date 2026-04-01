#!/usr/bin/env python3
"""Unit tests for cached-get-deps.py.

Tests are self-contained: they mock all network and subprocess calls and do
not build any real FBOSS or getdeps targets.
"""

from __future__ import annotations

import importlib.util
import io
import json
import os
import tempfile
import unittest
import urllib.error
import urllib.request
from pathlib import Path
from unittest.mock import MagicMock, patch

# Load cached-get-deps as a module (it's not a package, and has hyphens in the name).
_SCRIPT = str(Path(__file__).resolve().parent / "cached-get-deps.py")
_spec = importlib.util.spec_from_file_location("cached_get_deps", _SCRIPT)
cgd = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(cgd)


# ---------------------------------------------------------------------------
# _EXTRA_CMAKE_DEFINES consistency
# ---------------------------------------------------------------------------


class TestExtraCmakeDefines(unittest.TestCase):
    """The _EXTRA_CMAKE_DEFINES dict must contain the right keys and be
    serialised identically wherever it is used, so that getdeps computes the
    same hash whether called from create_loader() or from the subprocess."""

    def test_build_shared_libs_is_off(self):
        """BUILD_SHARED_LIBS=OFF is required to override glog's manifest which
        sets BUILD_SHARED_LIBS=ON, so all deps produce static archives."""
        self.assertEqual(cgd._EXTRA_CMAKE_DEFINES.get("BUILD_SHARED_LIBS"), "OFF")

    def test_range_v3_tests_disabled(self):
        self.assertEqual(cgd._EXTRA_CMAKE_DEFINES.get("RANGE_V3_TESTS"), "OFF")
        self.assertEqual(cgd._EXTRA_CMAKE_DEFINES.get("RANGE_V3_PERF"), "OFF")

    def test_json_roundtrip_is_stable(self):
        """_EXTRA_CMAKE_DEFINES_JSON must be exactly json.dumps(_EXTRA_CMAKE_DEFINES),
        so create_loader() and the subprocess both set GETDEPS_CMAKE_DEFINES to
        the identical string and compute matching project hashes."""
        expected = json.dumps(cgd._EXTRA_CMAKE_DEFINES)
        self.assertEqual(cgd._EXTRA_CMAKE_DEFINES_JSON, expected)

    def test_build_single_dep_passes_defines_to_subprocess(self):
        """build_single_dep() must pass --extra-cmake-defines=<JSON> to the
        getdeps.py subprocess so its hash matches the one computed by
        create_loader()."""
        mock_opts = MagicMock()
        mock_opts.scratch_dir = "/tmp/scratch"
        mock_manifest = MagicMock()
        mock_loader = MagicMock()
        mock_loader.load_manifest.return_value = mock_manifest
        mock_loader.get_project_install_dir.return_value = (
            "/tmp/scratch/installed/gflags-abc"
        )
        mock_loader.get_project_hash.return_value = "abc123"

        with (
            patch.object(cgd.subprocess, "run") as mock_run,
            patch.object(cgd.os.path, "exists", return_value=False),
        ):
            mock_run.return_value = MagicMock(returncode=0)
            cgd.build_single_dep(mock_opts, mock_loader, "gflags")

        self.assertEqual(mock_run.call_count, 1)
        cmd = mock_run.call_args[0][0]
        defines_arg = f"--extra-cmake-defines={cgd._EXTRA_CMAKE_DEFINES_JSON}"
        self.assertIn(
            defines_arg, cmd, f"subprocess cmd must include {defines_arg!r}; got {cmd}"
        )

    def test_create_loader_passes_defines_to_setup_build_options(self):
        """create_loader() must set extra_cmake_defines=_EXTRA_CMAKE_DEFINES_JSON
        in the Namespace it passes to setup_build_options, so that
        GETDEPS_CMAKE_DEFINES matches the subprocess call."""
        captured_args = []

        def fake_setup(args):
            captured_args.append(args)
            mock_opts = MagicMock()
            mock_opts.get_context_generator.return_value = MagicMock()
            return mock_opts

        with (
            patch.object(cgd, "setup_build_options", side_effect=fake_setup),
            patch.object(cgd, "ManifestLoader", return_value=MagicMock()),
        ):
            cgd.create_loader("/tmp/scratch")

        self.assertEqual(len(captured_args), 1)
        args = captured_args[0]
        self.assertEqual(args.extra_cmake_defines, cgd._EXTRA_CMAKE_DEFINES_JSON)
        # allow_system_packages must be False to match the subprocess default
        self.assertFalse(args.allow_system_packages)
        # shared_libs must be False so getdeps passes -DBUILD_SHARED_LIBS=OFF
        self.assertFalse(args.shared_libs)


# ---------------------------------------------------------------------------
# _maybe_symlink helper
# ---------------------------------------------------------------------------


class TestMaybeSymlink(unittest.TestCase):
    def test_no_dest_is_noop(self):
        with tempfile.TemporaryDirectory() as src:
            cgd._maybe_symlink(src, None)  # should not raise

    def test_same_src_and_dest_is_noop(self):
        with tempfile.TemporaryDirectory() as src:
            cgd._maybe_symlink(src, src)  # should not raise, no symlink created
            self.assertFalse(os.path.islink(src))

    def test_creates_symlink(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = os.path.join(tmpdir, "src")
            os.makedirs(src)
            dest = os.path.join(tmpdir, "subdir", "link")
            cgd._maybe_symlink(src, dest)
            self.assertTrue(os.path.islink(dest))
            self.assertEqual(os.readlink(dest), src)

    def test_replaces_existing_symlink(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src_new = os.path.join(tmpdir, "src_new")
            os.makedirs(src_new)
            dest = os.path.join(tmpdir, "link")
            os.symlink("/old/path", dest)
            cgd._maybe_symlink(src_new, dest)
            self.assertEqual(os.readlink(dest), src_new)


# ---------------------------------------------------------------------------
# cache_key and _cache_object_url
# ---------------------------------------------------------------------------


class TestCacheKey(unittest.TestCase):
    def test_format(self):
        key = cgd.cache_key("folly", "abc123")
        self.assertEqual(key, "fboss-getdeps-folly-abc123")

    def test_includes_project_name(self):
        self.assertIn("gflags", cgd.cache_key("gflags", "xyz"))

    def test_cache_object_url(self):
        url = cgd._cache_object_url(
            "http://cache.example.com", "fboss-getdeps-folly-abc"
        )
        self.assertEqual(
            url, "http://cache.example.com/fboss-getdeps-folly-abc.tar.zst"
        )


# ---------------------------------------------------------------------------
# check_cache
# ---------------------------------------------------------------------------


class TestCheckCache(unittest.TestCase):
    def test_empty_url_returns_false(self):
        self.assertFalse(cgd.check_cache("", "some-key"))

    def test_http_200_returns_true(self):
        mock_resp = MagicMock()
        mock_resp.status = 200
        with patch.object(cgd.urllib.request, "urlopen", return_value=mock_resp):
            self.assertTrue(cgd.check_cache("http://cache.example.com", "some-key"))

    def test_http_404_returns_false(self):
        with patch.object(
            cgd.urllib.request,
            "urlopen",
            side_effect=urllib.error.HTTPError(None, 404, "Not Found", {}, None),
        ):
            self.assertFalse(cgd.check_cache("http://cache.example.com", "some-key"))

    def test_network_error_returns_false(self):
        with patch.object(
            cgd.urllib.request,
            "urlopen",
            side_effect=urllib.error.URLError("connection refused"),
        ):
            self.assertFalse(cgd.check_cache("http://cache.example.com", "some-key"))

    def test_uses_head_method(self):
        """Must use HEAD (not GET) so we don't download the full tarball."""
        requests_made = []

        def fake_urlopen(req, timeout=None):  # noqa: ARG001
            requests_made.append(req)
            mock_resp = MagicMock()
            mock_resp.status = 200
            return mock_resp

        with patch.object(cgd.urllib.request, "urlopen", side_effect=fake_urlopen):
            cgd.check_cache("http://cache.example.com", "some-key")

        self.assertEqual(len(requests_made), 1)
        self.assertEqual(requests_made[0].method, "HEAD")


# ---------------------------------------------------------------------------
# download_from_cache
# ---------------------------------------------------------------------------


class TestDownloadFromCache(unittest.TestCase):
    def test_empty_url_returns_false(self):
        self.assertFalse(cgd.download_from_cache("", "key", "/tmp/dest"))

    def test_download_failure_returns_false(self):
        with patch.object(
            cgd.urllib.request, "urlretrieve", side_effect=urllib.error.URLError("fail")
        ):
            self.assertFalse(
                cgd.download_from_cache("http://cache.example.com", "key", "/tmp/dest")
            )

    def test_extraction_success(self):
        with tempfile.TemporaryDirectory() as dest:
            tarball_path = os.path.join(tempfile.gettempdir(), "key.tar.zst")

            def fake_retrieve(_url, dst):
                # Create a fake tarball file so unlink works
                open(dst, "wb").close()

            mock_result = MagicMock()
            mock_result.returncode = 0

            with (
                patch.object(
                    cgd.urllib.request, "urlretrieve", side_effect=fake_retrieve
                ),
                patch.object(
                    cgd.subprocess, "run", return_value=mock_result
                ) as mock_run,
                patch.object(cgd.os, "unlink") as mock_unlink,
            ):
                result = cgd.download_from_cache(
                    "http://cache.example.com", "key", dest
                )

            self.assertTrue(result)
            # tarball must be deleted after extraction
            mock_unlink.assert_called_once_with(tarball_path)
            # extraction command must use --strip-components=1 and -C dest
            cmd = mock_run.call_args[0][0]
            self.assertIn("--strip-components=1", cmd)
            self.assertIn(dest, cmd)

    def test_tar_failure_returns_false(self):
        with tempfile.TemporaryDirectory() as dest:

            def fake_retrieve(_url, dst):
                open(dst, "wb").close()

            mock_result = MagicMock()
            mock_result.returncode = 1

            with (
                patch.object(
                    cgd.urllib.request, "urlretrieve", side_effect=fake_retrieve
                ),
                patch.object(cgd.subprocess, "run", return_value=mock_result),
                patch.object(cgd.os, "unlink"),
            ):
                result = cgd.download_from_cache(
                    "http://cache.example.com", "key", dest
                )

            self.assertFalse(result)


# ---------------------------------------------------------------------------
# upload_to_cache
# ---------------------------------------------------------------------------


class TestUploadToCache(unittest.TestCase):
    def test_empty_url_returns_false(self):
        self.assertFalse(cgd.upload_to_cache("", "key", "/some/dir"))

    def test_tar_creation_failure_returns_false(self):
        mock_result = MagicMock()
        mock_result.returncode = 1

        with patch.object(cgd.subprocess, "run", return_value=mock_result):
            result = cgd.upload_to_cache("http://cache.example.com", "key", "/some/dir")

        self.assertFalse(result)

    def test_upload_success(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src_dir = os.path.join(tmpdir, "mylib-abc123")
            os.makedirs(src_dir)
            tarball_path = os.path.join(tempfile.gettempdir(), "key.tar.zst")
            open(tarball_path, "wb").close()  # fake tarball

            tar_result = MagicMock(returncode=0)
            mock_resp = MagicMock()

            with (
                patch.object(cgd.subprocess, "run", return_value=tar_result),
                patch.object(cgd.os.path, "getsize", return_value=1024 * 1024),
                patch.object(cgd.urllib.request, "urlopen", return_value=mock_resp),
                patch.object(cgd.os.path, "exists", return_value=True),
                patch.object(cgd.os, "unlink") as mock_unlink,
            ):
                result = cgd.upload_to_cache("http://cache.example.com", "key", src_dir)

            self.assertTrue(result)
            # tarball must be cleaned up in finally block
            mock_unlink.assert_called()

    def test_upload_failure_still_returns_true_and_cleans_up(self):
        """Upload failure is non-fatal (just a warning) but tarball must be cleaned."""
        with tempfile.TemporaryDirectory() as tmpdir:
            src_dir = os.path.join(tmpdir, "mylib-abc123")
            os.makedirs(src_dir)

            tar_result = MagicMock(returncode=0)
            tarball_path = os.path.join(tempfile.gettempdir(), "key.tar.zst")
            open(tarball_path, "wb").close()

            with (
                patch.object(cgd.subprocess, "run", return_value=tar_result),
                patch.object(cgd.os.path, "getsize", return_value=1024),
                patch.object(
                    cgd.urllib.request,
                    "urlopen",
                    side_effect=urllib.error.URLError("network error"),
                ),
                patch.object(cgd.os.path, "exists", return_value=True),
                patch.object(cgd.os, "unlink") as mock_unlink,
            ):
                result = cgd.upload_to_cache("http://cache.example.com", "key", src_dir)

            # upload failure warns but still returns True (tarball was created)
            self.assertTrue(result)
            mock_unlink.assert_called()


# ---------------------------------------------------------------------------
# build_single_dep: already-built detection
# ---------------------------------------------------------------------------


class TestBuildSingleDepAlreadyBuilt(unittest.TestCase):
    def test_skips_build_if_hash_matches(self):
        """If .built-by-getdeps marker exists with matching hash, skip build."""
        with tempfile.TemporaryDirectory() as tmpdir:
            inst_dir = os.path.join(tmpdir, "gflags-abc")
            os.makedirs(inst_dir)
            marker = os.path.join(inst_dir, ".built-by-getdeps")
            with open(marker, "w") as f:
                f.write("abc123")

            mock_opts = MagicMock()
            mock_opts.scratch_dir = tmpdir
            mock_manifest = MagicMock()
            mock_loader = MagicMock()
            mock_loader.load_manifest.return_value = mock_manifest
            mock_loader.get_project_install_dir.return_value = inst_dir
            mock_loader.get_project_hash.return_value = "abc123"

            with patch.object(cgd.subprocess, "run") as mock_run:
                result = cgd.build_single_dep(mock_opts, mock_loader, "gflags")

            # subprocess.run must NOT be called — dep is already built
            mock_run.assert_not_called()
            self.assertEqual(result, inst_dir)

    def test_rebuilds_if_hash_differs(self):
        """If .built-by-getdeps marker has a different hash, must rebuild."""
        with tempfile.TemporaryDirectory() as tmpdir:
            inst_dir = os.path.join(tmpdir, "gflags-new")
            os.makedirs(inst_dir)
            marker = os.path.join(inst_dir, ".built-by-getdeps")
            with open(marker, "w") as f:
                f.write("old-hash")  # stale hash

            mock_opts = MagicMock()
            mock_opts.scratch_dir = tmpdir
            mock_manifest = MagicMock()
            mock_loader = MagicMock()
            mock_loader.load_manifest.return_value = mock_manifest
            mock_loader.get_project_install_dir.return_value = inst_dir
            mock_loader.get_project_hash.return_value = "new-hash"

            mock_run_result = MagicMock(returncode=0)
            with patch.object(
                cgd.subprocess, "run", return_value=mock_run_result
            ) as mock_run:
                cgd.build_single_dep(mock_opts, mock_loader, "gflags")

            mock_run.assert_called_once()


# ---------------------------------------------------------------------------
# run-getdeps.py: print_info and print_error must go to stderr
# ---------------------------------------------------------------------------


class TestRunGetdepsPrintFunctions(unittest.TestCase):
    """Critical regression test: print_info/print_error must write to stderr.

    Before the fix they wrote to stdout, which corrupted --hash-only output
    read by getdeps_dep.bzl.  The fix redirected them to stderr.
    """

    def setUp(self):
        run_getdeps_path = str(Path(__file__).resolve().parent / "run-getdeps.py")
        spec = importlib.util.spec_from_file_location(
            "run_getdeps_test", run_getdeps_path
        )
        self.rg = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.rg)

    def test_print_info_goes_to_stderr(self):
        captured_out = io.StringIO()
        captured_err = io.StringIO()
        with patch("sys.stdout", captured_out), patch("sys.stderr", captured_err):
            self.rg.print_info("hello info")

        self.assertIn("hello info", captured_err.getvalue())
        self.assertEqual(captured_out.getvalue(), "")

    def test_print_error_goes_to_stderr(self):
        captured_out = io.StringIO()
        captured_err = io.StringIO()
        with patch("sys.stdout", captured_out), patch("sys.stderr", captured_err):
            self.rg.print_error("something failed")

        self.assertIn("something failed", captured_err.getvalue())
        self.assertEqual(captured_out.getvalue(), "")

    def test_print_info_has_ansi_color(self):
        """Info messages should be coloured (yellow) so they stand out in logs."""
        captured_err = io.StringIO()
        with patch("sys.stderr", captured_err):
            self.rg.print_info("check colors")
        # ANSI yellow escape code
        self.assertIn("\033[93m", captured_err.getvalue())

    def test_print_error_has_ansi_color(self):
        """Error messages should be coloured (red)."""
        captured_err = io.StringIO()
        with patch("sys.stderr", captured_err):
            self.rg.print_error("check colors")
        # ANSI red escape code
        self.assertIn("\033[91m", captured_err.getvalue())


if __name__ == "__main__":
    unittest.main(verbosity=2)
