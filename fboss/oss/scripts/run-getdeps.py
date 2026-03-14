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
import subprocess
import sys
import tempfile


# SDK Related Flags
ARG_NPU_SAI_IMPL = "--npu-sai-impl"
ARG_NPU_SAI_VERSION = "--npu-sai-version"
ARG_NPU_SAI_SDK_VERSION = "--npu-sai-sdk-version"
ARG_NPU_LIBSAI_IMPL_PATH = "--npu-libsai-impl-path"
ARG_NPU_EXPERIMENTS_PATH = "--npu-experiments-path"

# Misc Build Flags
ARG_BENCHMARK_INSTALL = "--benchmark-install"
ARG_SKIP_INSTALL = "--skip-install"
ARG_ASAN = "--asan"
ARG_GETDEPS_HELP = "--getdeps-help"
ARG_GETDEPS = "getdeps_args"

SAI_IMPL_CHOICES = [
    "SAI_BRCM_IMPL",
    "SAI_BRCM_PAI_IMPL",
    "CHENAB_SAI_SDK",
    "SAI_TAJO_IMPL",
    "BUILD_SAI_FAKE",
]
SAI_VERSION_SHAS = {
    "1.13.2": "d60935ba1e5cc7e4ebf2ae7d04f9e937d445e3f875822e27a359c775cb203bae",
    "1.14.0": "4e3a1d010bda0c589db46e077725a2cd9624a5cc255c89d1caa79deb408d1fa7",
    "1.15.0": "94b7a7dd9dbcc46bf14ba9f12b8597e9e9c2069fcb8e383a61cdf6ca172f3511",
    "1.15.3": "fd390d86e7abb419023decf1ec254054450a35d9147b0ad6499e6d12aa860812",
    "1.16.0": "c7d9e85646b28a4d788448db28da649da37cd3ec7955fbeb8d7d80f76ef1796f",
    "1.16.1": "cf65142d1a1286b5faa24c9ae61b3f955f04724d0bf5ef6e5679298353aa0871",
    "1.16.3": "5c89cdb6b2e4f1b42ced6b78d43d06d22434ddbf423cdc551f7c2001f12e63d9",
    "1.17.1": "05411b13b32abcc50f2f2b78e491e503b2b05e5a1503699abd4cc1b81f90d1ae",
}
# TODO: fill out
SAI_SDK_VERSIONS = [
    "SAI_VERSION_14_0_EA_ODP",
]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        ARG_NPU_SAI_IMPL,
        required=False,
        choices=SAI_IMPL_CHOICES,
        help="SAI implementation to be used for the build.",
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
        choices=SAI_SDK_VERSIONS,
        help="SAI SDK version to be used for the build.",
    )
    parser.add_argument(
        ARG_NPU_LIBSAI_IMPL_PATH,
        required=False,
        # TODO: can we also specify a directory of dynamic libs, or should
        # we add this to a diff flag
        help="Full path to libsai_impl.a.",
    )
    parser.add_argument(
        ARG_NPU_EXPERIMENTS_PATH,
        required=False,
        help="Full path to SAI spec experiments directory.",
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
    return parser.parse_args()


def path_to(*args):
    return os.path.join(os.path.dirname(__file__), "..", "..", "..", *args)


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
            ["g++", "--version"], capture_output=True, text=True, timeout=5
        )
    except (subprocess.TimeoutExpired, OSError) as e:
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
        print(
            f"Warning: Could not detect toolchain from g++ --version output. "
            f"Expected 'clang' or 'GCC' in output, got:\n{gxx_version.stdout}",
            file=sys.stderr,
        )
        return None

    return {
        "type": toolchain_type,
        "target_triple": target_triple,
        "install_dir": install_dir,
    }


def setup_clang_environment(toolchain_info):
    """
    Set up the environment for building with clang.
    This reads the clang-specific flags from CMakeLists.txt to avoid duplication.
    Prepends our flags to any existing user flags so user flags can override.

    Args:
        toolchain_info: dict with 'target_triple' and 'install_dir' from detect_toolchain()
    """
    print("Detected clang compiler, setting clang-specific flags", file=sys.stderr)

    target_triple = toolchain_info.get("target_triple")
    llvm_bin_dir = toolchain_info.get("install_dir")

    if not llvm_bin_dir:
        print(
            "Warning: Could not determine LLVM installation directory", file=sys.stderr
        )
        return
    if not target_triple:
        print("Warning: Could not determine target triple", file=sys.stderr)
        return

    # Read clang-specific CXXFLAGS from CMakeLists.txt using regex
    cxxflags = []
    try:
        with open(os.path.join(os.getcwd(), "CMakeLists.txt"), "r") as f:
            content = f.read()
    except FileNotFoundError:
        # CMakeLists.txt not found - this can happen during Docker build
        print(
            "Warning: CMakeLists.txt not found, skipping clang-specific flags",
            file=sys.stderr,
        )
        return

    # Extract the clang-specific section: if (CMAKE_CXX_COMPILER_ID MATCHES "Clang") ... endif()
    clang_section_match = re.search(
        r'if\s*\(\s*CMAKE_CXX_COMPILER_ID\s+MATCHES\s+"Clang"\s*\)(.*?)endif\(\)',
        content,
        re.DOTALL | re.IGNORECASE,
    )

    if clang_section_match:
        clang_section = clang_section_match.group(1)
        # Extract CMAKE_CXX_FLAGS lines and get all flags from them
        for line_match in re.finditer(r'CMAKE_CXX_FLAGS.*?"([^"]*)"', clang_section):
            flags_part = line_match.group(1)
            # Extract individual flags (e.g., -Wno-something, -DFMT_USE_CONSTEVAL=0)
            for flag in re.findall(r"(-[A-Za-z0-9_\-=]+)", flags_part):
                cxxflags.append(flag)

    # Add additional flags needed for third-party dependencies
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

    # Helper to prepend a value to an environment variable
    def prepend_env(new, var, sep=" "):
        os.environ[var] = (new + sep + os.environ.get(var, "")).strip(sep)

    # Set CFLAGS - prepend to existing flags
    cflags = ["-DHAVE_SETNS=1"]
    prepend_env(" ".join(cflags), "CFLAGS")

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
    # This is ugly but makes gcc compatibility simpler.
    for manifest in glob.glob(
        os.path.join(path_to("build", "fbcode_builder", "manifests"), "*")
    ):
        with open(manifest, "r") as f:
            content = f.read()
        if "\nbinutils" in content:
            with open(manifest, "w") as f:
                f.write(content.replace("\nbinutils", "\n#binutils"))


def _edit_libsai_manifest(version):
    """Overwrite the libsai manifest with the correct URL and SHA for the given version."""
    url = f"https://github.com/opencomputeproject/SAI/archive/v{version}.tar.gz"
    sha256 = SAI_VERSION_SHAS[version]
    manifest_path = path_to("build", "fbcode_builder", "manifests", "libsai")
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
    )
    with open(manifest_path, "w") as f:
        f.write(manifest_str)
    print(f"Updated libsai manifest for SAI version {version}", file=sys.stderr)


def _conditionally_prepare_sdk_artifacts(libsai_impl_path, experiments_path):
    """Validate SDK artifact paths, stage them, and prepend to CMAKE_PREFIX_PATH.

    Both paths must be provided together. When present, the artifacts are
    staged into a temporary directory with the lib/ and include/ structure
    that CMake expects, and that directory is prepended to CMAKE_PREFIX_PATH.
    """
    if (libsai_impl_path is None) and (experiments_path is None):
        print(
            f"Both {ARG_NPU_LIBSAI_IMPL_PATH} and {ARG_NPU_EXPERIMENTS_PATH} not provided! Skip preparing SDK artifacts.",
            file=sys.stderr,
        )
        return

    if (libsai_impl_path is None) or (experiments_path is None):
        print(
            f"Error: {ARG_NPU_LIBSAI_IMPL_PATH} and {ARG_NPU_EXPERIMENTS_PATH} must both be provided together.",
            file=sys.stderr,
        )
        sys.exit(1)

    # Stage artifacts into a prefix directory with lib/ and include/ subdirs
    # using symlinks to avoid copying potentially large files.
    staging_dir = tempfile.mkdtemp(prefix="fboss_sdk_")
    lib_dir = os.path.join(staging_dir, "lib")
    os.makedirs(lib_dir)
    os.symlink(
        os.path.abspath(libsai_impl_path),
        os.path.join(lib_dir, os.path.basename(libsai_impl_path)),
    )
    os.symlink(os.path.abspath(experiments_path), os.path.join(staging_dir, "include"))

    print(f"Staged SDK artifacts in {staging_dir}", file=sys.stderr)

    # Prepend the staging directory to CMAKE_PREFIX_PATH
    existing = os.environ.get("CMAKE_PREFIX_PATH", "")
    os.environ["CMAKE_PREFIX_PATH"] = (
        f"{staging_dir}:{existing}" if existing else staging_dir
    )


def main():
    args = parse_args()
    getdeps_path = path_to("build", "fbcode_builder", "getdeps.py")

    if args.getdeps_help:
        os.execv(getdeps_path, [getdeps_path, "-h"])

    _conditionally_prepare_sdk_artifacts(
        args.npu_libsai_impl_path, args.npu_experiments_path
    )

    if args.npu_sai_version is not None:
        _edit_libsai_manifest(args.npu_sai_version)

    # Detect which toolchain is active and set up environment accordingly
    toolchain_info = detect_toolchain()

    if toolchain_info:
        if toolchain_info["type"] == "clang":
            setup_clang_environment(toolchain_info)
        elif toolchain_info["type"] == "gcc":
            pass  # No action needed for GCC
    # If toolchain_info is None, detect_toolchain() already printed a warning
    # and we'll proceed without environment setup

    # Call the real getdeps.py with all arguments
    os.execv(getdeps_path, [getdeps_path] + args.getdeps_args)


if __name__ == "__main__":
    main()
