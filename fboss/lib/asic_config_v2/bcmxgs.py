# pyre-strict
from typing import List, Optional

import neteng.fboss.asic_config_v2.ttypes as asic_config_thrift
import neteng.fboss.platform_mapping_config.ttypes as pm_types

from fboss.lib.asic_config_v2.bcm import BcmAsicConfig
from fboss.lib.platform_mapping_v2.asic_vendor_config import AsicVendorConfig


# Common FBOSS bcm configs to all platforms
BCM_CFG_COMMON = {
    # Fix the ATOM CPU hung upon PCIe timeout bug. context in D25261964
    "pcie_host_intf_timeout_purge_enable": "0",
    # Fix BcmQosMapTest.BcmAllQosMaps failure. context in D26037426
    "qos_map_multi_get_mode": "1",
    # Fix HwHashTrunk8x3PolarizationTests.FullHalfxFullHalf failure.
    # Context in D40313640.
    "macro_flow_hash_shuffle_random_seed": "34345645",
    "bcm_linkscan_interval": "25000",
}

# Common SAI configs added to all platforms
SAI_CFG_COMMON = {
    "sai_common_hash_crc": "0x1",
    "sai_disable_srcmacqedstmac_ctrl": "0x1",
    "sai_acl_qset_optimization": "0x1",
    "sai_optimized_mmu": "0x1",
    "sai_pkt_rx_custom_cfg": "1",
    "sai_pkt_rx_pkt_size": "16512",
    "sai_pkt_rx_cfg_ppc": "16",
    "sai_async_fdb_nbr_enable": "0x1",
    "sai_pfc_defaults_disable": "0x1",
    "sai_ifp_enable_on_cpu_tx": "0x1",
    "sai_vfp_smac_drop_filter_disable": "1",
    "sai_macro_flow_based_hash": "1",
    "sai_mmu_qgroups_default": "1",
    "sai_dis_ctr_incr_on_port_ln_dn": "0",  # CS00012279422
    "custom_feature_mesh_topology_sync_mode": "1",
    "sai_ecmp_group_members_increment": "1",  # CS00012255302
}


class BcmXgsAsicConfig(BcmAsicConfig):
    def __init__(
        self, asic_config_params: asic_config_thrift.AsicConfigParameters
    ) -> None:
        super(BcmXgsAsicConfig, self).__init__(asic_config_params)

    def get_asic_vendor_config(self) -> AsicVendorConfig:
        raise NotImplementedError("Asic Vendor Config not provided.")

    def get_static_mapping(self) -> pm_types.StaticMapping:
        raise NotImplementedError("Platform Mapping not provided.")

    def generate_yaml_string(self, preamble: Optional[str] = None) -> str:
        raise NotImplementedError("generate_yaml_string() not defined")

    def get_lane_map_str(self, lane_map: List[int]) -> str:
        lane_map_prefix = "0x"
        lane_map_str = ""
        for lane in lane_map:
            lane_map_str += str(lane)
        return lane_map_prefix + lane_map_str[::-1]

    def get_polarity_map_str(self, pn_map: List[int]) -> str:
        pn_map_str = ""
        for pn in pn_map:
            pn_map_str += str(pn)
        pn_map_str = pn_map_str[::-1]
        # polarity maps are binary strings which need to be converted
        # to hex values. For eg, a binary string "01011100" will be
        # converted to 0x5C.
        pn_map_hex = "0x" + "{:02x}".format(int(pn_map_str, 2)).upper()
        return pn_map_hex

    def export(self, preamble: Optional[str] = None) -> asic_config_thrift.AsicConfig:
        configEntry = asic_config_thrift.AsicConfigEntry(
            yamlConfig=self.generate_yaml_string(preamble=preamble)
        )
        return asic_config_thrift.AsicConfig(common=configEntry)
