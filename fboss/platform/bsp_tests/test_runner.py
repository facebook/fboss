import argparse
import json
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import List

import pkg_resources
import pytest


@dataclass
class Config:
    platform: str
    kmods: List[str]


CONFIG = None

PLATFORMS = ["meru800bia"]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", type=str)
    parser.add_argument("--config-file", type=str)
    return parser.parse_args()


def set_config(args):
    global CONFIG
    if args.config_file:
        with open(Path(args.config_file), "r") as f:
            data = json.load(f)
        CONFIG = Config(**data)
        return

    if args.platform not in PLATFORMS:
        raise Exception(
            f"Unknown platform '{args.platform}'. Available platforms are {PLATFORMS}"
        )

    # Use config file from the directory
    data = json.loads(
        pkg_resources.resource_string(__name__, f"configs/{args.platform}.json").decode(
            "utf-8"
        )
    )
    CONFIG = Config(**data)
    return


def get_config():
    if CONFIG is None:
        raise Exception("No config has been set")
    return CONFIG


class TestBase:
    @classmethod
    def setup_class(cls):
        cls.config = get_config()

    def run_cmd(self, cmd, **kwargs):
        result = subprocess.run(cmd, **kwargs)
        return result.returncode == 0

    def check_cmd(self, cmd, **kwargs):
        result = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
        )
        ret_code = result.returncode
        stdout = result.stdout.decode("utf-8")
        stderr = result.stderr.decode("utf-8")

        if ret_code != 0:
            raise Exception(
                f"Command failed: `{' '.join(cmd)}`, stdout={stdout}, stderr={stderr}"
            )


def main():
    args = parse_args()
    set_config(args)

    pytest.main(["/tmp/bsp_tests/"])


if __name__ == "__main__":
    main()
