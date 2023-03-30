#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import os
import platform
import shutil
import subprocess
import sys


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--reload", help="Clear setup config, and reload", action="store_true"
    )
    return parser.parse_args()


class SetupFboss:
    FRUID_CONF = "fruid.json"
    FRUID_DIR_PATH = "/var/facebook/fboss"
    FRUID_FULL_PATH = os.path.join(FRUID_DIR_PATH, FRUID_CONF)

    BDE_CONF = "bde.conf"
    BDE_CONF_FULL_PATH = os.path.join("/etc/modprobe.d", BDE_CONF)

    BCM_CONF = "bcm.conf"
    BCM_CONF_DIR_PATH = "/etc/coop"
    BCM_CONF_FULL_PATH = os.path.join(BCM_CONF_DIR_PATH, BCM_CONF)

    USER_BDE = "linux-user-bde"
    KERNEL_BDE = "linux-kernel-bde"
    USER_BDE_KO = USER_BDE + ".ko"
    KERNEL_BDE_KO = KERNEL_BDE + ".ko"
    KMOD_FULL_PATH = os.path.join("/lib/modules/" + platform.uname().release)
    USER_BDE_KO_FULL_PATH = os.path.join(KMOD_FULL_PATH, USER_BDE_KO)
    KERNEL_BDE_KO_FULL_PATH = os.path.join(KMOD_FULL_PATH, KERNEL_BDE_KO)

    # Unfortunately, lsmod prints _ (underscore) for - (dash)
    LSMOD_USER_BDE = "linux_user_bde"
    LSMOD_KERNEL_BDE = "linux_kernel_bde"

    SRC_USER_BDE_KO_FULL_PATH = os.path.join(os.environ["FBOSS_KMODS"], USER_BDE_KO)
    SRC_KERNEL_BDE_KO_FULL_PATH = os.path.join(os.environ["FBOSS_KMODS"], KERNEL_BDE_KO)

    BCM_CONFIG_DIR = os.path.join(os.environ["FBOSS_DATA"], "bcm_configs")
    TH = "th"
    TH3 = "th3"
    J2CP = "j2cp"

    def __init__(self):
        output = subprocess.check_output(["lspci"]).decode("utf-8").split("\n")

        if [x for x in output if "Broadcom" in x and "BCM56960" in x]:
            self.src_fruid_full_path = os.path.join(
                *[os.environ["FBOSS_DATA"], SetupFboss.TH, SetupFboss.FRUID_CONF]
            )
            self.src_bde_full_path = os.path.join(
                *[os.environ["FBOSS_DATA"], SetupFboss.TH, SetupFboss.BDE_CONF]
            )
            PLATFORM = "WEDGE100S+RSW"
            self.src_bcm_conf_full_path = os.path.join(
                SetupFboss.BCM_CONFIG_DIR, PLATFORM + "-bcm.conf"
            )

        elif [x for x in output if "Broadcom" in x and "b980" in x]:
            self.src_fruid_full_path = os.path.join(
                *[os.environ["FBOSS_DATA"], SetupFboss.TH3, SetupFboss.FRUID_CONF]
            )
            self.src_bde_full_path = os.path.join(
                *[os.environ["FBOSS_DATA"], SetupFboss.TH3, SetupFboss.BDE_CONF]
            )
            PLATFORM = "MINIPACK+FSW"
            self.src_bcm_conf_full_path = os.path.join(
                SetupFboss.BCM_CONFIG_DIR, PLATFORM + "-bcm.conf"
            )

        elif [x for x in output if "Broadcom" in x and "8850" in x]:
            self.src_fruid_full_path = os.path.join(
                *[os.environ["FBOSS_DATA"], SetupFboss.J2CP, SetupFboss.FRUID_CONF]
            )
            self.src_bde_full_path = os.path.join(
                *[os.environ["FBOSS_DATA"], SetupFboss.J2CP, SetupFboss.BDE_CONF]
            )
            PLATFORM = "MERU400BIA"
            self.src_bcm_conf_full_path = os.path.join(
                SetupFboss.BCM_CONFIG_DIR, PLATFORM + "-bcm.conf"
            )

    def _cleanup_old_setup(self):
        if os.path.exists(SetupFboss.FRUID_FULL_PATH):
            os.remove(SetupFboss.FRUID_FULL_PATH)

        if os.path.exists(SetupFboss.BCM_CONF_FULL_PATH):
            os.remove(SetupFboss.BCM_CONF_FULL_PATH)

        if os.path.exists(SetupFboss.BDE_CONF_FULL_PATH):
            os.remove(SetupFboss.BDE_CONF_FULL_PATH)

        subprocess.run(["modprobe", "-r", SetupFboss.USER_BDE])
        subprocess.run(["modprobe", "-r", SetupFboss.KERNEL_BDE])

        if os.path.exists(SetupFboss.USER_BDE_KO_FULL_PATH):
            os.remove(SetupFboss.USER_BDE_KO_FULL_PATH)

        if os.path.exists(SetupFboss.KERNEL_BDE_KO_FULL_PATH):
            os.remove(SetupFboss.KERNEL_BDE_KO_FULL_PATH)

    def _copy_configs(self):
        if not os.path.exists(SetupFboss.FRUID_FULL_PATH):
            if not os.path.exists(SetupFboss.FRUID_DIR_PATH):
                os.makedirs(SetupFboss.FRUID_DIR_PATH)

            shutil.copy(self.src_fruid_full_path, SetupFboss.FRUID_FULL_PATH)

        if not os.path.exists(SetupFboss.BCM_CONF_FULL_PATH):
            if not os.path.exists(SetupFboss.BCM_CONF_DIR_PATH):
                os.mkdir(SetupFboss.BCM_CONF_DIR_PATH)

            shutil.copy(self.src_bcm_conf_full_path, SetupFboss.BCM_CONF_FULL_PATH)

        if not os.path.exists(SetupFboss.BDE_CONF_FULL_PATH):
            shutil.copy(self.src_bde_full_path, SetupFboss.BDE_CONF_FULL_PATH)

    def _link_kmods(self):
        new_kmod = False
        if not os.path.exists(SetupFboss.USER_BDE_KO_FULL_PATH):
            subprocess.run(
                [
                    "ln",
                    "-s",
                    SetupFboss.SRC_USER_BDE_KO_FULL_PATH,
                    "-t",
                    SetupFboss.KMOD_FULL_PATH,
                ]
            )
            new_kmod = True

        if not os.path.exists(SetupFboss.KERNEL_BDE_KO_FULL_PATH):
            subprocess.run(
                [
                    "ln",
                    "-s",
                    SetupFboss.SRC_KERNEL_BDE_KO_FULL_PATH,
                    "-t",
                    SetupFboss.KMOD_FULL_PATH,
                ]
            )
            new_kmod = True

        if new_kmod:
            subprocess.run(["depmod", "-a"])

    def _load_kmods(self):
        output = subprocess.check_output(["lsmod"]).decode("utf-8").split("\n")

        if not [x for x in output if SetupFboss.LSMOD_USER_BDE in x]:
            subprocess.run(["modprobe", SetupFboss.USER_BDE])
        if not [x for x in output if SetupFboss.LSMOD_KERNEL_BDE in x]:
            subprocess.run(["modprobe", SetupFboss.KERNEL_BDE])

    def run(self, args):
        if args.reload:
            self._cleanup_old_setup()

        self._copy_configs()
        self._link_kmods()
        self._load_kmods()


if __name__ == "__main__":
    if "FBOSS" not in os.environ:
        sys.exit(f"FBOSS is unset. Run 'source ./bin/setup_fboss_env' to set it up.")
    else:
        print(f"Running setup.py with FBOSS={os.environ['FBOSS']}")

    args = parse_args()
    SetupFboss().run(args)
