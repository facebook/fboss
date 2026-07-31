#!/usr/bin/env python3
# Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
# All Rights Reserved

"""
Wrapper script for getdeps.py that sets up clang-specific environment variables.

This script auto-detects if we're using clang and sets appropriate environment
variables before calling the real getdeps.py. This allows us to configure the
build environment without modifying the upstream getdeps.py script.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import sysconfig
import tempfile
from pathlib import Path


def print_info(msg):
    print(f"\033[93m{msg}\033[0m")


def print_error(msg):
    print(f"\033[91m{msg}\033[0m")


# SDK Related Flags
ARG_NPU_SAI_IMPL = "--npu-sai-impl"
ARG_NPU_SAI_VERSION = "--npu-sai-version"
ARG_NPU_SAI_SDK_VERSION = "--npu-sai-sdk-version"
ARG_NPU_LIBSAI_IMPL_PATH = "--npu-libsai-impl-path"
ARG_NPU_LIBSAI_IMPL_TARBALL = "--npu-libsai-impl-tarball"
ARG_NPU_EXPERIMENTS_PATH = "--npu-experiments-path"
ARG_PHY_SAI_IMPL = "--phy-sai-impl"
ARG_PHY_SAI_VERSION = "--phy-sai-version"
ARG_PHY_PAI_SDK_PATH = "--phy-pai-sdk-path"
ARG_PHY_PAI_SDK_TARBALL = "--phy-pai-sdk-tarball"

# Misc Build Flags
ARG_BENCHMARK_INSTALL = "--benchmark-install"
ARG_SKIP_INSTALL = "--skip-install"
ARG_ASAN = "--asan"
ARG_GETDEPS_HELP = "--getdeps-help"
ARG_GETDEPS = "getdeps_args"
ARG_USE_GCC = "--use-gcc"

SUPPORTED_SAI_IMPLS = {
    "SAI_BRCM_IMPL",
    "CHENAB_SAI_SDK",
    "SAI_TAJO_IMPL",
}
SUPPORTED_PHY_IMPLS = {
    "SAI_BRCM_PAI_IMPL",
}
SAI_VERSION_SHAS = {
    "1.13.2": "d60935ba1e5cc7e4ebf2ae7d04f9e937d445e3f875822e27a359c775cb203bae",
    "1.14.0": "4e3a1d010bda0c589db46e077725a2cd9624a5cc255c89d1caa79deb408d1fa7",
    "1.15.0": "94b7a7dd9dbcc46bf14ba9f12b8597e9e9c2069fcb8e383a61cdf6ca172f3511",
    "1.15.3": "fd390d86e7abb419023decf1ec254054450a35d9147b0ad6499e6d12aa860812",
    "1.16.0": "c7d9e85646b28a4d788448db28da649da37cd3ec7955fbeb8d7d80f76ef1796f",
    "1.16.1": "cf65142d1a1286b5faa24c9ae61b3f955f04724d0bf5ef6e5679298353aa0871",
    "1.16.3": "5c89cdb6b2e4f1b42ced6b78d43d06d22434ddbf423cdc551f7c2001f12e63d9",
    "1.17.1": "05411b13b32abcc50f2f2b78e491e503b2b05e5a1503699abd4cc1b81f90d1ae",
    "1.17.4": "362640d5398c53e7257daf67ff7044591937a8443bf037748527bb2cd185f660",
    "1.18.0": "606e35da083056e60e818964bcc0737f229a78f1a150e8aa398bc76b3a360509",
    "1.18.1": "84f2fbd6bf672abaefddfd78a28fec794e37477bf9702fbb56d7bd53ff930ba3",
}
SUPPORTED_SAI_SDK_VERSIONS = {
    # BRCM XGS
    "SAI_VERSION_8_2_0_0_ODP",
    "SAI_VERSION_10_2_0_0_ODP",
    "SAI_VERSION_11_7_0_0_ODP",
    "SAI_VERSION_12_2_0_0_ODP",
    "SAI_VERSION_13_3_0_0_ODP",
    "SAI_VERSION_14_0_EA_ODP",
    "SAI_VERSION_14_2_0_0_ODP",
    "SAI_VERSION_15_4_EA_ODP",
    # BRCM DNX
    "SAI_VERSION_11_7_0_0_DNX_ODP",
    "SAI_VERSION_12_2_0_0_DNX_ODP",
    "SAI_VERSION_13_3_0_0_DNX_ODP",
    "SAI_VERSION_14_0_EA_DNX_ODP",
    "SAI_VERSION_14_2_0_0_DNX_ODP",
    "SAI_VERSION_15_0_EA_DNX_ODP",
    "SAI_VERSION_16_0_EA_DNX_ODP",
    # Tajo
    "TAJO_SDK_VERSION_1_42_8",
    "TAJO_SDK_VERSION_24_8_3001",
    "TAJO_SDK_VERSION_25_5_4210",
    "TAJO_SDK_VERSION_25_11_4210",
    "TAJO_SDK_VERSION_26_2_4210",
    "TAJO_SDK_VERSION_26_2_5210",
    "TAJO_SDK_VERSION_26_5_5211",
    "TAJO_SDK_VERSION_26_5_5210",
    # Chenab
    "CHENAB_SAI_SDK_VERSION_2505_34_0_38",
    "CHENAB_SAI_SDK_VERSION_2511_36_0_20",
    "CHENAB_SAI_SDK_VERSION_2511_6_0_21_ea",
    "CHENAB_SAI_SDK_VERSION_2605_37_0_20",
}

# Per-pass SAI implementation selectors.
PASS_IMPL_NPU = "npu"
PASS_IMPL_PHY = "phy"
PASS_IMPL_FAKE = "fake"

# The CMake target that the PHY pass always (re)builds against the PAI SDK.
PHY_CMAKE_TARGET = "qsfp_targets"

# CMake reads the PAI SDK from this hard-coded location, so a user-provided PAI
# SDK dir/tarball is symlinked/extracted to point here. The expected layout is
# lib/ (the archives below) and include/ (the subdirs below).
PAI_IMPL_DIR = "/var/FBOSS/pai_impl"
PAI_EXPECTED_LIBS = ("libepdm.a", "libpai.a", "libphymodepil.a")
PAI_EXPECTED_INCLUDE_SUBDIRS = ("sai", "pai_macsec", "epdm")

# Env vars that select the SAI implementation. These are reset between passes so
# one pass's selection never leaks into the next.
_SAI_ENV_VARS = (
    "SAI_BRCM_IMPL",
    "SAI_TAJO_IMPL",
    "CHENAB_SAI_SDK",
    "SAI_BRCM_PAI_IMPL",
    "BUILD_SAI_FAKE",
    "SAI_SDK_VERSION",
    "SAI_VERSION",
)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        ARG_NPU_SAI_IMPL,
        required=False,
        help="NPU (agent) SAI implementation to be used for the build. "
        "May be combined with --phy-sai-impl to build the agent against this "
        "NPU SDK and qsfp_targets against the PHY SDK in a single command "
        "(two sequential passes into the same build tree). "
        "If neither is provided, a fake SAI build is used (BUILD_SAI_FAKE=1). "
        f"Meta officially supports: {sorted(SUPPORTED_SAI_IMPLS)}",
    )
    parser.add_argument(
        ARG_PHY_SAI_IMPL,
        required=False,
        help="PHY (XPHY) SAI implementation to be used for the build. "
        "When provided, qsfp_targets are (re)built against the PHY SDK. "
        "If --npu-sai-impl is also provided, the agent is first built against "
        "that NPU SDK; otherwise the rest of the build uses fake SAI. "
        "If neither is provided, a fake SAI build is used (BUILD_SAI_FAKE=1). "
        f"Meta officially supports: {sorted(SUPPORTED_PHY_IMPLS)}",
    )
    parser.add_argument(
        ARG_PHY_SAI_VERSION,
        required=False,
        choices=SAI_VERSION_SHAS.keys(),
        help="SAI spec version to use for the PHY (PAI) build pass. "
        "Only used when --phy-sai-impl is set. When omitted, the PHY pass "
        "uses the build's default SAI version (current behavior).",
    )
    parser.add_argument(
        ARG_PHY_PAI_SDK_PATH,
        required=False,
        help="Path to a prepared PAI SDK directory containing lib/ (with "
        f"{', '.join(PAI_EXPECTED_LIBS)}) and include/ (with "
        f"{', '.join(d + '/' for d in PAI_EXPECTED_INCLUDE_SUBDIRS)}). It is "
        f"symlinked to {PAI_IMPL_DIR} so CMake finds it, instead of requiring "
        f"that directory to be populated by hand. Only used with "
        f"{ARG_PHY_SAI_IMPL}. Mutually exclusive with {ARG_PHY_PAI_SDK_TARBALL}.",
    )
    parser.add_argument(
        ARG_PHY_PAI_SDK_TARBALL,
        required=False,
        help="Path to a PAI SDK tarball with the same lib/ and include/ layout "
        f"as {ARG_PHY_PAI_SDK_PATH} (optionally under a single top-level "
        f"directory). It is extracted and symlinked to {PAI_IMPL_DIR}. Only used "
        f"with {ARG_PHY_SAI_IMPL}. Mutually exclusive with {ARG_PHY_PAI_SDK_PATH}.",
    )
    parser.add_argument(
        ARG_NPU_SAI_VERSION,
        required=False,
        choices=SAI_VERSION_SHAS.keys(),
        help="SAI version to be used for the build.",
    )
    parser.add_argument(
        ARG_NPU_SAI_SDK_VERSION,
        required=False,
        help="SAI SDK version to be used for the build. "
        f"Meta officially supports: {sorted(SUPPORTED_SAI_SDK_VERSIONS)}",
    )
    parser.add_argument(
        ARG_NPU_LIBSAI_IMPL_PATH,
        required=False,
        help="Path to a directory containing libsai_impl.a (and optionally "
        "sai_dependencies.txt and any other SDK libs to stage alongside it).",
    )
    parser.add_argument(
        ARG_NPU_LIBSAI_IMPL_TARBALL,
        required=False,
        help="Full path to a pre-built NPU SDK tarball. May use either the flat "
        f"layout consumed by {ARG_NPU_LIBSAI_IMPL_PATH}/{ARG_NPU_EXPERIMENTS_PATH} "
        "(libsai_impl.a plus an experimental/ headers dir) or a lib/ + include/ "
        "layout, optionally under a single top-level directory. Mutually exclusive "
        f"with {ARG_NPU_LIBSAI_IMPL_PATH} and {ARG_NPU_EXPERIMENTS_PATH}.",
    )
    parser.add_argument(
        ARG_NPU_EXPERIMENTS_PATH,
        required=False,
        help="Full path to SAI spec experiments directory.",
    )
    parser.add_argument(
        "--preserve-env",
        required=False,
        action="store_true",
        help="When set, flags will not overwrite previously set environment variables.",
    )
    parser.add_argument(
        ARG_BENCHMARK_INSTALL,
        required=False,
        action="store_true",
        help="Set this flag to install benchmark binaries.",
    )
    parser.add_argument(
        ARG_SKIP_INSTALL,
        required=False,
        action="store_true",
        help="Set this flag to skip installing binaries.",
    )
    # TODO: re-enable when asain builds are supported
    # parser.add_argument(
    #     ARG_ASAN,
    #     required=False,
    #     action="store_true",
    #     help="Set this flag to enable ASAN builds.",
    # )
    parser.add_argument(
        ARG_GETDEPS_HELP,
        required=False,
        action="store_true",
        help="Set this flag to show the help menu for getdeps.py.",
    )
    parser.add_argument(
        ARG_GETDEPS,
        nargs=argparse.REMAINDER,
        help="Arguments to be passed to getdeps.py.",
    )
    parser.add_argument(
        ARG_USE_GCC,
        required=False,
        action="store_true",
        help="Stay on GCC instead of auto-switching to Clang.",
    )
    return parser.parse_args()


def path_to(*args):
    root = os.environ.get("FBOSS_ROOT") or os.path.join(
        os.path.dirname(__file__), "..", "..", ".."
    )

    if not Path(root).is_dir():
        raise FileNotFoundError(f"Provided root is not valid={root}")

    return os.path.join(root, *args)


def detect_toolchain():
    """
    Detect which toolchain is currently active and extract relevant information.
    Returns a dict with:
      - 'type': "clang" or "gcc"
      - 'target_triple': target architecture triple (e.g., "x86_64-unknown-linux-gnu")
      - 'install_dir': installation directory (e.g., "/usr/local/llvm/bin" for clang)
    Returns None if unable to detect.
    """
    try:
        gxx_version = subprocess.run(
            ["g++", "--version"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError) as e:
        print(f"Warning: Could not detect compiler: {e}", file=sys.stderr)
        return None

    if gxx_version.returncode != 0:
        return None

    # Determine toolchain type
    toolchain_type = None
    target_triple = None
    install_dir = None

    if "clang" in gxx_version.stdout:
        toolchain_type = "clang"
        # Extract target triple and install dir from clang version output
        # Example clang output:
        #   clang version x.y.z (https://github.com/llvm/llvm-project <sha1>)
        #   Target: x86_64-unknown-linux-gnu
        #   Thread model: posix
        #   InstalledDir: /usr/local/llvm/bin
        for line in gxx_version.stdout.splitlines():
            if line.startswith("Target:"):
                target_triple = line.split(":", 1)[1].strip()
            elif line.startswith("InstalledDir:"):
                install_dir = line.split(":", 1)[1].strip()

    elif "GCC" in gxx_version.stdout:
        toolchain_type = "gcc"
        # For GCC, check if LLVM's libunwind is installed
        # Search for libunwind.so in /usr/local/llvm/lib subdirectories (e.g., /usr/local/llvm/lib/x86_64-unknown-linux-gnu)
        llvm_lib = "/usr/local/llvm/lib"
        if os.path.isdir(llvm_lib):
            for entry in os.listdir(llvm_lib):
                candidate = os.path.join(llvm_lib, entry)
                if os.path.isdir(candidate):
                    libunwind_path = os.path.join(candidate, "libunwind.so")
                    if os.path.exists(libunwind_path):
                        install_dir = candidate
                        # Infer target triple from directory name (e.g., "x86_64-unknown-linux-gnu")
                        target_triple = entry
                        break

    else:
        print_info(
            f"Warning: Could not detect toolchain from g++ --version output. "
            f"Expected 'clang' or 'GCC' in output, got:\n{gxx_version.stdout}"
        )
        return None

    return {
        "type": toolchain_type,
        "target_triple": target_triple,
        "install_dir": install_dir,
    }


def _extract_clang_cxxflags():
    """Read clang-specific CXXFLAGS from CMakeLists.txt.

    Returns the list of flags, or None if CMakeLists.txt is not found.
    """
    cxxflags = []
    try:
        with open(os.path.join(os.getcwd(), "CMakeLists.txt")) as f:
            content = f.read()
    except FileNotFoundError:
        print_info("Warning: CMakeLists.txt not found, skipping clang-specific flags")
        return None

    clang_section_match = re.search(
        r'if\s*\(\s*CMAKE_CXX_COMPILER_ID\s+MATCHES\s+"Clang"\s*\)(.*?)endif\(\)',
        content,
        re.DOTALL | re.IGNORECASE,
    )

    if clang_section_match:
        clang_section = clang_section_match.group(1)
        for line_match in re.finditer(r'CMAKE_CXX_FLAGS.*?"([^"]*)"', clang_section):
            flags_part = line_match.group(1)
            for flag in re.findall(r"(-[A-Za-z0-9_\-=]+)", flags_part):
                cxxflags.append(flag)

    additional_flags = [
        "-std=c++20",
        "-Wno-deprecated-ofast",
        "-Wno-reserved-identifier",
        "-Wno-unsafe-buffer-usage",
        "-Wno-logical-op-parentheses",
        "-Wno-deprecated-declarations",
        "-Wno-undef",
    ]
    for flag in additional_flags:
        if flag not in cxxflags:
            cxxflags.insert(0, flag)

    return cxxflags


def _detect_python_march_flag(cxxflags):
    """Detect -march flag from Python sysconfig and append to cxxflags if found."""
    try:
        py_cflags = sysconfig.get_config_var("CFLAGS") or ""
        for token in py_cflags.split():
            if token.startswith("-march="):
                if token not in cxxflags:
                    cxxflags.append(token)
                    print_info(
                        f"Added {token} from Python sysconfig"
                        " to match Cython extensions"
                    )
                break
    except Exception as e:
        print_info(f"WARNING: failed to detect -march from Python sysconfig: {e}")


def _patch_manifests_disable_binutils():
    """Disable binutils in manifests when using clang."""
    for manifest in glob.glob(
        os.path.join(path_to("build", "fbcode_builder", "manifests"), "*")
    ):
        if not os.path.isfile(manifest):
            continue
        with open(manifest) as f:
            content = f.read()
        if "\nbinutils" in content:
            with open(manifest, "w") as f:
                f.write(content.replace("\nbinutils", "\n#binutils"))
            print_info(f"Patching manifest {manifest} to disable binutils")


def setup_clang_environment(toolchain_info):
    """
    Set up the environment for building with clang.
    This reads the clang-specific flags from CMakeLists.txt to avoid duplication.
    Prepends our flags to any existing user flags so user flags can override.

    Args:
        toolchain_info: dict with 'target_triple' and 'install_dir' from detect_toolchain()
    """
    print_info("Detected clang compiler, setting clang-specific flags")

    target_triple = toolchain_info.get("target_triple")
    llvm_bin_dir = toolchain_info.get("install_dir")

    if not llvm_bin_dir:
        print_info("Warning: Could not determine LLVM installation directory")
        return
    if not target_triple:
        print_info("Warning: Could not determine target triple")
        return

    cxxflags = _extract_clang_cxxflags()
    if cxxflags is None:
        return

    # Helper to prepend a value to an environment variable
    def prepend_env(new, var, sep=" "):
        os.environ[var] = (new + sep + os.environ.get(var, "")).strip(sep)
        print_info(f"Set {var}={os.environ[var]}")

    # Set CFLAGS - prepend to existing flags
    cflags = ["-DHAVE_SETNS=1"]
    prepend_env(" ".join(cflags), "CFLAGS")

    # Match Python's -march flag to avoid ABI mismatch between C++ libraries
    # and Cython extensions.  Python sysconfig on some distros (e.g. CentOS
    # Stream 9) embeds -march=x86-64-v2.  Cython inherits this, but cmake
    # does not.  Without matching, folly's F14 hash tables use different
    # intrinsics modes causing undefined symbol errors at runtime.
    # This propagates -march to CXXFLAGS for ALL getdeps dependency builds
    # (folly-python, fbthrift-python, fizz-python, etc.) so their libfolly.so
    # and Cython extensions agree on F14IntrinsicsMode.  The same detection
    # also exists in fboss/github/CMakeLists.txt for direct cmake invocations.
    _detect_python_march_flag(cxxflags)

    # Set CXXFLAGS - prepend to existing flags
    prepend_env(" ".join(cxxflags), "CXXFLAGS")

    # Derive the lib directory from the bin directory
    # e.g., /usr/local/llvm/bin -> /usr/local/llvm/lib/x86_64-unknown-linux-gnu
    # Get the parent directory (e.g., /usr/local/llvm)
    llvm_prefix = os.path.dirname(llvm_bin_dir)
    llvm_lib_dir = os.path.join(llvm_prefix, "lib", target_triple)

    # Set LDFLAGS to embed rpath so binaries can find LLVM libraries without LD_LIBRARY_PATH
    # This allows test binaries to run directly without needing environment setup
    rpath_flag = f"-Wl,-rpath,{llvm_lib_dir}"
    prepend_env(rpath_flag, "LDFLAGS")

    # Set LD_LIBRARY_PATH to find LLVM's libunwind and other libraries during build
    prepend_env(llvm_lib_dir, "LD_LIBRARY_PATH", ":")

    # Make sure we don't install binutils when we use clang.
    _patch_manifests_disable_binutils()


def _libsai_manifest_path():
    return path_to("build", "fbcode_builder", "manifests", "libsai")


def _read_libsai_manifest():
    """Snapshot the current libsai manifest content, or None if it is absent."""
    path = _libsai_manifest_path()
    if not os.path.isfile(path):
        return None
    with open(path) as f:
        return f.read()


def _restore_libsai_manifest(content):
    """Restore the libsai manifest to a snapshot taken before any pass edited it,
    so one pass's --*-sai-version choice never leaks into the next."""
    if content is None:
        return
    with open(_libsai_manifest_path(), "w") as f:
        f.write(content)


def _edit_libsai_manifest(version):
    """Overwrite the libsai manifest with the correct URL and SHA for the given version."""
    url = f"https://github.com/opencomputeproject/SAI/archive/v{version}.tar.gz"
    sha256 = SAI_VERSION_SHAS[version]
    manifest_path = _libsai_manifest_path()
    manifest_str = (
        "[manifest]\n"
        "name = libsai\n"
        "\n"
        "[download]\n"
        f"url = {url}\n"
        f"sha256 = {sha256}\n"
        "\n"
        "[build]\n"
        "builder = nop\n"
        f"subdir = SAI-{version}\n"
        "\n"
        "[install.files]\n"
        "inc = include\n"
        "experimental = experimental\n"
    )
    with open(manifest_path, "w") as f:
        f.write(manifest_str)
    print_info(f"Updated libsai manifest for SAI version {version}")


def _find_dir_with_file(root, filename):
    """First directory at/under root that directly contains filename, or None."""
    for dirpath, _dirs, files in os.walk(root):
        if filename in files:
            return dirpath
    return None


def _stage_npu_sdk(libsai_impl_dir, experiments_dir):
    """Stage a flat NPU SDK into a temp prefix and prepend it to
    CMAKE_PREFIX_PATH. Shared by the --npu-libsai-impl-path and
    --npu-libsai-impl-tarball flows.

    ``libsai_impl_dir`` holds libsai_impl.a (and any sibling libs /
    sai_dependencies.txt). ``experiments_dir`` holds the flat SAI extension
    headers and is symlinked BOTH as include/ (so bare includes like
    <brcm_sai_extensions.h> resolve) and as experimental/ off the staging root
    (so <experimental/...>-prefixed includes resolve, since SAI_IMPL_DIR puts the
    staging root on the include path).
    """
    staging_dir = tempfile.mkdtemp(prefix="fboss_sdk_")
    abs_lib = os.path.abspath(libsai_impl_dir)
    abs_exp = os.path.abspath(experiments_dir)
    os.symlink(abs_lib, os.path.join(staging_dir, "lib"))
    os.symlink(abs_exp, os.path.join(staging_dir, "include"))
    os.symlink(abs_exp, os.path.join(staging_dir, "experimental"))
    print_info(f"Symlinked {abs_lib} -> {os.path.join(staging_dir, 'lib')}")
    print_info(f"Symlinked {abs_exp} -> {os.path.join(staging_dir, 'include')}")
    print_info(f"Symlinked {abs_exp} -> {os.path.join(staging_dir, 'experimental')}")
    print_info(f"Staged SDK artifacts in {staging_dir}")

    existing = os.environ.get("CMAKE_PREFIX_PATH", "")
    os.environ["CMAKE_PREFIX_PATH"] = (
        f"{staging_dir}:{existing}" if existing else staging_dir
    )


def _conditionally_prepare_sdk_artifacts(libsai_impl_path, experiments_path):
    """Validate SDK artifact paths, stage them, and prepend to CMAKE_PREFIX_PATH.

    Both paths must be provided together. ``libsai_impl_path`` is a directory
    that contains libsai_impl.a (and may contain sai_dependencies.txt or
    additional SDK libs). When present, the artifacts are staged into a
    temporary directory with the lib/, include/ and experimental/ structure
    that CMake expects, and that directory is prepended to CMAKE_PREFIX_PATH.
    """
    if (libsai_impl_path is None) and (experiments_path is None):
        print_info(
            f"Both {ARG_NPU_LIBSAI_IMPL_PATH} and {ARG_NPU_EXPERIMENTS_PATH} not provided! Skip preparing SDK artifacts."
        )
        return

    if (libsai_impl_path is None) or (experiments_path is None):
        print_info(
            f"Error: {ARG_NPU_LIBSAI_IMPL_PATH} and {ARG_NPU_EXPERIMENTS_PATH} must both be provided together."
        )
        sys.exit(1)

    # Validate paths exist and are non-empty
    if not os.path.isdir(libsai_impl_path):
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_PATH} path does not exist "
            f"or is not a directory: {libsai_impl_path}"
        )
        sys.exit(1)
    libsai_impl_a = os.path.join(libsai_impl_path, "libsai_impl.a")
    if not os.path.isfile(libsai_impl_a):
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_PATH} directory does not contain "
            f"libsai_impl.a: {libsai_impl_path}"
        )
        sys.exit(1)
    if os.path.getsize(libsai_impl_a) == 0:
        print_error(f"Error: libsai_impl.a is empty: {libsai_impl_a}")
        sys.exit(1)
    if not os.path.isdir(experiments_path):
        print_error(
            f"Error: {ARG_NPU_EXPERIMENTS_PATH} path does not exist "
            f"or is not a directory: {experiments_path}"
        )
        sys.exit(1)
    if not os.listdir(experiments_path):
        print_error(
            f"Error: {ARG_NPU_EXPERIMENTS_PATH} directory is empty: {experiments_path}"
        )
        sys.exit(1)

    _stage_npu_sdk(libsai_impl_path, experiments_path)


def _get_scratch_path(getdeps_args):
    """Extract --scratch-path from getdeps args, or return the conventional default."""
    for i, arg in enumerate(getdeps_args):
        if arg == "--scratch-path" and i + 1 < len(getdeps_args):
            return getdeps_args[i + 1]
        if arg.startswith("--scratch-path="):
            return arg.split("=", 1)[1]
    return "/var/FBOSS/tmp_bld_dir"


def _prepare_sdk_from_tarball(tarball_path, scratch_path):
    """Extract a pre-built NPU SDK tarball and stage it for the build.

    The tarball may use either the flat layout consumed by
    --npu-libsai-impl-path/--npu-experiments-path (libsai_impl.a plus an
    experimental/ headers dir) or the lib/ + include/ layout produced by
    build-helper.py, each optionally wrapped in a single top-level directory.
    It is staged the same way the path flow stages its artifacts (via
    _stage_npu_sdk), so both bare and experimental/-prefixed SAI includes
    resolve.

    Files are extracted to <scratch_path>/installed/sai_impl_tarball/ so they
    live alongside other getdeps-installed packages rather than in /tmp.
    """
    if not os.path.isfile(tarball_path):
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_TARBALL} path does not exist "
            f"or is not a file: {tarball_path}"
        )
        sys.exit(1)
    if os.path.getsize(tarball_path) == 0:
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_TARBALL} file is empty: {tarball_path}"
        )
        sys.exit(1)

    extract_dir = os.path.join(scratch_path, "installed", "sai_impl_tarball")
    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)
    os.makedirs(extract_dir)
    subprocess.run(
        ["tar", "xzf", tarball_path, "-C", extract_dir],
        check=True,
    )
    print_info(f"Extracted SDK tarball {tarball_path} to {extract_dir}")

    # libsai_impl.a locates the SDK root (os.walk descends any wrapper dir). The
    # flat layout keeps the headers in a sibling experimental/; the build-helper
    # layout keeps libsai_impl.a in lib/ and the headers in a sibling include/.
    libsai_dir = _find_dir_with_file(extract_dir, "libsai_impl.a")
    if libsai_dir is None:
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_TARBALL} does not contain "
            f"libsai_impl.a: {tarball_path}"
        )
        sys.exit(1)
    experiments_dir = next(
        (
            d
            for d in (
                os.path.join(libsai_dir, "experimental"),
                os.path.join(os.path.dirname(libsai_dir), "include"),
            )
            if os.path.isdir(d)
        ),
        None,
    )
    if experiments_dir is None:
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_TARBALL} does not contain an "
            f"experimental/ or include/ headers directory: {tarball_path}"
        )
        sys.exit(1)

    _stage_npu_sdk(libsai_dir, experiments_dir)


def _validate_pai_sdk_dir(sdk_dir):
    """Validate that sdk_dir has the lib/ and include/ layout CMake expects for
    the PAI SDK. Exits with a clear error otherwise."""
    if not os.path.isdir(sdk_dir):
        print_error(
            f"Error: PAI SDK path does not exist or is not a directory: {sdk_dir}"
        )
        sys.exit(1)

    lib_dir = os.path.join(sdk_dir, "lib")
    if not os.path.isdir(lib_dir):
        print_error(f"Error: PAI SDK is missing a lib/ subdirectory: {lib_dir}")
        sys.exit(1)
    missing_libs = []
    for lib_name in PAI_EXPECTED_LIBS:
        lib_path = os.path.join(lib_dir, lib_name)
        if not os.path.isfile(lib_path):
            missing_libs.append(lib_name)
        elif os.path.getsize(lib_path) == 0:
            print_error(f"Error: PAI SDK library is empty: {lib_path}")
            sys.exit(1)
    if missing_libs:
        print_error(
            f"Error: PAI SDK lib/ is missing expected libraries: {sorted(missing_libs)}"
        )
        sys.exit(1)

    inc_dir = os.path.join(sdk_dir, "include")
    if not os.path.isdir(inc_dir):
        print_error(f"Error: PAI SDK is missing an include/ subdirectory: {inc_dir}")
        sys.exit(1)
    missing_inc = [
        subdir
        for subdir in PAI_EXPECTED_INCLUDE_SUBDIRS
        if not os.path.isdir(os.path.join(inc_dir, subdir))
    ]
    if missing_inc:
        print_error(
            f"Error: PAI SDK include/ is missing expected subdirectories: "
            f"{sorted(missing_inc)}"
        )
        sys.exit(1)


def _symlink_pai_impl(sdk_dir):
    """Point PAI_IMPL_DIR at sdk_dir so CMake's hard-coded /var/FBOSS/pai_impl
    include/lib paths resolve to the provided SDK."""
    abs_sdk = os.path.abspath(sdk_dir)
    if abs_sdk == os.path.abspath(PAI_IMPL_DIR):
        print_info(f"PAI SDK is already at {PAI_IMPL_DIR}, no symlink needed.")
        return
    if os.path.islink(PAI_IMPL_DIR):
        if os.path.realpath(PAI_IMPL_DIR) == os.path.realpath(abs_sdk):
            print_info(f"Symlink {PAI_IMPL_DIR} -> {abs_sdk} already exists, skipping.")
            return
        os.remove(PAI_IMPL_DIR)
    elif os.path.exists(PAI_IMPL_DIR):
        print_error(
            f"Error: {PAI_IMPL_DIR} already exists and is not a symlink. Remove it "
            f"before using {ARG_PHY_PAI_SDK_PATH}/{ARG_PHY_PAI_SDK_TARBALL}."
        )
        sys.exit(1)
    os.makedirs(os.path.dirname(PAI_IMPL_DIR), exist_ok=True)
    os.symlink(abs_sdk, PAI_IMPL_DIR)
    print_info(f"Created symlink {PAI_IMPL_DIR} -> {abs_sdk}")


def _find_pai_sdk_root(extract_dir):
    """Return the directory that directly contains lib/ within an extracted PAI
    tarball, transparently descending through a single top-level wrapper dir
    (the tarball ships its contents under a pai_impl/ prefix)."""
    if os.path.isdir(os.path.join(extract_dir, "lib")):
        return extract_dir
    subdirs = [
        os.path.join(extract_dir, entry)
        for entry in os.listdir(extract_dir)
        if os.path.isdir(os.path.join(extract_dir, entry))
    ]
    if len(subdirs) == 1 and os.path.isdir(os.path.join(subdirs[0], "lib")):
        return subdirs[0]
    # Fall back to extract_dir; _validate_pai_sdk_dir will emit a clear error.
    return extract_dir


def _prepare_pai_from_dir(sdk_dir):
    """Stage a prepared PAI SDK directory for the build."""
    _validate_pai_sdk_dir(sdk_dir)
    _symlink_pai_impl(sdk_dir)


def _prepare_pai_from_tarball(tarball_path, scratch_path):
    """Extract a PAI SDK tarball and stage it for the build.

    Files are extracted to <scratch_path>/installed/pai_impl_tarball/ so they live
    alongside other getdeps-installed packages rather than in /tmp."""
    if not os.path.isfile(tarball_path):
        print_error(
            f"Error: {ARG_PHY_PAI_SDK_TARBALL} path does not exist "
            f"or is not a file: {tarball_path}"
        )
        sys.exit(1)
    if os.path.getsize(tarball_path) == 0:
        print_error(f"Error: {ARG_PHY_PAI_SDK_TARBALL} file is empty: {tarball_path}")
        sys.exit(1)

    extract_dir = os.path.join(scratch_path, "installed", "pai_impl_tarball")
    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)
    os.makedirs(extract_dir)
    subprocess.run(["tar", "xzf", tarball_path, "-C", extract_dir], check=True)
    print_info(f"Extracted PAI SDK tarball {tarball_path} to {extract_dir}")

    sdk_dir = _find_pai_sdk_root(extract_dir)
    _validate_pai_sdk_dir(sdk_dir)
    _symlink_pai_impl(sdk_dir)


def _warn_if_unsupported(flag_name, value, supported_values):
    """Print an info message if value is not in the set of officially supported values."""
    if value not in supported_values:
        print_info(
            f"Note: '{value}' is not a Meta officially supported value for {flag_name}. "
            f"Supported values: {sorted(supported_values)}"
        )


def _validate_sai_flags(args):
    """Validate SAI flag combinations. NPU and PHY are no longer mutually
    exclusive: they may be combined to build the agent against the NPU SDK and
    qsfp_targets against the PHY SDK in a single command."""
    if args.npu_sai_impl is not None:
        _warn_if_unsupported(ARG_NPU_SAI_IMPL, args.npu_sai_impl, SUPPORTED_SAI_IMPLS)
        # Real NPU SAI impl requires an SDK version.
        if args.npu_sai_sdk_version is None:
            print_error(
                f"Error: {ARG_NPU_SAI_SDK_VERSION} is required when using "
                f"{ARG_NPU_SAI_IMPL}={args.npu_sai_impl}."
            )
            sys.exit(1)
        _warn_if_unsupported(
            ARG_NPU_SAI_SDK_VERSION,
            args.npu_sai_sdk_version,
            SUPPORTED_SAI_SDK_VERSIONS,
        )

    if args.phy_sai_impl is not None:
        _warn_if_unsupported(ARG_PHY_SAI_IMPL, args.phy_sai_impl, SUPPORTED_PHY_IMPLS)

    if args.phy_pai_sdk_path is not None and args.phy_pai_sdk_tarball is not None:
        print_error(
            f"Error: {ARG_PHY_PAI_SDK_PATH} and {ARG_PHY_PAI_SDK_TARBALL} are "
            "mutually exclusive."
        )
        sys.exit(1)

    phy_only_flags = [
        (ARG_PHY_SAI_VERSION, args.phy_sai_version is not None),
        (ARG_PHY_PAI_SDK_PATH, args.phy_pai_sdk_path is not None),
        (ARG_PHY_PAI_SDK_TARBALL, args.phy_pai_sdk_tarball is not None),
    ]
    if args.phy_sai_impl is None:
        for flag_name, provided in phy_only_flags:
            if provided:
                print_info(
                    f"Warning: {flag_name} is provided but {ARG_PHY_SAI_IMPL} "
                    "is not set; it will be ignored."
                )


def _impl_env_vars(args, impl):
    """Return the SAI env vars to export for a single build pass."""
    env_vars = {}
    if impl == PASS_IMPL_NPU:
        env_vars[args.npu_sai_impl] = "1"
        env_vars["SAI_SDK_VERSION"] = args.npu_sai_sdk_version
        if args.npu_sai_version is not None:
            env_vars["SAI_VERSION"] = args.npu_sai_version
    elif impl == PASS_IMPL_PHY:
        env_vars[args.phy_sai_impl] = "1"
        if args.phy_sai_version is not None:
            env_vars["SAI_VERSION"] = args.phy_sai_version
    elif impl == PASS_IMPL_FAKE:
        env_vars["BUILD_SAI_FAKE"] = "1"
        # A fake build still targets a SAI spec version; honor --npu-sai-version.
        if args.npu_sai_version is not None:
            env_vars["SAI_VERSION"] = args.npu_sai_version
    return env_vars


def _reset_sai_env(orig_sai_env, preserve_env):
    """Clear SAI selection env vars before a pass, restoring any that were set in
    the original environment when --preserve-env is requested."""
    for name in _SAI_ENV_VARS:
        if preserve_env and name in orig_sai_env:
            os.environ[name] = orig_sai_env[name]
        else:
            os.environ.pop(name, None)


def _apply_env(env_vars, preserve_env):
    """Export env vars, honoring --preserve-env for already-set vars."""
    for name, value in env_vars.items():
        if preserve_env and name in os.environ:
            print_info(
                f"Preserved existing ENV {name}={os.environ[name]} "
                f"(flag value was {value})"
            )
        else:
            os.environ[name] = str(value).strip()
            print_info(f"Set ENV {name}={value}")


def _set_misc_env(args):
    """Set pass-independent build env vars once."""
    misc = {}
    if args.benchmark_install:
        misc["BENCHMARK_INSTALL"] = "1"
    if args.skip_install:
        misc["SKIP_ALL_INSTALL"] = "1"
    _apply_env(misc, args.preserve_env)


def _get_cmake_target(getdeps_args):
    """Return the --cmake-target value from getdeps args, or None."""
    for i, arg in enumerate(getdeps_args):
        if arg == "--cmake-target" and i + 1 < len(getdeps_args):
            return getdeps_args[i + 1]
        if arg.startswith("--cmake-target="):
            return arg.split("=", 1)[1]
    return None


def _force_cmake_target(getdeps_args, target):
    """Return a copy of getdeps args with --cmake-target set to target,
    replacing any existing --cmake-target. The flag is inserted right after the
    getdeps subcommand (e.g. 'build') so it precedes the positional project."""
    result = []
    skip_next = False
    for arg in getdeps_args:
        if skip_next:
            skip_next = False
            continue
        if arg == "--cmake-target":
            skip_next = True
            continue
        if arg.startswith("--cmake-target="):
            continue
        result.append(arg)
    if result:
        result[1:1] = ["--cmake-target", target]
    else:
        result = ["--cmake-target", target]
    return result


def _pass_spec(impl, label, getdeps_args):
    """Build one self-contained pass description.

    getdeps_args are the exact arguments to hand to getdeps.py for this pass, so
    the caller never has to reinterpret them."""
    return {"impl": impl, "label": label, "getdeps_args": getdeps_args}


def _get_pass_specs(args):
    """Return the ordered build passes to run, derived from the SAI flags.

    The PHY pass always (re)builds just qsfp_targets against the PAI SDK; every
    other pass builds whatever the caller asked for (args.getdeps_args as-is).
    Passes run into the same build tree, so a later pass overwrites the earlier
    pass's artifacts (this is how PHY qsfp overwrites the base build's qsfp)."""
    caller_args = args.getdeps_args
    npu = args.npu_sai_impl is not None
    phy = args.phy_sai_impl is not None

    qsfp_phy_pass = _pass_spec(
        PASS_IMPL_PHY,
        "PHY (PAI) qsfp",
        _force_cmake_target(caller_args, PHY_CMAKE_TARGET),
    )

    if npu and phy:
        # Build the agent (and any requested targets) against the NPU SDK, then
        # rebuild qsfp against the PHY SDK on top.
        return [_pass_spec(PASS_IMPL_NPU, "NPU base", caller_args), qsfp_phy_pass]

    if phy:  # PHY only (npu is False here)
        # If the caller only wants qsfp, a single PHY pass is enough. Otherwise
        # build everything with fake SAI first (no NPU SDK needed), then rebuild
        # qsfp against the PHY SDK on top.
        if _get_cmake_target(caller_args) == PHY_CMAKE_TARGET:
            return [_pass_spec(PASS_IMPL_PHY, "PHY (PAI) qsfp", caller_args)]
        return [_pass_spec(PASS_IMPL_FAKE, "fake SAI base", caller_args), qsfp_phy_pass]

    if npu:
        return [_pass_spec(PASS_IMPL_NPU, "NPU", caller_args)]

    print_info(
        "No SAI implementation provided, defaulting to fake SAI build (BUILD_SAI_FAKE=1)"
    )
    return [_pass_spec(PASS_IMPL_FAKE, "fake SAI", caller_args)]


def _prepare_pass(
    args, pass_spec, orig_sai_env, orig_cmake_prefix, orig_libsai_manifest
):
    """Set up env + SDK artifacts for a single pass."""
    impl = pass_spec["impl"]

    # Start each pass from a clean SAI env, CMAKE_PREFIX_PATH, and libsai manifest
    # so the previous pass's SDK/version selection cannot leak into this one.
    _reset_sai_env(orig_sai_env, args.preserve_env)
    if orig_cmake_prefix is None:
        os.environ.pop("CMAKE_PREFIX_PATH", None)
    else:
        os.environ["CMAKE_PREFIX_PATH"] = orig_cmake_prefix
    _restore_libsai_manifest(orig_libsai_manifest)

    _apply_env(_impl_env_vars(args, impl), args.preserve_env)

    if impl == PASS_IMPL_NPU:
        # Stage the NPU SDK artifacts onto CMAKE_PREFIX_PATH.
        if args.npu_libsai_impl_tarball is not None:
            if (
                args.npu_libsai_impl_path is not None
                or args.npu_experiments_path is not None
            ):
                print_error(
                    f"Error: {ARG_NPU_LIBSAI_IMPL_TARBALL} is mutually exclusive with "
                    f"{ARG_NPU_LIBSAI_IMPL_PATH} and {ARG_NPU_EXPERIMENTS_PATH}."
                )
                sys.exit(1)
            _prepare_sdk_from_tarball(
                args.npu_libsai_impl_tarball, _get_scratch_path(args.getdeps_args)
            )
        else:
            _conditionally_prepare_sdk_artifacts(
                args.npu_libsai_impl_path, args.npu_experiments_path
            )
    elif impl == PASS_IMPL_PHY:
        # Stage a user-provided PAI SDK so CMake's hard-coded /var/FBOSS/pai_impl
        # resolves to it. If neither flag is given, rely on that directory being
        # populated already (unchanged behavior).
        if args.phy_pai_sdk_tarball is not None:
            _prepare_pai_from_tarball(
                args.phy_pai_sdk_tarball, _get_scratch_path(args.getdeps_args)
            )
        elif args.phy_pai_sdk_path is not None:
            _prepare_pai_from_dir(args.phy_pai_sdk_path)

    # Pin the libsai (SAI spec) version for the pass: the base pass (NPU or fake)
    # follows --npu-sai-version; the PHY pass follows --phy-sai-version.
    if impl in (PASS_IMPL_NPU, PASS_IMPL_FAKE):
        if args.npu_sai_version is not None:
            _edit_libsai_manifest(args.npu_sai_version)
    elif impl == PASS_IMPL_PHY:
        if args.phy_sai_version is not None:
            _edit_libsai_manifest(args.phy_sai_version)


def _setup_toolchain(args):
    """Detect the active toolchain and configure the build environment once.
    This is global and applies to every build pass."""
    toolchain_info = detect_toolchain()

    set_clang_cmd = [
        "update-alternatives",
        "--set",
        "gcc",
        "/usr/local/llvm/bin/clang",
    ]
    set_gcc_cmd = [
        "update-alternatives",
        "--set",
        "gcc",
        "/opt/rh/gcc-toolset-12/root/usr/bin/gcc",
    ]
    if toolchain_info:
        print_info(f"Detected toolchain: {toolchain_info}")
        if args.use_gcc:
            print_info(f"{ARG_USE_GCC} set, proceeding with GCC")
            subprocess.run(set_gcc_cmd, check=False)
        elif toolchain_info["type"] == "clang":
            setup_clang_environment(toolchain_info)
        elif toolchain_info["type"] == "gcc":
            print_info("Switching to Clang via update-alternatives...")
            result = subprocess.run(set_clang_cmd, check=False)
            if result.returncode != 0:
                print_error(
                    "Failed to switch to Clang via update-alternatives. "
                    f"Please run manually: {' '.join(set_clang_cmd)}"
                )
                sys.exit(1)
            toolchain_info = detect_toolchain()
            if toolchain_info and toolchain_info["type"] == "clang":
                setup_clang_environment(toolchain_info)
            else:
                print_error(
                    "Failed to detect Clang after switching. "
                    f"Please run manually: {' '.join(set_clang_cmd)}"
                )
                sys.exit(1)
    # If toolchain_info is None, detect_toolchain() already printed a warning
    # and we'll proceed without environment setup


def main():
    args = parse_args()
    print_info("Starting run-getdeps.py")
    getdeps_path = path_to("build", "fbcode_builder", "getdeps.py")

    if args.getdeps_help:
        os.execv(getdeps_path, [getdeps_path, "-h"])

    _validate_sai_flags(args)
    _set_misc_env(args)

    # Toolchain setup is global; do it once before any pass.
    _setup_toolchain(args)

    passes = _get_pass_specs(args)

    # Snapshot the state we mutate per-pass so each pass starts from a clean slate.
    orig_sai_env = {v: os.environ[v] for v in _SAI_ENV_VARS if v in os.environ}
    orig_cmake_prefix = os.environ.get("CMAKE_PREFIX_PATH")
    orig_libsai_manifest = _read_libsai_manifest()

    # Run each pass as a subprocess so control returns between passes. Passes
    # build into the same tree, so a later pass overwrites the artifacts of an
    # earlier one (e.g. PHY qsfp overwrites the base build's qsfp). Abort on the
    # first failure.
    for i, pass_spec in enumerate(passes):
        print_info(f"=== Build pass {i + 1}/{len(passes)}: {pass_spec['label']} ===")
        _prepare_pass(
            args, pass_spec, orig_sai_env, orig_cmake_prefix, orig_libsai_manifest
        )
        gd_args = pass_spec["getdeps_args"]
        print_info(f"Executing getdeps.py with args: {gd_args}")
        result = subprocess.run([getdeps_path, *gd_args], check=False)
        if result.returncode != 0:
            print_error(
                f"Build pass '{pass_spec['label']}' failed with exit code "
                f"{result.returncode}."
            )
            sys.exit(result.returncode)


if __name__ == "__main__":
    main()
