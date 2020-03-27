#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import os
import shutil
import subprocess
import tarfile
import tempfile

import git


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rpm", help="Builds RPM", action="store_true")
    return parser.parse_args()


class PackageFboss:
    FBOSS_BINS = "fboss_bins-1"
    FBOSS_BINS_SPEC = FBOSS_BINS + ".spec"

    HOME_DIR_ABS = os.path.expanduser("~")

    SCRIPT_DIR_ABS = os.path.dirname(os.path.realpath(__file__))
    RPM_SPEC_ABS = os.path.join(SCRIPT_DIR_ABS, FBOSS_BINS_SPEC)

    BIN = "bin"
    LIB = "lib"
    LIB64 = "lib64"

    GETDEPS = "build/fbcode_builder/getdeps.py"

    DEVTOOLS_LIBRARY_PATH = "/opt/rh/devtoolset-8/root/usr/lib64"

    NAME_TO_EXECUTABLES = {
        "fboss": (BIN, ["wedge_agent", "bcm_test", "sai_test-fake-1.5.0"]),
        "gflags": (LIB, ["libgflags.so.2.2"]),
        "glog": (LIB64, ["libglog.so.0"]),
        "zstd": (LIB64, ["libzstd.so.1.3.8"]),
        "libusb": (LIB, ["libusb-1.0.so.0"]),
        "libnl": (LIB, ["libnl-3.so.200"]),
        "libcurl": (LIB, ["libcurl.so.4"]),
        "libsodium": (LIB, ["libsodium.so.23"]),
        "libmnl": (LIB, ["libmnl.so.0"]),
        "nghttp2": (LIB, ["libnghttp2.so.14"]),
    }

    def __init__(self):
        self.tmp_dir_name = tempfile.mkdtemp(prefix=PackageFboss.FBOSS_BINS)

        self.rpm_dir_abs = os.path.join(PackageFboss.HOME_DIR_ABS, "rpmbuild")
        self.rpm_src_dir_abs = os.path.join(self.rpm_dir_abs, "SOURCES")
        self.rpm_spec_dir_abs = os.path.join(self.rpm_dir_abs, "SPECS")

        self.rpm_src_fboss_dir_abs = os.path.join(
            self.rpm_src_dir_abs, PackageFboss.FBOSS_BINS
        )
        self.rpm_src_fboss_tar_abs = self.rpm_src_fboss_dir_abs + ".tar.gz"

    def _get_install_dir_for(self, name):
        return (
            subprocess.check_output([PackageFboss.GETDEPS, "show-inst-dir", name])
            .decode("utf-8")
            .strip()
            .split("\n")[-1]
        )

    def _get_git_root(self, path):
        git_repo = git.Repo(path, search_parent_directories=True)
        return git_repo.git.rev_parse("--show-toplevel")

    def _copy_run_scripts(self, tmp_dir_name):
        run_scripts_path = os.path.join(PackageFboss.SCRIPT_DIR_ABS, "run_scripts")
        print(f"Copying {run_scripts_path} to {tmp_dir_name}")
        shutil.copytree(run_scripts_path, os.path.join(tmp_dir_name, "run_scripts"))

    def _copy_configs(self, tmp_dir_name):
        bcm_configs_path = os.path.join(
            self._get_git_root(__file__), "fboss/bcm_configs"
        )
        print(f"Copying {bcm_configs_path} to {tmp_dir_name}")
        shutil.copytree("fboss/bcm_configs", os.path.join(tmp_dir_name, "bcm_configs"))

    def _copy_kos(self, tmp_dir_name):
        # If kernel modules are built (e.g. by build-bcm-ko.sh), copy those
        opennsa_base_dir = self._get_install_dir_for("OpenNSA")
        opennsa_ko_dir = os.path.join(
            opennsa_base_dir, "src/gpl-modules/build/linux-x86-smp_generic_64-2_6"
        )
        if os.path.exists(opennsa_ko_dir):
            linux_user_bde_path = os.path.join(opennsa_ko_dir, "linux-user-bde.ko")
            linux_kernel_bde_path = os.path.join(opennsa_ko_dir, "linux-kernel-bde.ko")
            print(f"Copying {linux_user_bde_path} to {tmp_dir_name}")
            shutil.copy(linux_user_bde_path, tmp_dir_name)
            print(f"Copying {linux_kernel_bde_path} to {tmp_dir_name}")
            shutil.copy(linux_kernel_bde_path, tmp_dir_name)

    def _copy_binaries(self, tmp_dir_name):
        print(f"Copying binaries...")

        for name, exec_type_and_execs in list(PackageFboss.NAME_TO_EXECUTABLES.items()):
            executable_path = self._get_install_dir_for(name)
            executable_type, executables = exec_type_and_execs
            for e in executables:
                abs_path = os.path.join(executable_path, executable_type, e)
                print(f"Copying {abs_path} to {tmp_dir_name}")
                shutil.copy(abs_path, tmp_dir_name)

        self._copy_run_scripts(tmp_dir_name)
        self._copy_configs(tmp_dir_name)
        self._copy_kos(tmp_dir_name)

    def _setup_for_rpmbuild(self):
        print(f"Setup for rpmbuild...")
        if os.path.exists(self.rpm_dir_abs):
            shutil.rmtree(self.rpm_dir_abs)
        os.makedirs(self.rpm_dir_abs)
        subprocess.run("rpmdev-setuptree")
        os.makedirs(self.rpm_src_fboss_dir_abs)

    def _prepare_for_build(self):
        print(f"Preparing for build...")
        with tarfile.open(self.rpm_src_fboss_tar_abs, "w:gz") as tar:
            tar.add(
                self.rpm_src_fboss_dir_abs,
                arcname=os.path.basename(self.rpm_src_fboss_dir_abs),
            )

        # package .tar.gz, so this can be removed
        shutil.rmtree(self.rpm_src_fboss_dir_abs)

        # TODO use rpmdev-newspec $FBOSS_BINS to create spec and edit it.
        # For now, copy pre-created SPEC file.
        shutil.copy(PackageFboss.RPM_SPEC_ABS, self.rpm_spec_dir_abs)

    def _build_rpm_helper(self):
        print(f"Building rpm...")
        env = dict(os.environ)
        env["LD_LIBRARY_PATH"] = PackageFboss.DEVTOOLS_LIBRARY_PATH
        subprocess.run(
            ["rpmbuild", "-ba", PackageFboss.FBOSS_BINS_SPEC],
            cwd=self.rpm_spec_dir_abs,
            env=env,
        )

    def _build_rpm(self):
        self._setup_for_rpmbuild()
        self._copy_binaries(self.rpm_src_fboss_dir_abs)
        self._prepare_for_build()
        self._build_rpm_helper()

    def run(self, args):
        if args.rpm:
            print(f"Building RPM...")
            self._build_rpm()
        else:
            self._copy_binaries(self.tmp_dir_name)


if __name__ == "__main__":
    args = parse_args()
    PackageFboss().run(args)
