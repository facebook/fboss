# pyre-strict

import json
import os
import sys
from typing import Any, Dict

import neteng.fboss.asic_config_v2.ttypes as asic_config_thrift
from fboss.lib.asic_config_v2.all_asic_config_params import all_params
from fboss.lib.asic_config_v2.icecube800bc import gen_icecube800bc_asic_config
from fboss.lib.asic_config_v2.montblanc import gen_montblanc_asic_config
from fboss.lib.asic_config_v2.wedge800bact import gen_wedge800bact_asic_config
from fboss.lib.asic_config_v2.icecube800banw import gen_icecube800banw_asic_config
from neteng.fboss.fboss_common.ttypes import PlatformType
from thrift.util import Serializer

_PLATFORM_TO_ASIC_CONFIG_FUNC: Dict[PlatformType, Any] = {
    PlatformType.PLATFORM_ICECUBE800BANW: gen_icecube800banw_asic_config,
    PlatformType.PLATFORM_ICECUBE800BC: gen_icecube800bc_asic_config,
    PlatformType.PLATFORM_MONTBLANC: gen_montblanc_asic_config,
    PlatformType.PLATFORM_WEDGE800BACT: gen_wedge800bact_asic_config,
}

_FBOSS_DIR: str = os.getcwd() + "/fboss"
OUTPUT_DIR: str = f"{_FBOSS_DIR}/lib/asic_config_v2/generated_asic_configs"

from thrift.protocol import TSimpleJSONProtocol


def generate_all_asic_configs() -> None:
    # Clear out all configs in the output directory
    for filename in os.listdir(OUTPUT_DIR):
        if filename.endswith(".json") or filename.endswith(".yml"):
            os.remove(os.path.join(OUTPUT_DIR, filename))

    # Generate new configs
    for platform, asic_config_map in all_params.items():
        if platform not in _PLATFORM_TO_ASIC_CONFIG_FUNC:
            continue
        # Convert PLATFORM_ICECUBE800BC -> icecube800bc
        platform_str = (
            PlatformType._VALUES_TO_NAMES[platform].replace("PLATFORM_", "").lower()
        )
        asic_config_func = _PLATFORM_TO_ASIC_CONFIG_FUNC[platform]
        for config_name, config_data in asic_config_map.items():
            deepcopy_config_data = config_data.copy()
            preamble = (
                deepcopy_config_data.pop("preamble")
                if "preamble" in config_data
                else ""
            )
            asic_config = asic_config_func(**deepcopy_config_data)
            is_yaml_config: bool = (
                asic_config.asic_config_params.configType
                == asic_config_thrift.AsicConfigType.YAML_CONFIG
            )

            output_dir = os.path.expanduser(OUTPUT_DIR)
            os.makedirs(output_dir, exist_ok=True)

            print(
                f"Writing asic config for {platform_str}...",
                file=sys.stderr,
            )
            if is_yaml_config:
                file_contents = asic_config.generate_yaml_string(preamble=preamble)

                output_path = os.path.expanduser(
                    f"{output_dir}/{platform_str}_{config_name}.yml"
                )
                print(f"Writing to {output_path}", file=sys.stderr)
                with open(output_path, "w", encoding="utf-8") as file:
                    file.write(file_contents)
            else:
                thrift_contents = asic_config.export()
                serialized = Serializer.serialize(
                    TSimpleJSONProtocol.TSimpleJSONProtocolFactory(), thrift_contents
                )
                # Parse the serialized JSON and format it with indentation
                json_data = json.loads(serialized.decode("utf-8"))
                prettified_json = json.dumps(json_data, indent=2)
                output_path = os.path.expanduser(f"{output_dir}/{platform}.json")
                print(f"Writing to {output_path}", file=sys.stderr)
                with open(output_path, "w", encoding="utf-8") as file:
                    file.write(prettified_json)


if __name__ == "__main__":
    generate_all_asic_configs()
