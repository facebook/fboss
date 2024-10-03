#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import glob
import os
import shutil
import subprocess
import tempfile


OPT_ARG_SCRATCH_PATH = "--scratch-path"


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        OPT_ARG_SCRATCH_PATH,
        type=str,
        help=(
            "use this path for build and install files e.g. "
            + OPT_ARG_SCRATCH_PATH
            + "=/opt/app"
        ),
    )
    parser.add_argument(
        "--compress", help="Compress the FBOSS Binaries", action="store_true"
    )
    return parser.parse_args()


class PackageFboss:
    FBOSS_BINS = "fboss_bins-1"
    FBOSS_BIN_TAR = "fboss_bins.tar.zst"
    FBOSS = "fboss"

    BIN = "bin"
    LIB = "lib"
    DATA = "share"

    # Suffix for getdeps output location
    INSTALLED = "installed"
    BUILD = "build"

    GETDEPS = "build/fbcode_builder/getdeps.py"

    DEVTOOLS_LIBRARY_PATH = "/opt/rh/devtoolset-8/root/usr/lib64"

    NAME_TO_EXECUTABLES = {
        FBOSS: (BIN, [], BUILD),
        "gflags": (LIB, [], INSTALLED),
        "glog": (LIB, [], INSTALLED),
        "libevent": (LIB, [], INSTALLED),
        "libgpiod": (LIB, [], INSTALLED),
        "libsodium": (LIB, [], INSTALLED),
        "python": (LIB, [], INSTALLED),
    }

    def __init__(self):

        self.scratch_path = args.scratch_path
        self.git_path = os.path.join(
            self.scratch_path, "repos/github.com-facebook-fboss.git"
        )
        self.tmp_dir_name = tempfile.mkdtemp(
            prefix=PackageFboss.FBOSS_BINS, dir=args.scratch_path
        )
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.BIN))
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.LIB))
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.DATA))

    def _get_dir_for(self, name, location):
        # TODO: Getdeps' show-inst-dir has issues. This needs to be
        # investigated and fixed. Until then, use a workaround using
        # regex matching for the directory name.
        if self.scratch_path is not None:
            scratch_path = self.scratch_path
            scratch_path_installed_dir = os.path.join(scratch_path, location, name)
            target_installed_dirs = glob.glob(scratch_path_installed_dir + "*")
            return target_installed_dirs

        get_install_dir_cmd = [PackageFboss.GETDEPS, "show-inst-dir", name]
        if location == PackageFboss.BUILD:
            get_install_dir_cmd = [PackageFboss.GETDEPS, "show-build-dir", name]
        return (
            subprocess.check_output(get_install_dir_cmd)
            .decode("utf-8")
            .strip()
            .split("\n")[-1]
        )

    def _copy_run_scripts(self, tmp_dir_name):
        run_scripts_path = os.path.join(self.git_path, "fboss/oss/scripts/run_scripts")
        src_files = os.listdir(run_scripts_path)
        for file_name in src_files:
            full_file_name = os.path.join(run_scripts_path, file_name)
            script_pkg_path = os.path.join(tmp_dir_name, PackageFboss.BIN)
            print(f"Copying {full_file_name} to {script_pkg_path}")
            shutil.copy(full_file_name, script_pkg_path)

    def _copy_run_configs(self, tmp_dir_name):
        run_configs_path = os.path.join(self.git_path, "fboss/oss/scripts/run_configs")
        src_files = os.listdir(run_configs_path)
        for file_name in src_files:
            full_file_name = os.path.join(run_configs_path, file_name)
            config_path = os.path.join(tmp_dir_name, PackageFboss.DATA)
            full_config_name = os.path.join(config_path, file_name)
            print(f"Copying {full_file_name} to {full_config_name}")
            shutil.copytree(full_file_name, full_config_name)

    def _copy_configs(self, tmp_dir_name):
        hw_test_configs_path = os.path.join(self.git_path, "fboss/oss/hw_test_configs")
        qsfp_test_configs_path = os.path.join(
            self.git_path, "fboss/oss/qsfp_test_configs"
        )
        link_test_configs_path = os.path.join(
            self.git_path, "fboss/oss/link_test_configs"
        )
        hw_sanity_test_configs_path = os.path.join(
            self.git_path, "fboss/oss/hw_sanity_tests"
        )
        print(f"Copying {hw_test_configs_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/hw_test_configs",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "hw_test_configs"),
        )
        print(f"Copying {qsfp_test_configs_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/qsfp_test_configs",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "qsfp_test_configs"),
        )
        print(f"Copying {link_test_configs_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/link_test_configs",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "link_test_configs"),
        )
        print(f"Copying {hw_sanity_test_configs_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/hw_sanity_tests",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "hw_sanity_tests"),
        )

    def _copy_production_features(self, tmp_dir_name):
        production_features_path = os.path.join(
            self.git_path, "fboss/oss/production_features"
        )
        print(f"Copying {production_features_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/production_features",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "production_features"),
        )

    def _copy_known_bad_tests(self, tmp_dir_name):
        hw_known_bad_tests_path = os.path.join(
            self.git_path, "fboss/oss/hw_known_bad_tests"
        )
        qsfp_known_bad_tests_path = os.path.join(
            self.git_path, "fboss/oss/qsfp_known_bad_tests"
        )
        link_known_bad_tests_path = os.path.join(
            self.git_path, "fboss/oss/link_known_bad_tests"
        )
        print(f"Copying {hw_known_bad_tests_path} to {tmp_dir_name}")
        print(f"Copying {qsfp_known_bad_tests_path} to {tmp_dir_name}")
        print(f"Copying {link_known_bad_tests_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/hw_known_bad_tests",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "hw_known_bad_tests"),
        )
        shutil.copytree(
            "fboss/oss/qsfp_known_bad_tests",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "qsfp_known_bad_tests"),
        )
        shutil.copytree(
            "fboss/oss/link_known_bad_tests",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "link_known_bad_tests"),
        )

    def _copy_unsupported_tests(self, tmp_dir_name):
        qsfp_unsupported_tests_path = os.path.join(
            self.git_path, "fboss/oss/qsfp_unsupported_tests"
        )
        print(f"Copying {qsfp_unsupported_tests_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/oss/qsfp_unsupported_tests",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "qsfp_unsupported_tests"),
        )

    def _copy_binaries(self, tmp_dir_name):
        print("Copying binaries...")

        for name, metadata in list(PackageFboss.NAME_TO_EXECUTABLES.items()):
            executable_type, executables, location = metadata
            installed_dirs = self._get_dir_for(name, location)
            for installed_dir in installed_dirs:
                bin_pkg_path = os.path.join(tmp_dir_name, executable_type)
                # Get the matching executable_type dir in the installed dir
                if name == PackageFboss.FBOSS:
                    # FBOSS binaries are specifically located under {scratch}/build/fboss/...
                    executable_path = glob.glob(installed_dir + "*")[0]
                else:
                    # Shared libraries are located under {scratch}/installed/...
                    executable_path = glob.glob(
                        os.path.join(installed_dir, executable_type) + "*"
                    )[0]
                # If module does not have executables listed, then copy all
                if not executables:
                    executables = os.listdir(executable_path)

                for e in executables:
                    abs_path = os.path.join(executable_path, e)
                    print(f"Copying {abs_path} to {bin_pkg_path}")
                    try:
                        shutil.copy(abs_path, bin_pkg_path)
                    except OSError:
                        print("Skipping non-existent " + abs_path)

        self._copy_run_scripts(tmp_dir_name)
        self._copy_run_configs(tmp_dir_name)
        self._copy_configs(tmp_dir_name)
        self._copy_known_bad_tests(tmp_dir_name)
        self._copy_unsupported_tests(tmp_dir_name)
        self._copy_production_features(tmp_dir_name)

    def _compress_binaries(self):
        print("Compressing FBOSS Binaries...")
        tar_path = os.path.join(args.scratch_path, PackageFboss.FBOSS_BIN_TAR)
        compressed_file = shutil.make_archive(tar_path, "gztar", self.tmp_dir_name)
        subprocess.run(
            ["tar", "-cvf", tar_path, "--zstd", "-C", self.tmp_dir_name, "."]
        )
        print(f"Compressed to {tar_path}")

    def run(self, args):
        self._copy_binaries(self.tmp_dir_name)
        if args.compress:
            self._compress_binaries()


if __name__ == "__main__":
    args = parse_args()
    PackageFboss().run(args)
