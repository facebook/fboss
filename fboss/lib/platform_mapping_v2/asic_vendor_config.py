# pyre-strict
from typing import Dict

from neteng.fboss.asic_config_v2.ttypes import (
    AsicConfigGenType,
    AsicVendorConfigParams,
    MultistageRole,
)


class AsicVendorConfig:
    def __init__(self, asic_vendor_config_params: AsicVendorConfigParams) -> None:
        self._asic_vendor_config_params = asic_vendor_config_params

    def get_asic_vendor_config_params(self) -> AsicVendorConfigParams:
        return self._asic_vendor_config_params

    def get_asic_vendor_common_config(self) -> Dict[str, str]:
        return self._asic_vendor_config_params.commonConfig

    def get_asic_vendor_prod_config(self) -> Dict[str, str]:
        return self._asic_vendor_config_params.prodConfig

    def get_asic_vendor_hw_test_config(self) -> Dict[str, str]:
        return self._asic_vendor_config_params.hwTestConfig

    def get_asic_vendor_link_test_config(self) -> Dict[str, str]:
        return self._asic_vendor_config_params.linkTestConfig

    def get_asic_vendor_benchmark_config(self) -> Dict[str, str]:
        return self._asic_vendor_config_params.benchmarkConfig

    def get_asic_vendor_config(
        self, configGenType: AsicConfigGenType
    ) -> Dict[str, str]:
        if configGenType == AsicConfigGenType.HW_TEST:
            return self.get_asic_vendor_hw_test_config()
        elif configGenType == AsicConfigGenType.LINK_TEST:
            return self.get_asic_vendor_link_test_config()
        elif configGenType == AsicConfigGenType.BENCHMARK:
            return self.get_asic_vendor_benchmark_config()
        else:
            return self.get_asic_vendor_prod_config()

    def get_asic_vendor_port_map_config(self, port_map_type: str) -> str:
        port_map_config = self._asic_vendor_config_params.portMapConfig
        if port_map_type not in port_map_config:
            raise Exception(
                "port map type {} not found in vendor config".format(port_map_type)
            )
        return port_map_config[port_map_type]

    def get_asic_vendor_multistage_config(self, role: MultistageRole) -> str:
        if not role:
            role = MultistageRole.NONE
        multistage_config = self._asic_vendor_config_params.multistageConfig
        role_name = MultistageRole._VALUES_TO_NAMES[role]
        if role_name not in multistage_config:
            raise Exception(
                "multistage role {} not found in vendor config".format(role_name)
            )
        return multistage_config[role_name]
