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
import sysconfig
import tempfile


def print_info(msg):
    print(f"\033[93m{msg}\033[0m")


def print_error(msg):
    print(f"\033[91m{msg}\033[0m")


# SDK Related Flags
ARG_NPU_SAI_IMPL = "--npu-sai-impl"
ARG_NPU_SAI_VERSION = "--npu-sai-version"
ARG_NPU_SAI_SDK_VERSION = "--npu-sai-sdk-version"
ARG_NPU_LIBSAI_IMPL_PATH = "--npu-libsai-impl-path"
ARG_NPU_EXPERIMENTS_PATH = "--npu-experiments-path"
ARG_PHY_SAI_IMPL = "--phy-sai-impl"

# Misc Build Flags
ARG_BENCHMARK_INSTALL = "--benchmark-install"
ARG_SKIP_INSTALL = "--skip-install"
ARG_ASAN = "--asan"
ARG_GETDEPS_HELP = "--getdeps-help"
ARG_GETDEPS = "getdeps_args"

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
    "SAI_VERSION_15_0_EA_ODP",
    # BRCM DNX
    "SAI_VERSION_11_7_0_0_DNX_ODP",
    "SAI_VERSION_12_2_0_0_DNX_ODP",
    "SAI_VERSION_13_3_0_0_DNX_ODP",
    "SAI_VERSION_14_0_EA_DNX_ODP",
    "SAI_VERSION_14_2_0_0_DNX_ODP",
    "SAI_VERSION_15_0_EA_DNX_ODP",
    # Tajo
    "TAJO_SDK_VERSION_1_42_8",
    "TAJO_SDK_VERSION_24_8_3001",
    "TAJO_SDK_VERSION_25_5_4210",
    "TAJO_SDK_VERSION_25_11_5210",
    # Chenab
    "CHENAB_SAI_SDK_VERSION_2505_34_0_38",
    "CHENAB_SAI_SDK_VERSION_2511_35_0_19",
}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        ARG_NPU_SAI_IMPL,
        required=False,
        help="SAI implementation to be used for the build. "
        "Mutually exclusive with --phy-sai-impl. "
        "If neither is provided, a fake SAI build is used (BUILD_SAI_FAKE=1). "
        f"Meta officially supports: {sorted(SUPPORTED_SAI_IMPLS)}",
    )
    parser.add_argument(
        ARG_PHY_SAI_IMPL,
        required=False,
        help="PHY (XPHY) SAI implementation to be used for the build. "
        "Mutually exclusive with --npu-sai-impl. "
        "If neither is provided, a fake SAI build is used (BUILD_SAI_FAKE=1). "
        f"Meta officially supports: {sorted(SUPPORTED_PHY_IMPLS)}",
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
            ["g++", "--version"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
    except (subprocess.TimeoutExpired, OSError) as e:
        print_info(f"Warning: Could not detect compiler: {e}")
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

    # Read clang-specific CXXFLAGS from CMakeLists.txt using regex
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
        "experimental = experimental\n"
    )
    with open(manifest_path, "w") as f:
        f.write(manifest_str)
    print_info(f"Updated libsai manifest for SAI version {version}")


def _conditionally_prepare_sdk_artifacts(libsai_impl_path, experiments_path):
    """Validate SDK artifact paths, stage them, and prepend to CMAKE_PREFIX_PATH.

    Both paths must be provided together. When present, the artifacts are
    staged into a temporary directory with the lib/ and include/ structure
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
    if not os.path.isfile(libsai_impl_path):
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_PATH} path does not exist "
            f"or is not a file: {libsai_impl_path}"
        )
        sys.exit(1)
    if os.path.getsize(libsai_impl_path) == 0:
        print_error(
            f"Error: {ARG_NPU_LIBSAI_IMPL_PATH} file is empty: {libsai_impl_path}"
        )
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
    print_info(f"Symlinked {libsai_impl_path} -> {lib_dir}")
    print_info(
        f"Symlinked {experiments_path} -> {os.path.join(staging_dir, 'include')}"
    )

    print_info(f"Staged SDK artifacts in {staging_dir}")

    # Prepend the staging directory to CMAKE_PREFIX_PATH
    existing = os.environ.get("CMAKE_PREFIX_PATH", "")
    os.environ["CMAKE_PREFIX_PATH"] = (
        f"{staging_dir}:{existing}" if existing else staging_dir
    )


def _warn_if_unsupported(flag_name, value, supported_values):
    """Print an info message if value is not in the set of officially supported values."""
    if value not in supported_values:
        print_info(
            f"Note: '{value}' is not a Meta officially supported value for {flag_name}. "
            f"Supported values: {sorted(supported_values)}"
        )


def _set_build_env_vars(args):
    """
    Validate flag groups and set the corresponding environment variables.
    """
    env_vars = {}

    # --- SAI impl mutual exclusivity check ---
    if args.npu_sai_impl is not None and args.phy_sai_impl is not None:
        print_error(
            f"Error: {ARG_NPU_SAI_IMPL} and {ARG_PHY_SAI_IMPL} are mutually exclusive. "
            "Specify only one SAI implementation."
        )
        sys.exit(1)

    # --- SAI flags ---
    if args.npu_sai_version is not None:
        env_vars["SAI_VERSION"] = args.npu_sai_version

    if args.npu_sai_impl is not None:
        _warn_if_unsupported(ARG_NPU_SAI_IMPL, args.npu_sai_impl, SUPPORTED_SAI_IMPLS)

        # Real SAI impl requires an SDK version
        if args.npu_sai_sdk_version is None:
            print_error(
                f"Error: {ARG_NPU_SAI_SDK_VERSION} is required when using "
                f"{ARG_NPU_SAI_IMPL}={args.npu_sai_impl}."
            )
            sys.exit(1)
        env_vars[args.npu_sai_impl] = "1"

        _warn_if_unsupported(
            ARG_NPU_SAI_SDK_VERSION,
            args.npu_sai_sdk_version,
            SUPPORTED_SAI_SDK_VERSIONS,
        )
        env_vars["SAI_SDK_VERSION"] = args.npu_sai_sdk_version
    elif args.phy_sai_impl is not None:
        _warn_if_unsupported(ARG_PHY_SAI_IMPL, args.phy_sai_impl, SUPPORTED_PHY_IMPLS)
        env_vars[args.phy_sai_impl] = "1"
    else:
        print_info(
            "No SAI implementation provided, defaulting to fake SAI build (BUILD_SAI_FAKE=1)"
        )
        env_vars["BUILD_SAI_FAKE"] = "1"

    # --- Misc build flags ---
    if args.benchmark_install:
        env_vars["BENCHMARK_INSTALL"] = "1"
    if args.skip_install:
        env_vars["SKIP_ALL_INSTALL"] = "1"

    # Set all collected env vars
    for name, value in env_vars.items():
        if args.preserve_env and name in os.environ:
            print_info(
                f"Preserved existing ENV {name}={os.environ[name]} "
                f"(flag value was {value})"
            )
        else:
            os.environ[name] = str(value).strip()
            print_info(f"Set ENV {name}={value}")


def main():
    args = parse_args()
    print_info("Starting run-getdeps.py")
    getdeps_path = path_to("build", "fbcode_builder", "getdeps.py")

    if args.getdeps_help:
        os.execv(getdeps_path, [getdeps_path, "-h"])

    _set_build_env_vars(args)

    _conditionally_prepare_sdk_artifacts(
        args.npu_libsai_impl_path, args.npu_experiments_path
    )

    if args.npu_sai_version is not None:
        _edit_libsai_manifest(args.npu_sai_version)

    # Detect which toolchain is active and set up environment accordingly
    toolchain_info = detect_toolchain()

    if toolchain_info:
        print_info(f"Detected toolchain: {toolchain_info}")
        if toolchain_info["type"] == "clang":
            setup_clang_environment(toolchain_info)
        elif toolchain_info["type"] == "gcc":
            print_info("Detected GCC compiler, no additional flags needed")
    # If toolchain_info is None, detect_toolchain() already printed a warning
    # and we'll proceed without environment setup

    # Call the real getdeps.py with all arguments
    print_info(f"Executing getdeps.py with args: {args.getdeps_args}")
    os.execv(getdeps_path, [getdeps_path, *args.getdeps_args])


if __name__ == "__main__":
    main()
