from typing import Optional

import neteng.fboss.asic_config_v2.ttypes as asic_config_thrift
import neteng.fboss.platform_mapping_config.ttypes as pm_types
from fboss.lib.asic_config_v2.tomahawk5 import Tomahawk5AsicConfig
from fboss.lib.platform_mapping_v2.asic_vendor_config import AsicVendorConfig
from fboss.lib.platform_mapping_v2.gen import read_all_vendor_data
from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingParser


class MontblancAsicConfig(Tomahawk5AsicConfig):
    def __init__(
        self,
        asic_config_params: asic_config_thrift.AsicConfigParameters,
        mgmt_port_speed: Optional[int] = None,
    ) -> None:
        super(MontblancAsicConfig, self).__init__(asic_config_params)

        self.parser = PlatformMappingParser(read_all_vendor_data(), "montblanc")
        self.num_ports_per_core = 2
        if mgmt_port_speed == 10000:
            self.MGMT_PORT_SPEED: int = mgmt_port_speed
            self.MGMT_PORT_NUM_LANES: int = 1
            self.MGMT_PORT_FEC: str = "PC_FEC_NONE"

    def get_asic_vendor_config(self) -> AsicVendorConfig:
        asic_vendor_config = self.parser.get_asic_vendor_config()
        if not asic_vendor_config:
            raise Exception("No asic vendor config found.")
        return asic_vendor_config

    def get_static_mapping(self) -> pm_types.StaticMapping:
        return self.parser.get_static_mapping().get_static_mapping()

    def generate_port_config(self, mgmt_port: bool = False) -> None:
        self.values["PC_PORT"][("PORT_ID: {}".format(0),)] = (
            "ENABLE: 1",
            "SPEED: 10000",
            "NUM_LANES: 1",
        )
        speed_to_fec = {
            100000: "PC_FEC_RS544",
            200000: "PC_FEC_RS544_2XN",
            400000: "PC_FEC_RS544_2XN",
            800000: "PC_FEC_RS544_2XN",
        }
        speed = 400000
        port_ranges = []
        for lp_map in self.get_logical_port_to_physical_port_mapping():
            port_ranges.append("[{}, {}]".format(lp_map[0], lp_map[0]))
        pc_key = ("PORT_ID: [{}]".format(", ".join(port_ranges)),)
        pc_value = (
            "ENABLE: 0",
            "SPEED: {}".format(speed),
            "NUM_LANES: {}".format(4),
            "FEC_MODE: {}".format(speed_to_fec[speed]),
            "MAX_FRAME_SIZE: {}".format(self.MMU_SIZE),
        )
        self.values["PC_PORT"][pc_key] = pc_value
        if mgmt_port:
            mgmt_port_key = ("PORT_ID: {}".format(self.MGMT_PORT_LOGICAL_ID),)
            mgmt_port_value = (
                "ENABLE: 0",
                "SPEED: {}".format(self.MGMT_PORT_SPEED),
                "NUM_LANES: {}".format(self.MGMT_PORT_NUM_LANES),
                "FEC_MODE: {}".format(self.MGMT_PORT_FEC),
                "MAX_FRAME_SIZE: {}".format(self.MMU_SIZE),
            )
            self.values["PC_PORT"][mgmt_port_key] = mgmt_port_value
            if self.MGMT_PORT_SPEED == 10000:
                mgmt_port_medium_value = (
                    "MEDIUM_TYPE_AUTO: 0",
                    "MEDIUM_TYPE: PC_PHY_MEDIUM_COPPER",
                )
                self.values["PC_PMD_FIRMWARE"][mgmt_port_key] = mgmt_port_medium_value

        self.values["PORT"][pc_key] = (
            "MTU: {}".format(self.MMU_SIZE),
            "MTU_CHECK: 1",
        )

    def generate_low_clock_freq_settings(self) -> None:
        self.values["DEVICE_CONFIG"]["CORE_CLK_FREQ"] = "CLK_1125MHZ"
        self.values["DEVICE_CONFIG"]["PP_CLK_FREQ"] = "CLK_675MHZ"

    def generate_asic_config(self) -> None:
        self.generate_global_settings()
        self.generate_logical_port_to_physical_port_mapping(mgmt_port=True)
        self.generate_lane_map()
        self.generate_polarity_map()
        self.generate_port_config(mgmt_port=True)
        self.generate_autoload_board_settings()
        self.generate_low_clock_freq_settings()
        self.generate_flex_counter_settings()
        self.generate_asic_vendor_config()
        self.generate_dlb_specific_config()


def gen_montblanc_asic_config(
    asic_config_params: asic_config_thrift.AsicConfigParameters,
    mgmt_port_speed: Optional[int] = None,
) -> MontblancAsicConfig:
    cfg = MontblancAsicConfig(asic_config_params, mgmt_port_speed)
    cfg.generate_asic_config()
    return cfg
