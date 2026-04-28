#!/usr/bin/env python3
"""Build or fetch a single third-party dependency with caching.

This script wraps Meta's getdeps build system to build one dependency at a
time, with a per-dependency caching layer backed by an HTTP cache server
(e.g., bazel-remote).

Usage:
    # Build gflags, using cache if available:
    cached-get-deps.py --project gflags

    # Build folly, pointing to pre-built dep install dirs:
    cached-get-deps.py --project folly --dep-install-dirs /path/to/gflags:/path/to/glog

    # Just compute and print the hash (for debugging):
    cached-get-deps.py --project gflags --hash-only
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

# Import getdeps modules without modifying them.
FBCODE_BUILDER_DIR = str(
    Path(__file__).resolve().parents[3] / "build" / "fbcode_builder"
)
sys.path.insert(0, FBCODE_BUILDER_DIR)

from getdeps.buildopts import setup_build_options  # noqa: E402
from getdeps.load import ManifestLoader  # noqa: E402

# Import detect_toolchain and setup_clang_environment from the sibling
# run-getdeps.py so we set up the same clang flags/rpath/LD_LIBRARY_PATH
# that a normal getdeps build uses.
_run_getdeps_path = str(Path(__file__).resolve().parent / "run-getdeps.py")
_spec = importlib.util.spec_from_file_location("run_getdeps", _run_getdeps_path)
_run_getdeps_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_run_getdeps_mod)
detect_toolchain = _run_getdeps_mod.detect_toolchain
setup_clang_environment = _run_getdeps_mod.setup_clang_environment

# Global CMake defines passed to every getdeps build.  Using a single
# consistent value here AND in create_loader() (via extra_cmake_defines=)
# ensures that GETDEPS_CMAKE_DEFINES is identical when we compute a dep's
# cache key and when getdeps computes the same dep's expected install path
# inside a downstream build (e.g. folly looking up boost's install dir).
# Any difference would produce a hash mismatch and a "dep not found" error.
# BUILD_SHARED_LIBS is intentionally omitted: getdeps already passes
# -DBUILD_SHARED_LIBS=OFF for all deps via shared_libs=False.
_EXTRA_CMAKE_DEFINES: dict[str, str] = {
    "RANGE_V3_TESTS": "OFF",
    "RANGE_V3_PERF": "OFF",
    # glog's manifest sets BUILD_SHARED_LIBS=ON; override it so all deps
    # produce static archives that Bazel can consume without LD_LIBRARY_PATH.
    "BUILD_SHARED_LIBS": "OFF",
    # Match nhfboss-get-deps.sh (MinSizeRel = -Os -DNDEBUG, no debug info).
    # Including this in GETDEPS_CMAKE_DEFINES (not just --build-type) ensures
    # it is part of the hash, so stale installs built with a different build
    # type are not silently reused.
    "CMAKE_BUILD_TYPE": "MinSizeRel",
    # Prevent double-wrapping: in the Bazel build, CXX=sccache-clang++ (a
    # sccache wrapper). If a stale CMakeCache.txt has CMAKE_*_COMPILER_LAUNCHER
    # =sccache, cmake calls "sccache sccache-clang++ [args]" and sccache fails
    # with "Compiler not supported" because sccache-clang++ is a bash script.
    "CMAKE_CXX_COMPILER_LAUNCHER": "",
    "CMAKE_C_COMPILER_LAUNCHER": "",
}
_EXTRA_CMAKE_DEFINES_JSON: str = json.dumps(_EXTRA_CMAKE_DEFINES)


def create_loader(
    scratch_path: str,
) -> tuple[object, ManifestLoader]:
    """Set up BuildOptions and ManifestLoader, reusing getdeps internals."""
    args = argparse.Namespace(
        scratch_path=scratch_path,
        install_prefix=None,
        host_type=None,
        facebook_internal=False,
        vcvars_path=None,
        allow_system_packages=False,
        lfs_path=None,
        shared_libs=False,
        # Must match --extra-cmake-defines passed to the getdeps subprocess so
        # that GETDEPS_CMAKE_DEFINES is identical in both hash computations.
        extra_cmake_defines=_EXTRA_CMAKE_DEFINES_JSON,
    )
    opts = setup_build_options(args)
    ctx_gen = opts.get_context_generator()
    loader = ManifestLoader(opts, ctx_gen)
    return opts, loader


def _maybe_symlink(src_dir: str, dest: str | None) -> None:
    """If dest is set and differs from src_dir, create/replace a symlink."""
    if not dest or dest == src_dir:
        return
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if os.path.lexists(dest):
        os.remove(dest)
    os.symlink(src_dir, dest)


def cache_key(project_name: str, project_hash: str) -> str:
    """Compute a cache key for a dependency tarball."""
    return f"fboss-getdeps-{project_name}-{project_hash}"


def _cache_object_url(cache_url: str, key: str) -> str:
    """Compute the URL for a cached tarball."""
    return f"{cache_url}/{key}.tar.zst"


def check_cache(cache_url: str, key: str) -> bool:
    """Check if a cached tarball exists in the remote cache."""
    if not cache_url:
        return False
    url = _cache_object_url(cache_url, key)
    try:
        req = urllib.request.Request(url, method="HEAD")
        resp = urllib.request.urlopen(req, timeout=10)
        return resp.status == 200
    except (urllib.error.URLError, urllib.error.HTTPError):
        return False


def download_from_cache(cache_url: str, key: str, dest_dir: str) -> bool:
    """Download and extract a cached tarball."""
    if not cache_url:
        return False
    url = _cache_object_url(cache_url, key)
    tarball = os.path.join(tempfile.gettempdir(), f"{key}.tar.zst")
    try:
        urllib.request.urlretrieve(url, tarball)
    except (urllib.error.URLError, urllib.error.HTTPError):
        return False

    # Extract into dest_dir
    os.makedirs(dest_dir, exist_ok=True)
    result = subprocess.run(
        ["tar", "--zstd", "-xf", tarball, "-C", dest_dir, "--strip-components=1"],
        check=False,
        capture_output=True,
    )
    os.unlink(tarball)
    return result.returncode == 0


def upload_to_cache(cache_url: str, key: str, src_dir: str) -> bool:
    """Create a tarball and upload to the remote cache."""
    if not cache_url:
        return False
    tarball = os.path.join(tempfile.gettempdir(), f"{key}.tar.zst")

    # Create tarball
    result = subprocess.run(
        [
            "tar",
            "--zstd",
            "-cf",
            tarball,
            "-C",
            os.path.dirname(src_dir),
            os.path.basename(src_dir),
        ],
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        return False

    tarball_size = os.path.getsize(tarball)
    url = _cache_object_url(cache_url, key)
    try:
        with open(tarball, "rb") as f:
            req = urllib.request.Request(url, data=f, method="PUT")
            req.add_header("Content-Type", "application/octet-stream")
            req.add_header("Content-Length", str(tarball_size))
            urllib.request.urlopen(req, timeout=600)
        print(f"  Uploaded to cache ({tarball_size / 1048576:.1f} MB)")
    except (urllib.error.URLError, urllib.error.HTTPError) as e:
        print(f"  Warning: cache upload failed: {e}")
    finally:
        if os.path.exists(tarball):
            os.unlink(tarball)
    return True


def build_single_dep(
    opts,
    loader: ManifestLoader,
    project_name: str,
) -> str:
    """Build a single dependency using getdeps, assuming deps are already installed.

    Returns the install directory path.
    """
    manifest = loader.load_manifest(project_name)
    inst_dir = loader.get_project_install_dir(manifest)
    project_hash = loader.get_project_hash(manifest)

    # Check if already built with matching hash
    built_marker = os.path.join(inst_dir, ".built-by-getdeps")
    if os.path.exists(built_marker):
        with open(built_marker) as f:
            existing_hash = f.read().strip()
        if existing_hash == project_hash:
            print("  Already built (hash matches)")
            return inst_dir

    # Use getdeps.py build --no-deps to build just this project.
    # This is the simplest approach that reuses all of getdeps' build logic
    # (fetching, patching, cmake configuration, etc.) without modification.
    repo_root = str(Path(FBCODE_BUILDER_DIR).parent.parent)
    cmd = [
        sys.executable,
        os.path.join(FBCODE_BUILDER_DIR, "getdeps.py"),
        "build",
        "--no-deps",
        "--scratch-path",
        opts.scratch_dir,
        f"--extra-cmake-defines={_EXTRA_CMAKE_DEFINES_JSON}",
        project_name,
    ]
    print(f"  Building {project_name}...")
    result = subprocess.run(cmd, check=False, cwd=repo_root)
    if result.returncode != 0:
        print(
            f"  ERROR: build failed with exit code {result.returncode}", file=sys.stderr
        )
        sys.exit(1)

    return inst_dir


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build or fetch a cached third-party dependency"
    )
    parser.add_argument(
        "--project", required=True, help="Dependency name (e.g., gflags, folly)"
    )
    parser.add_argument(
        "--scratch-path", default="/var/FBOSS/tmp_bld_dir", help="Scratch directory"
    )
    parser.add_argument(
        "--cache-url",
        default="",
        help="HTTP cache server URL for dep tarballs (empty = no caching)",
    )
    parser.add_argument(
        "--install-dir", help="Override install directory (for Bazel repo rule)"
    )
    parser.add_argument(
        "--hash-only", action="store_true", help="Just print the project hash and exit"
    )
    args = parser.parse_args()

    repo_root = str(Path(FBCODE_BUILDER_DIR).parent.parent)

    # Set up clang environment (LD_LIBRARY_PATH, LDFLAGS, CXXFLAGS) before
    # any getdeps operations so that builds can find libunwind.so.1 etc.
    # Pass repo_root explicitly because Bazel repo rules set cwd to the
    # execution root, not the repo root where CMakeLists.txt lives.
    toolchain_info = detect_toolchain()
    if toolchain_info and toolchain_info.get("type") == "clang":
        setup_clang_environment(toolchain_info, repo_root=repo_root)

    opts, loader = create_loader(args.scratch_path)
    manifest = loader.load_manifest(args.project)
    project_hash = loader.get_project_hash(manifest)
    inst_dir = loader.get_project_install_dir(manifest)

    if args.hash_only:
        print(project_hash)
        return 0

    key = cache_key(args.project, project_hash)
    print(f"[{args.project}] hash={project_hash[:30]}... cache_key={key[:50]}...")

    # Try downloading from cache
    if args.cache_url and check_cache(args.cache_url, key):
        print("  Cache HIT — downloading...")
        if download_from_cache(args.cache_url, key, inst_dir):
            # Write marker so getdeps recognizes it
            with open(os.path.join(inst_dir, ".built-by-getdeps"), "w") as f:
                f.write(project_hash)
            print(f"  Installed to {inst_dir}")
            _maybe_symlink(inst_dir, args.install_dir)
            return 0

    # Cache miss — build
    print("  Cache MISS — building...")
    result_dir = build_single_dep(opts, loader, args.project)

    # Upload to cache
    if args.cache_url and os.path.isdir(result_dir):
        upload_to_cache(args.cache_url, key, result_dir)

    _maybe_symlink(result_dir, args.install_dir)
    print(f"  Installed to {result_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
