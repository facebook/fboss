#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import glob
import os
import shutil
import subprocess
import tarfile
import tempfile

import git


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
    return parser.parse_args()


class PackageFboss:
    SCRIPT_DIR_ABS = os.path.dirname(os.path.realpath(__file__))

    BIN = "bin"
    LIB = "lib"
    LIB64 = "lib64"
    DATA = "share"
    MODULES = "lib/modules"

    GETDEPS = "build/fbcode_builder/getdeps.py"

    DEVTOOLS_LIBRARY_PATH = "/opt/rh/devtoolset-8/root/usr/lib64"

    NAME_TO_EXECUTABLES = {
        "fboss": (BIN, []),
        "gflags": (LIB, []),
        "glog": (LIB64, []),
        "libevent": (LIB, []),
        "libsodium": (LIB, []),
        "python-ld": (LIB, []),
    }

    def __init__(self):

        self.scratch_path = args.scratch_path
        self.tmp_dir_name = tempfile.mkdtemp(
            prefix=PackageFboss.FBOSS_BINS, dir=args.scratch_path
        )
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.BIN))
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.LIB))
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.LIB64))
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.MODULES))
        os.makedirs(os.path.join(self.tmp_dir_name, PackageFboss.DATA))

    def _get_install_dir_for(self, name):
        if self.scratch_path is not None:
            scratch_path = self.scratch_path
            scratch_path_installed_dir = scratch_path + "/" + "installed/"
            target_installed_dir = glob.glob(scratch_path_installed_dir + name + "*")
            return target_installed_dir

        get_install_dir_cmd = [PackageFboss.GETDEPS, "show-inst-dir", name]
        return (
            subprocess.check_output(get_install_dir_cmd)
            .decode("utf-8")
            .strip()
            .split("\n")[-1]
        )

    def _get_git_root(self, path):
        git_repo = git.Repo(path, search_parent_directories=True)
        return git_repo.git.rev_parse("--show-toplevel")

    def _copy_run_scripts(self, tmp_dir_name):
        run_scripts_path = os.path.join(PackageFboss.SCRIPT_DIR_ABS, "run_scripts")
        src_files = os.listdir(run_scripts_path)
        for file_name in src_files:
            full_file_name = os.path.join(run_scripts_path, file_name)
            script_pkg_path = os.path.join(tmp_dir_name, PackageFboss.BIN)
            print(f"Copying {full_file_name} to {script_pkg_path}")
            shutil.copy(full_file_name, script_pkg_path)

    def _copy_run_configs(self, tmp_dir_name):
        run_configs_path = os.path.join(PackageFboss.SCRIPT_DIR_ABS, "run_configs")
        src_files = os.listdir(run_configs_path)
        for file_name in src_files:
            full_file_name = os.path.join(run_configs_path, file_name)
            config_path = os.path.join(tmp_dir_name, PackageFboss.DATA)
            full_config_name = os.path.join(config_path, file_name)
            print(f"Copying {full_file_name} to {full_config_name}")
            shutil.copytree(full_file_name, full_config_name)

    # TODO Future work planned
    # - All tests should consume /etc/coop/agent.conf where sw config block could
    #   be empty
    # - fboss/bcm_sai_configs/* to be replaced with fboss/agent_configs/* where for each
    #   config under agent_config has sw config block empty
    # - package-fboss.py, and helper scripts would need to be modified to work with
    #   agent_configs
    def _copy_configs(self, tmp_dir_name):
        bcm_sai_configs_path = os.path.join(
            self._get_git_root(__file__), "fboss/bcm_sai_configs"
        )
        print(f"Copying {bcm_sai_configs_path} to {tmp_dir_name}")
        shutil.copytree(
            "fboss/bcm_sai_configs",
            os.path.join(tmp_dir_name, PackageFboss.DATA, "bcm_sai_configs"),
        )

    def _copy_binaries(self, tmp_dir_name):
        print(f"Copying binaries...")

        for name, exec_type_and_execs in list(PackageFboss.NAME_TO_EXECUTABLES.items()):
            executable_path = self._get_install_dir_for(name)
            executable_type, executables = exec_type_and_execs
            bin_pkg_path = os.path.join(tmp_dir_name, executable_type)
            # If module does not have executables listed, then copy all
            if not executables:
                executables = os.listdir(os.path.join(executable_path, executable_type))

            for e in executables:
                abs_path = os.path.join(executable_path, executable_type, e)
                print(f"Copying {abs_path} to {bin_pkg_path}")
                try:
                    shutil.copy(abs_path, bin_pkg_path)
                except IOError:
                    print("Skipping non-existent " + abs_path)

        self._copy_run_scripts(tmp_dir_name)
        self._copy_run_configs(tmp_dir_name)
        self._copy_configs(tmp_dir_name)

    def run(self, args):
        self._copy_binaries(self.tmp_dir_name)


if __name__ == "__main__":
    args = parse_args()
    PackageFboss().run(args)
