import argparse
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

import pkg_resources
import pytest
from dataclasses_json import dataclass_json

from fboss.platform.bsp_tests.utils.cdev_types import FpgaSpec, spi_dev_info
from fboss.platform.bsp_tests.utils.cmd_utils import check_cmd


@dataclass_json
@dataclass
class Config:
    platform: str
    kmods: List[str]
    fpgas: List[FpgaSpec]


CONFIG: Optional[Config] = None

PLATFORMS = ["meru800bia", "meru800bfa"]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", type=str)
    parser.add_argument("--config-file", type=str)
    parser.add_argument("pytest_args", nargs=argparse.REMAINDER)

    return parser.parse_args()


def set_config(args):
    global CONFIG
    if args.config_file:
        with open(Path(args.config_file), "r") as f:
            json_string = f.read()
        CONFIG = Config.from_json(json_string)
        return

    if args.platform not in PLATFORMS:
        raise Exception(
            f"Unknown platform '{args.platform}'. Available platforms are {PLATFORMS}"
        )

    # Use config file from the directory
    json_string = pkg_resources.resource_string(
        __name__, f"configs/{args.platform}.json"
    ).decode("utf-8")
    CONFIG = Config.from_json(json_string)
    return


def get_config():
    if CONFIG is None:
        raise Exception("No config has been set")
    return CONFIG


class TestBase:
    kmods: List[str] = []
    fpgas: List[FpgaSpec] = []

    # Map of fpga to /sys/bus/pci directory
    fpgaToDir: Dict[FpgaSpec, str] = {}

    @classmethod
    def setup_class(cls):
        cls.config = get_config()
        cls.kmods = cls.config.kmods
        cls.fpgas = cls.config.fpgas
        cls.fpgaToDir = findFpgaDirs(cls.fpgas)

    def load_kmods(self) -> None:
        for kmod in self.kmods:
            check_cmd(["modprobe", kmod])

    def unload_kmods(self) -> None:
        for kmod in reversed(self.kmods):
            check_cmd(["modprobe", "-r", kmod])


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
    set_config(args)

    pytest.main(args.pytest_args[1:] + ["/tmp/bsp_tests/"])


if __name__ == "__main__":
    main()
