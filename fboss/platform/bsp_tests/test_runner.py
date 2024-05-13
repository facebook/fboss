import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional

import pkg_resources
import pytest
from dataclasses_json import dataclass_json

from fboss.platform.bsp_tests.cdev_types import FpgaSpec
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

    @classmethod
    def setup_class(cls):
        cls.config = get_config()
        cls.kmods = cls.config.kmods

    def load_kmods(self) -> None:
        for kmod in self.kmods:
            check_cmd(["modprobe", kmod])


def main():
    args = parse_args()
    set_config(args)

    pytest.main(args.pytest_args[1:] + ["/tmp/bsp_tests/"])


if __name__ == "__main__":
    main()
