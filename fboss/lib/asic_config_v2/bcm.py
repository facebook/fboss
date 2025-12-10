# pyre-strict

from fboss.lib.asic_config_v2.asic_config import AsicConfig
from neteng.fboss.asic_config_v2.ttypes import AsicConfigParameters


class BcmAsicConfig(AsicConfig):
    def __init__(self, asic_config_params: AsicConfigParameters) -> None:
        super(BcmAsicConfig, self).__init__(asic_config_params)
