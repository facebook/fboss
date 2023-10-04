#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#
# Login to IAC VM snc-fbossiac801 and open a Docker container shell with the following commands:
# export DOCKER_NAME=FBOSS_CENTOS8
# sudo docker exec -it $DOCKER_NAME bash
#
# Change directory in shell to /var/FBOSS and copy contents to separate file fboss_oss_verifier.py.
# Run the following command: ./fboss_oss_verifier.py.
#
# If a fresh container is used, the following is necessary for this script to work:
# 1. Copy /usr/local/fboss/brcmsim/simulator/th4_b0 from IAC VM snc-fbossiac802 to /home/bcmsim and /usr/local/fboss/brcmsim/simulator/th4_b0 of container
# 2. Copy /home/bcmsim/bcmsim.key to /home/bcmsim/bcmsim/framework/misc/bcmsim.key and run chmod 400 /home/bcmsim/bcmsim/framework/misc/bcmsim.key
# 3. Copy over and build BCM SAI SDK sources in /var/FBOSS
# 4. Copy over P844030024 to /etc/topology.cdf
# 5. Run dnf install ruby --assumeyes
# 6. Run dnf install libnsl --assumeyes
# 7. Copy P832580816 to /var/FBOSS/runner.sh and run chmod +x /var/FBOSS/runner.sh
# 8. Copy https://github.com/facebook/fboss/blob/main/fboss/oss/hw_test_configs/fuji.agent.materialized_JSON to /var/FBOSS/fuji.materialized_JSON
# 9. Copy https://github.com/facebook/fboss/blob/main/fboss/oss/scripts/run_configs/th4/fruid.json to /var/facebook/fboss/fruid.json
#
#
# This script automates verification of stable commits in OSS.

import glob
import os
import shutil
import subprocess

OUTPUT_DIR = "x86-xgsall-bcmsim-deb-static-fboss"
SAI_SDK_VERSION = "SAI_VERSION_10_0_EA_SIM_ODP"
SAI_TEST_BINARY = "sai_test-sai_impl"
CLONE_OSS_REPO_CMD = r"""git clone https://github.com/facebook/fboss fboss.git"""
RUN_BUILD_HELPER_CMD = r"""./build-helper.py {0} {1} {2}"""
VERIFY_STABLE_COMMIT_CMD = r"""source ./setup_fboss_env; ./run_test.py sai --sai-bin {0} --config {1} --mgmt-if eth0 --filter HwVlanTest.VlanApplyConfig --simulator th4_b0 --coldboot_only"""
LIBRARIES = ["gflags", "glog", "libevent", "libsodium", "python"]


class FBOSSOSSVerifier:
    def __init__(self) -> None:
        self._oss_dir = os.getcwd()
        self._bcm_sai_dir = os.path.join(self._oss_dir, "bcm_sai")
        self._built_bcmsim_sai_dir = os.path.join(self._oss_dir, "built-bcmsim-sai")
        self._scratch_dir = os.path.join(self._oss_dir, "tmp_bld_dir")
        self._test_pkg_dir = os.path.join(self._oss_dir, "fboss_oss_verifier")
        self._git_dir = os.path.join(self._oss_dir, "fboss.git")
        self._scripts_dir = os.path.join(self._git_dir, "fboss", "oss", "scripts")
        self._test_config = os.path.join(self._oss_dir, "fuji.materialized_JSON")

    def verify_stable_commit(self):
        os.chdir(self._test_pkg_dir)
        verify_stable_commit_cmd = VERIFY_STABLE_COMMIT_CMD.format(
            f"./{SAI_TEST_BINARY}",
            self._test_config,
        )
        # TODO: Parse output and check for errors
        subprocess.run(
            verify_stable_commit_cmd,
            shell=True,
        )

    def post_build(self):
        os.mkdir(self._test_pkg_dir)
        for lib in LIBRARIES:
            target_lib_dirs = glob.glob(
                os.path.join(self._scratch_dir, "installed", lib) + "*"
            )
            for lib_dir in target_lib_dirs:
                executable_path = glob.glob(os.path.join(lib_dir, "lib") + "*")[0]
                executables = os.listdir(executable_path)
                for executable in executables:
                    src = os.path.join(executable_path, executable)
                    try:
                        shutil.copy(src, "/lib64/")
                    except IOError:
                        print("Skipping non-existent " + src)
        target_bin = glob.glob(
            os.path.join(
                self._scratch_dir, "installed", "fboss", "bin", SAI_TEST_BINARY
            )
            + "*"
        )[0]
        shutil.copy(
            target_bin,
            os.path.join(self._test_pkg_dir, SAI_TEST_BINARY),
        )
        shutil.copy(
            os.path.join(self._scripts_dir, "run_scripts", "setup_fboss_env"),
            os.path.join(self._test_pkg_dir, "setup_fboss_env"),
        )
        shutil.copy(
            os.path.join(self._scripts_dir, "run_scripts", "brcmsim.py"),
            os.path.join(self._test_pkg_dir, "brcmsim.py"),
        )
        shutil.copy(
            os.path.join(self._scripts_dir, "run_scripts", "run_test.py"),
            os.path.join(self._test_pkg_dir, "run_test.py"),
        )
        shutil.copy(
            os.path.join(self._oss_dir, "runner.sh"),
            os.path.join(self._test_pkg_dir, "runner.sh"),
        )

    def build(self):
        os.chdir(os.path.join(self._git_dir, "build", "fbcode_builder"))
        build_fboss_oss_cmd = f'time ./getdeps.py build --allow-system-packages --scratch-path {self._scratch_dir} fboss --extra-cmake-defines \'{{"CMAKE_BUILD_TYPE": "MinSizeRel"}}\''
        # TODO: Handle build failures
        subprocess.run(build_fboss_oss_cmd, shell=True)
        print("FBOSS OSS built successfully")

    def pre_build(self):
        built_bcmsim_sai_experimental_dir = os.path.join(
            self._built_bcmsim_sai_dir, "experimental"
        )
        os.makedirs(built_bcmsim_sai_experimental_dir)
        shutil.copy(
            os.path.join(
                self._bcm_sai_dir,
                "output",
                OUTPUT_DIR,
                "libraries",
                "libsai.a",
            ),
            os.path.join(self._built_bcmsim_sai_dir, "libsai_impl.a"),
        )
        src_experimental_path = os.path.join(
            self._oss_dir, "bcm_sai", "include", "experimental"
        )
        dst_experimental_path = built_bcmsim_sai_experimental_dir
        for file in os.listdir(src_experimental_path):
            src = os.path.join(src_experimental_path, file)
            dst = os.path.join(dst_experimental_path, file)
            if os.path.isdir(src):
                shutil.copytree(src, dst)
            else:
                shutil.copy(src, dst)
        os.chdir(self._scripts_dir)
        run_build_helper_cmd = RUN_BUILD_HELPER_CMD.format(
            os.path.join(self._built_bcmsim_sai_dir, "libsai_impl.a"),
            os.path.join(self._built_bcmsim_sai_dir, "experimental"),
            os.path.join(self._oss_dir, "sai_impl_output"),
        )
        subprocess.run(run_build_helper_cmd, shell=True)
        print("Build helper script ran successfully")

    def clone(self):
        subprocess.run(CLONE_OSS_REPO_CMD, shell=True)
        print("Cloned FBOSS git repo successfully")

    def clean(self):
        if os.path.exists(self._scratch_dir):
            shutil.rmtree(self._scratch_dir)
        if os.path.exists(self._git_dir):
            shutil.rmtree(self._git_dir)
        if os.path.exists(self._built_bcmsim_sai_dir):
            shutil.rmtree(self._built_bcmsim_sai_dir)
        if os.path.exists(self._test_pkg_dir):
            shutil.rmtree(self._test_pkg_dir)

    def setup_env(self):
        env_dict = {
            "SAI_ONLY": "1",
            "SAI_BRCM_IMPL": "1",
            "GETDEPS_USE_WGET": "1",
            "SAI_SDK_VERSION": SAI_SDK_VERSION,
        }
        os.environ.update(env_dict)

    def run(self):
        self.clean()
        self.setup_env()
        self.clone()
        self.pre_build()
        self.build()
        self.post_build()
        self.verify_stable_commit()
        # TODO: Add print_result() step, which should print details such as: fboss commit, date/timestamp,
        # result of build, list of tests to run as part of verification and pass/fail result.


if __name__ == "__main__":
    verifier = FBOSSOSSVerifier()
    verifier.run()
