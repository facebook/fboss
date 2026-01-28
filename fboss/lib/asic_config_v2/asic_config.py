# pyre-strict
from typing import Any, Dict

from neteng.fboss.asic_config_v2.ttypes import AsicConfigParameters


class AsicConfig(object):
    def __init__(self, asic_config_params: AsicConfigParameters) -> None:
        self.asic_config_params: AsicConfigParameters = asic_config_params
        self.values: Dict[str, Any] = {}
