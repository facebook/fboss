#!/usr/bin/env python3
# Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
# All Rights Reserved

"""
Wrapper script for getdeps.py that sets up clang-specific environment variables.

This script auto-detects if we're using clang and sets appropriate environment
variables before calling the real getdeps.py. This allows us to configure the
build environment without modifying the upstream getdeps.py script.
"""

import glob
import os
import re
import subprocess
import sys


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


def main():
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
    getdeps_path = path_to("build", "fbcode_builder", "getdeps.py")
    os.execv(getdeps_path, [getdeps_path] + sys.argv[1:])


if __name__ == "__main__":
    main()
