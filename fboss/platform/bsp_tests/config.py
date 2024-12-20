# pyre-strict

import importlib.resources

import json
from dataclasses import dataclass
from typing import Optional

from dataclasses_json import dataclass_json
from fboss.platform.bsp_tests.cdev_types import FpgaSpec
from fboss.platform.platform_manager.platform_manager_config.types import (
    BspKmodsFile,
    PlatformConfig,
)

from thrift.py3 import Protocol, serializer


@dataclass_json
@dataclass
class TestConfig:
    platform: str
    vendor: str
    fpgas: list[FpgaSpec]


@dataclass_json
@dataclass
class RuntimeConfig:
    platform: str
    vendor: str
    fpgas: list[FpgaSpec]
    kmods: BspKmodsFile


class ConfigLib:
    def __init__(self, pm_config_dir: Optional[str]) -> None:
        self.pm_config_dir = pm_config_dir

    def get_platform_manager_config(self, platform_name: str) -> PlatformConfig:
        content: str = ""
        if not self.pm_config_dir:
            for resource in importlib.resources.contents(f"{__package__}"):
                print(resource)
            # Use package resources
            package_name = f"{__package__}.fboss_config_files.{platform_name}"
            with importlib.resources.open_binary(
                package_name, "platform_manager.json"
            ) as file:
                content = file.read().decode("utf-8")
        else:
            with open(
                f"{self.pm_config_dir}/{platform_name}/platform_manager.json"
            ) as file:
                content = file.read()

        return serializer.deserialize(
            PlatformConfig,
            json.dumps(json.loads(content)).encode("utf-8"),
            protocol=Protocol.JSON,
        )

    def build_runtime_config(
        self, test_config: TestConfig, kmods: BspKmodsFile
    ) -> RuntimeConfig:
        _pm_conf = self.get_platform_manager_config(test_config.platform)  # noqa
        runtime_config = RuntimeConfig("", "", [], BspKmodsFile())
        return runtime_config
