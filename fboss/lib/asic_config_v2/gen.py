# pyre-strict

import os
import sys
from typing import Dict

import neteng.fboss.asic_config_v2.thrift_types as asic_config_thrift
import yaml

from fboss.lib.asic_config_v2.asic_config import AsicConfig
from fboss.lib.asic_config_v2.icecube800bc import gen_icecube800bc_asic_config

_PLATFORM_TO_ASIC_CONFIG: Dict[str, AsicConfig] = {
    "icecube800bc": gen_icecube800bc_asic_config()
}

_FBOSS_DIR: str = os.getcwd() + "/fboss"
OUTPUT_DIR: str = f"{_FBOSS_DIR}/lib/asic_config_v2/generated_asic_configs/"


def generate_all_asic_configs() -> None:
    for platform, asic_config in _PLATFORM_TO_ASIC_CONFIG.items():
        is_yaml_config: bool = (
            asic_config.asic_config_params.configType
            == asic_config_thrift.AsicConfigType.YAML_CONFIG
        )

        output_dir = os.path.expanduser(OUTPUT_DIR)
        os.makedirs(output_dir, exist_ok=True)

        if is_yaml_config:
            # pyre-ignore
            file_contents = asic_config.generate_yaml_string()
            print(
                f"Writing asic config for {platform}...",
                file=sys.stderr,
            )
            output_path = os.path.expanduser(f"{output_dir}/{platform}.yml")
            with open(output_path, "w", encoding="utf-8") as file:
                file.write(file_contents)


if __name__ == "__main__":
    generate_all_asic_configs()
