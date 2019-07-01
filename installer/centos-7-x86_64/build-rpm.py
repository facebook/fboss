#!/usr/bin/env python3

# Copyright 2004-present Facebook. All Rights Reserved.

from __future__ import absolute_import, division, print_function, unicode_literals

import subprocess
import os
import shutil
import tarfile


class BuildRpm:
    FBOSS_BINS = 'fboss_bins-1'
    FBOSS_BINS_SPEC = FBOSS_BINS + '.spec'

    HOME_DIR_ABS = os.path.expanduser('~')

    SCRIPT_DIR_ABS = os.path.dirname(os.path.realpath(__file__))
    RPM_SPEC_ABS = os.path.join(SCRIPT_DIR_ABS, FBOSS_BINS_SPEC)

    BIN = 'bin'
    LIB = 'lib'
    LIB64 = 'lib64'

    GETDEPS = "build/fbcode_builder/getdeps.py"

    DEVTOOLS_LIBRARY_PATH = '/opt/rh/devtoolset-8/root/usr/lib64'

    NAME_TO_EXECUTABLES = {
        'fboss' : (BIN, ['wedge_agent', 'bcm_test']),
        'gflags' : (LIB, ['libgflags.so.2.2']),
        'glog' : (LIB64, ['libglog.so.0']),
        'zstd' : (LIB64, ['libzstd.so.1.3.8']),
        'OpenNSL' : (LIB, ['libopennsl.so.1']),
        'libusb' : (LIB, ['libusb-1.0.so.0']),
        'libnl' : (LIB, ['libnl-3.so.200']),
        'libcurl' : (LIB, ['libcurl.so.4']),
        'libsodium' : (LIB, ['libsodium.so.23']),
        'libmnl' : (LIB, ['libmnl.so.0']),
    }

    def __init__(self):
        self.rpm_dir_abs = os.path.join(BuildRpm.HOME_DIR_ABS, 'rpmbuild')
        self.rpm_src_dir_abs = os.path.join(self.rpm_dir_abs, 'SOURCES')
        self.rpm_spec_dir_abs = os.path.join(self.rpm_dir_abs, 'SPECS')

        self.rpm_src_fboss_dir_abs = os.path.join(
            self.rpm_src_dir_abs, BuildRpm.FBOSS_BINS)
        self.rpm_src_fboss_tar_abs = self.rpm_src_fboss_dir_abs + '.tar.gz'

    def _setup_for_rpmbuild(self):
        print(f"Setup for rpmbuild...")
        if os.path.exists(self.rpm_dir_abs):
            shutil.rmtree(self.rpm_dir_abs)
        os.makedirs(self.rpm_dir_abs)
        subprocess.run('rpmdev-setuptree')
        os.makedirs(self.rpm_src_fboss_dir_abs)

    def _get_install_dir_for(self, name):
        return subprocess.check_output(
            [BuildRpm.GETDEPS, 'show-inst-dir', name]).decode(
                "utf-8").strip().split('\n')[-1]

    def _copy_binaries(self):
        print(f"Copying binaries...")

        for name, exec_type_and_execs in BuildRpm.NAME_TO_EXECUTABLES.items():
            executable_path = self._get_install_dir_for(name)
            executable_type, executables = exec_type_and_execs
            for e in executables:
                abs_path = os.path.join(executable_path, executable_type, e)
                print(f"Copying {abs_path} to {self.rpm_src_fboss_dir_abs}")
                shutil.copy(abs_path, self.rpm_src_fboss_dir_abs)

    def _prepare_for_build(self):
        print(f"Preparing for build...")
        with tarfile.open(self.rpm_src_fboss_tar_abs, "w:gz") as tar:
            tar.add(self.rpm_src_fboss_dir_abs,
                arcname=os.path.basename(self.rpm_src_fboss_dir_abs))

        # package .tar.gz, so this can be removed
        shutil.rmtree(self.rpm_src_fboss_dir_abs)

        # TODO use rpmdev-newspec $FBOSS_BINS to create spec and edit it.
        # For now, copy pre-created SPEC file.
        shutil.copy(BuildRpm.RPM_SPEC_ABS, self.rpm_spec_dir_abs)

    def _build_rpm(self):
        print(f"Building rpm...")
        env = dict(os.environ)
        env['LD_LIBRARY_PATH'] = BuildRpm.DEVTOOLS_LIBRARY_PATH
        subprocess.run(['rpmbuild', '-ba', BuildRpm.FBOSS_BINS_SPEC],
            cwd=self.rpm_spec_dir_abs, env=env)

    def run(self):
        self._setup_for_rpmbuild()
        self._copy_binaries()
        self._prepare_for_build()
        self._build_rpm()


if __name__ == '__main__':
    BuildRpm().run()
