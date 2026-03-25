# pyre-strict

from typing import Any, Dict, List

import neteng.fboss.asic_config_v2.ttypes as asic_config_thrift
from neteng.fboss.fboss_common.ttypes import PlatformType


all_params: Dict[PlatformType, Dict[str, Any]] = {
    PlatformType.PLATFORM_MERU800BIA: {},
    PlatformType.PLATFORM_MERU800BFA: {},
    PlatformType.PLATFORM_MONTBLANC: {
        "base": {
            "asic_config_params": asic_config_thrift.AsicConfigParameters(
                configType=asic_config_thrift.AsicConfigType.YAML_CONFIG,
                exactMatch=False,
                mmuLossless=False,
                configGenType=asic_config_thrift.AsicConfigGenType.DEFAULT,
            ),
        },
    },
    PlatformType.PLATFORM_MORGAN800CC: {},
    PlatformType.PLATFORM_WEDGE400: {},
    PlatformType.PLATFORM_JANGA800BIC: {},
    # chassis is default here
    PlatformType.PLATFORM_TAHAN800BC: {},
    PlatformType.PLATFORM_ICECUBE800BANW: {
        "default": {
            "asic_config_params": asic_config_thrift.AsicConfigParameters(
                configType=asic_config_thrift.AsicConfigType.YAML_CONFIG,
                exactMatch=False,
                mmuLossless=False,
                configGenType=asic_config_thrift.AsicConfigGenType.DEFAULT,
            ),
        }
    },
    PlatformType.PLATFORM_ICECUBE800BC: {
        "default": {
            "asic_config_params": asic_config_thrift.AsicConfigParameters(
                configType=asic_config_thrift.AsicConfigType.YAML_CONFIG,
                exactMatch=False,
                mmuLossless=False,
                configGenType=asic_config_thrift.AsicConfigGenType.DEFAULT,
            ),
        }
    },
    PlatformType.PLATFORM_WEDGE800BACT: {
        "base": {
            "asic_config_params": asic_config_thrift.AsicConfigParameters(
                configType=asic_config_thrift.AsicConfigType.YAML_CONFIG,
                exactMatch=False,
                mmuLossless=False,
                configGenType=asic_config_thrift.AsicConfigGenType.DEFAULT,
            ),
        },
    },
    PlatformType.PLATFORM_MINIPACK3N: {},
}
