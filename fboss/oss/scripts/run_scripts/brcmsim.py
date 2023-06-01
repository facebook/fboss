#!/usr/bin/env python

import getopt
import logging
import os
import shutil
import subprocess
import sys
import time

BCM_BASE_DIR = "/home/bcmsim/"
BCM_CXM_PORT = 55555
SCID = 10
SOC_TARGET_PORT = 22222
SOC_BOOT_FLAGS = 0x420000
LOCALHOST = "127.0.0.1"
SIM_SRC_DIR = "/usr/local/fboss/brcmsim/simulator/%s/"
BCMSIM_USER = "root"
ASIC_BCMNAMES = {
    "th3": "BCM56980",
    "th4": "BCM56990",
    "th4_b0": "BCM56990_B0",
    "th5": "BCM78900",
}
ASIC_BCMFILES = {
    "th3": "wedge400_alpm.bcm.conf",
    "th4": "fuji_alpm.yml",
    "th4_b0": "fuji_alpm.yml",
    "th5": "montblanc_alpm.yml",
}

logging.basicConfig(
    level=logging.INFO, format="%(asctime)s %(name)-12s [%(levelname)s] %(message)s"
)
logger = logging.getLogger("bcmsim")


def _run_command(cmd, cwd=None, close_fds=False, wait_to_finish=True):
    proc = subprocess.Popen(
        cmd.split(" "),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,
        close_fds=close_fds,
    )
    if not wait_to_finish:
        return proc
    stdout, stderr = proc.communicate()
    if stderr:
        raise Exception(stderr)
    logger.info(f"Excecuted {cmd}")
    return proc


def setup(asic, bcmsim_path):
    logger.info(f"Set up {asic} BCM SIM environment .. ")
    cmd = "getent group bcmsim"
    proc = _run_command(cmd=cmd, wait_to_finish=False)
    stdout = proc.stdout.readlines()
    if stdout:
        logger.info(f"bcmsim group alrady exists {stdout}.. skip setup")
        return True

    cmd = "groupadd bcmsim"
    _run_command(cmd=cmd)

    cmd = "useradd -m bcmsim -r -p bcmsim -g bcmsim"
    _run_command(cmd=cmd)

    src_dir = SIM_SRC_DIR % asic
    # copy install_bcmsim.sh
    shutil.copy2(os.path.join(src_dir, "install_bcmsim.sh"), bcmsim_path)
    # copy bcmsim.tar.gz
    shutil.copy2(os.path.join(src_dir, "bcmsim.tar.gz"), bcmsim_path)

    src_config_dir = os.path.join(src_dir, "config/")
    install_config_dir = os.path.join(bcmsim_path, "config/")
    shutil.copytree(src_config_dir, install_config_dir)

    cmd = "chmod +x install_bcmsim.sh"
    _run_command(cmd=cmd, cwd=bcmsim_path)

    cmd = "/home/bcmsim/install_bcmsim.sh"
    _run_command(cmd, cwd=bcmsim_path)

    bcmsim_key_path = os.path.join(bcmsim_path, "bcmsim", "framework", "misc")

    if not os.path.exists(bcmsim_key_path):
        logger.info(f"Error as bcm_key_path {bcmsim_key_path} dooesn't exist")
        return False

    # copy bcmsim.key
    shutil.copy2(os.path.join(src_dir, "bcmsim.key"), bcmsim_key_path)

    # validate paths
    fruid_file_path = os.path.join(bcmsim_path, "config", "fruid.json")
    if not os.path.exists(fruid_file_path):
        logger.info(f"Error as fruid_file path doesn't exist {fruid_file_path}")
        return False

    asic_bcmfile = ASIC_BCMFILES[asic]
    bcm_file_path = os.path.join(bcmsim_path, "config", asic_bcmfile)
    if not os.path.exists(bcm_file_path):
        logger.info(f"Error as bcm_file_path path doesn't exist {bcm_file_path}")
        return False

    return True


def launchBcmSimFramework(bcmsim_path):
    logger.info("Launch BCM SIM framework.. (1/2)")
    os.environ["BCMSIM_ROOT"] = os.path.join(bcmsim_path, "bcmsim", "framework")
    os.environ["BCMSIM_CXM_SERVER"] = LOCALHOST
    os.environ["BCMSIM_CXM_HOST"] = LOCALHOST
    os.environ["BCMSIM_CXM_PORT"] = str(BCM_CXM_PORT)
    os.environ["BCMSIM_USER"] = BCMSIM_USER
    os.environ["BCMSIM_WD_ROOT"] = os.path.join(
        bcmsim_path, "bcmsim", "framework", "run"
    )
    os.environ["USER"] = BCMSIM_USER

    bcmsim_root_path = os.getenv("BCMSIM_ROOT")
    bcm_framework_path = os.path.join(bcmsim_root_path, "bin", "bcmsim")

    cmd = f"{bcm_framework_path} -c /etc/topology.cdf"
    proc = _run_command(cmd, close_fds=True, wait_to_finish=False)
    logger.info(f"Excecuted {cmd}")
    return proc


def launchBcmSimLoader(asic, bcmsim_path):
    logger.info("Launch BCM SIM linux loader .. (2/2)")
    os.environ["CXM_HOSTNAME"] = LOCALHOST
    os.environ["CXM_PORT"] = str(BCM_CXM_PORT)
    os.environ["SCID"] = str(SCID)
    os.environ["SOC_TARGET_PORT"] = str(SOC_TARGET_PORT)
    os.environ["SOC_BOOT_FLAGS"] = str(SOC_BOOT_FLAGS)
    os.environ["BCMSIM_USER"] = BCMSIM_USER
    os.environ["BCMSIM_CXM_SERVER"] = LOCALHOST
    os.environ["BCMSIM_CXM_PORT"] = str(BCM_CXM_PORT)

    # new code
    os.environ["BCMSIM_ROOT"] = os.path.join(bcmsim_path, "bcmsim", "framework")

    asic_codename = ASIC_BCMNAMES[asic]
    bcmsim_root_path = os.getenv("BCMSIM_ROOT")
    bcm_loader_path = os.path.join(bcmsim_root_path, "bin", "bcmsim_loader.linux")
    cmd = f"{bcm_loader_path} {asic_codename} {LOCALHOST} {BCM_CXM_PORT} {SCID} {SOC_TARGET_PORT}"
    proc = _run_command(cmd, close_fds=True, wait_to_finish=False)
    logger.info(f"Excecuted {cmd}")
    return proc


def main(argv):
    asic = None
    try:
        opts, args = getopt.getopt(
            argv, "hsa:p:", ["asic=", "skip-setup", "bcmsim-path="]
        )
    except getopt.GetoptError:
        print("brcmsim -a <th3|th4|th4_b0|th5> <-s> <-p path_of_bcmsim>")
        sys.exit(2)
    skip_setup = False
    bcmsim_path = BCM_BASE_DIR
    for opt, arg in opts:
        if opt == "-h":
            print(
                "brcmsim --asic <th3|th4|th4_b0|th5> <--skip-setup> <--bcmsim-path path_of_bcmsim>"
            )
            sys.exit()
        elif opt in ("-a", "--asic"):
            asic = arg
        elif opt in ("-s", "--skip-setup"):
            skip_setup = True
        elif opt in ("-p", "--bcmsim-path"):
            bcmsim_path = arg
    if asic is None or asic not in ["th3", "th4", "th4_b0", "th5"]:
        print("missing asic type, please specify -a <th3|th4|th4_b0|th5>")
        sys.exit()

    if skip_setup or setup(asic, bcmsim_path):
        p1 = launchBcmSimFramework(bcmsim_path)
        # sleep 30s to wait for bcmsim framework fully up
        time.sleep(30)
        p2 = launchBcmSimLoader(asic, bcmsim_path)

        exitCode = p2.wait()
        logger.info(f"launchBcmSimLoader exited with code {exitCode}")

    logger.info("Done with BCM SIM")


if __name__ == "__main__":
    main(sys.argv[1:])
