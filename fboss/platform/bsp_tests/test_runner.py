import argparse
import os
from dataclasses import dataclass
from typing import Dict, List

import pytest
from dataclasses_json import dataclass_json

from fboss.platform.bsp_tests.utils.cdev_types import FpgaSpec


@dataclass_json
@dataclass
class Config:
    platform: str
    kmods: List[str]
    fpgas: List[FpgaSpec]


PLATFORMS = ["meru800bia", "meru800bfa"]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", type=str, default="", choices=PLATFORMS)
    parser.add_argument("--config-file", type=str)
    parser.add_argument("--install-dir", type=str, default="")
    parser.add_argument("--config-subdir", type=str, default="configs")
    parser.add_argument("--tests-subdir", type=str, default="tests")

    # Remainder of args are passed to pytest (e.g. --mark, --collect-only, etc)
    parser.add_argument("pytest_args", nargs=argparse.REMAINDER)

    return parser.parse_args()


class TestBase:
    # Map of fpga to /sys/bus/pci directory
    fpgaToDir: Dict[FpgaSpec, str] = {}


def findFpgaDirs(fpgas: List[FpgaSpec]) -> Dict[str, str]:
    ret: Dict[str, str] = {}
    for fpga in fpgas:
        found = False
        for subdir in os.listdir("/sys/bus/pci/devices/"):
            subdir_path = os.path.join("/sys/bus/pci/devices/", subdir)
            if not os.path.isdir(subdir_path):
                continue
            if check_files_for_fpga(fpga, subdir_path):
                found = True
                ret[fpga.name] = subdir_path
                break
        if not found:
            raise Exception(f"Could not find dir for fpga {fpga}")
    return ret


def check_files_for_fpga(fpga: FpgaSpec, dirPath: str) -> bool:
    fileValues = {
        "vendor": fpga.vendorId,
        "device": fpga.deviceId,
        "subsystem_vendor": fpga.subSystemVendorId,
        "subsystem_device": fpga.subSystemDeviceId,
    }

    for filename, value in fileValues.items():
        file_path = os.path.join(dirPath, filename)
        if not os.path.isfile(file_path):
            return False
        with open(file_path, "r") as f:
            content = f.read().strip()
            if content != value:
                return False
    return True


def main():
    args = parse_args()

    # Some args exposed to pytest for use in fixt res
    pytest_args = [
        os.path.join(args.install_dir, args.tests_subdir)
    ] + args.pytest_args[1:]
    if args.platform:
        pytest_args += [f"--platform={args.platform}"]
    if args.config_file:
        pytest_args += [f"--config_file={args.config_file}"]
    if args.install_dir:
        pytest_args += [f"--install_dir={args.install_dir}"]
    if args.config_subdir:
        pytest_args += [f"--config_subdir={args.config_subdir}"]

    pytest.main(pytest_args)


if __name__ == "__main__":
    main()
