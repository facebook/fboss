# pyre-strict
from typing import List, Optional

import neteng.fboss.asic_config_v2.ttypes as asic_config_thrift

import yaml
from fboss.lib.asic_config_v2.bcmxgs import (
    BCM_CFG_COMMON,
    BcmXgsAsicConfig,
    SAI_CFG_COMMON,
)

TABLE_NAMES = [
    "PC_PM_CORE",
    "PC_PORT_PHYS_MAP",
    "PC_PORT",
    "PORT_CONFIG",
    "global",
    "port",
    "TM_THD_CONFIG",
    "PORT",
    "DEVICE_CONFIG",
    "FP_CONFIG",
    "CTR_EFLEX_CONFIG",
]

TH6_SAI_CFG = {
    "rx_cosq_mapping_management_mode": "0",
    "sai_field_group_auto_prioritize": "1",
    "bcm_tunnel_term_compatible_mode": "1",
    # Overwrite l3 ingress reservation config for SAI TH4.
    # This shouldn't be skipped for SAI TH4.
    "l3_iif_reservation_skip": "0",
    "sai_l2_cpu_fdb_event_suppress": "1",
    "sai_port_phy_time_sync_en": "1",
    "sai_stats_support_mask": "0x2",
    "sai_disable_internal_port_serdes": "1",
}


class Tomahawk6AsicConfig(BcmXgsAsicConfig):
    MMU_SIZE = 9416

    # mgmt port settings taken from broadcom/celestica example
    MGMT_PORT_LOGICAL_ID = 76
    MGMT_PORT_PHYSICAL_ID = 513
    MGMT_PORT_SPEED = 100000
    MGMT_PORT_NUM_LANES = 4
    MGMT_PORT_FEC = "PC_FEC_RS528"

    def __init__(
        self, asic_config_params: asic_config_thrift.AsicConfigParameters
    ) -> None:
        super(Tomahawk6AsicConfig, self).__init__(asic_config_params)
        for table_name in TABLE_NAMES:
            self.values[table_name] = {}
        self.num_ports_per_core: int = 0

    def generate_lane_map(self) -> None:
        static_mapping = self.get_static_mapping()
        lane_maps = static_mapping.phy_lane_map
        for core_id, lane_info in lane_maps.items():
            tx_lane_info, rx_lane_info = lane_info.tx_lane_info, lane_info.rx_lane_info
            pm_key = ("PC_PM_ID: {}".format(core_id + 1), "CORE_INDEX: 0")
            pm_value = (
                "RX_LANE_MAP: {}".format(self.get_lane_map_str(list(rx_lane_info))),
                "RX_LANE_MAP_AUTO: 0",
                "TX_LANE_MAP: {}".format(self.get_lane_map_str(list(tx_lane_info))),
                "TX_LANE_MAP_AUTO: 0",
            )
            pm_value = self.values["PC_PM_CORE"].get(pm_key, ()) + pm_value
            self.values["PC_PM_CORE"][pm_key] = pm_value

    def generate_polarity_map(self) -> None:
        static_mapping = self.get_static_mapping()
        pn_swap_maps = static_mapping.polarity_swap_map
        for core_id, swap_map in pn_swap_maps.items():
            tx_swap_map, rx_swap_map = swap_map.tx_lane_info, swap_map.rx_lane_info
            pm_key = ("PC_PM_ID: {}".format(core_id + 1), "CORE_INDEX: 0")
            pm_value = (
                "RX_POLARITY_FLIP: {}".format(
                    self.get_polarity_map_str(list(rx_swap_map))
                ),
                "RX_POLARITY_FLIP_AUTO: 0",
                "TX_POLARITY_FLIP: {}".format(
                    self.get_polarity_map_str(list(tx_swap_map))
                ),
                "TX_POLARITY_FLIP_AUTO: 0",
            )
            pm_value = self.values["PC_PM_CORE"].get(pm_key, ()) + pm_value
            self.values["PC_PM_CORE"][pm_key] = pm_value

    def generate_yaml_string(self, preamble: Optional[str] = None) -> str:
        if (
            self.asic_config_params.configType
            != asic_config_thrift.AsicConfigType.YAML_CONFIG
        ):
            raise Exception("This Bcm config does not use yaml config.")
        unit = 0
        tables = []
        for table_name in TABLE_NAMES:
            if (
                table_name.startswith("PC_")
                or table_name.startswith("PORT_CONFIG")
                or table_name.startswith("TM_THD_CONFIG")
                or table_name.startswith("PORT")
                or table_name.startswith("DEVICE_CONFIG")
                or table_name.startswith("FP_CONFIG")
                or table_name.startswith("CTR_EFLEX_CONFIG")
            ):
                # device configuration
                device = "device"
            else:
                # BAE configuration
                device = "bcm_device"

            # No need to create such table if it's empty
            if table_name in self.values and self.values[table_name]:
                tables.append({device: {unit: {table_name: self.values[table_name]}}})
        # need to remove some special characters
        return (
            yaml.dump_all(
                tables, sort_keys=False, explicit_start=True, explicit_end=True
            )
            .replace(" !!python/tuple", "")
            .replace("- ", "  ")
            .replace("'", "")
        )

    def generate_global_settings(self) -> None:
        # 0: logical table PORT_SYSTEM_PROFILE in duplicated mode with size 68
        # 1: logical table PORT_SYSTEM_PROFILE in unique mode with size 68*4
        # We need large table size, otherwise APIs like bcm_port_untagged_vlan_set()
        # or bcm_port_vlan_member_set() might complain table full.
        self.values["PORT_CONFIG"]["PORT_SYSTEM_PROFILE_OPERMODE_PIPEUNIQUE"] = 1

        # "l3_alpm_tempalte" is equivalent to "l3_alpm_enable" in SDK6.
        # 1: combined mode, 2: parallel mode
        self.values["global"]["l3_alpm_template"] = 1
        # enable hit mode for the ease of debugging
        self.values["global"]["l3_alpm_hit_mode"] = 1
        self.values["global"]["ipv6_lpm_128b_enable"] = 1
        # enable pktio
        self.values["global"]["pktio_driver_type"] = 1
        # avoid getting ghost egress mpls qos map entries
        self.values["global"]["qos_map_multi_get_mode"] = 1
        # use priority as entry key when programming rx cosq mapping
        self.values["global"]["rx_cosq_mapping_management_mode"] = 1
        # Skip l3 ingress interface reservation
        # This is applicable only for native SDK, for SAI this config
        # will be overwritten in SaiBcmConfig in bcm.cinc
        self.values["global"]["l3_iif_reservation_skip"] = 1

        for attribute, value in BCM_CFG_COMMON.items():
            self.values["global"][attribute] = value
        for attribute, value in SAI_CFG_COMMON.items():
            self.values["global"][attribute] = value
        for attribute, value in TH6_SAI_CFG.items():
            self.values["global"][attribute] = value

        # Set MMU lossy mode by default
        if not self.asic_config_params.mmuLossless:
            self.values["TM_THD_CONFIG"]["THRESHOLD_MODE"] = "LOSSY"
        else:
            self.values["TM_THD_CONFIG"]["THRESHOLD_MODE"] = "LOSSY_AND_LOSSLESS"
            self.values["TM_THD_CONFIG"]["SKIP_BUFFER_RESERVATION"] = 1

        if self.asic_config_params.exactMatch:
            # 0: Single width (default).
            # 1: Minimum supported width.
            self.values["global"]["fpem_mem_entries_width"] = 1
            self.values["global"]["fpem_mem_entries"] = 65536

        # required by ACL feature to support qualifier bcmFieldQualifySystemPortBitmap
        self.values["FP_CONFIG"]["FP_ING_OPERMODE"] = "GLOBAL_PIPE_AWARE"

    def get_logical_port_to_physical_port_mapping(self) -> List[List[int]]:
        NUM_LOGICAL_PORTS_PER_DATAPATH = 10
        NUM_PHYSICAL_PORTS_PER_DATAPATH = 16
        NUM_CORES = 64
        NUM_LP_PORTS_ON_EVEN_CORE = 8
        NUM_LANES_PER_CORE = 8
        logical_to_physical_port_mapping = []
        num_lanes_per_port = NUM_LANES_PER_CORE // self.num_ports_per_core
        # TH6 share the similar archtecture with TH5, so most of the following
        # statements should apply to TH6
        # TH5 has 64 PM cores and each core has 8 lanes. Upto 10 logical ports
        # can be created for 2 PM cores (even and odd). For eg, for PM cores
        # 0 and 1, we can create upto 10 logical ports with 5 on each core or
        # 9 in core 0 and 1 in core 1. We have picked 8 logical ports in even
        # core and 2 in odd core.
        # core 0 will have 8 logical ports
        # core 1 will have 2 logical ports
        # core 2 will have 8 logical ports
        # core 3 will have 2 logical ports and so on.
        #
        # Reason for picking 8 ports in even core and 2 in odd core is to have
        # support 8 * 100G breakout on one of the core. Currently, bcm config
        # has 4 logical ports - 2 from each PM core and all will be configured
        # as 400G.
        # For eg:
        # PM core 2: Logical port 11 and 15
        # PM core 3: Logical port 19 and 20
        # PM core 4: Logical port 22 and 26
        # PM core 5: Logical port 30 and 31 and so on.
        #
        # This provides the flexibility of breaking down logical ports in even
        # cores into 8 * 100G.
        #
        # core_offset:
        # All logical ports begin with multiple of 11 except PM core 0 and 1. This
        # is because CPU port is allocated with logical port 0 and hence the logical
        # port for front panel begins from 1 through 10 for PM core 0 and 1.
        # Hence, use the core offset for logical port computation.
        #
        # lp_start_offset:
        # As mentioned above, 8 logical ports are used for even core and 2 logical
        # ports are used for odd core. Add an offset of 8 when the core is odd for
        # logical port computation.
        #
        # lp_start:
        # Based on TH5 spec, all logical ports for even core begin with a multiple
        # of 11 (except PM core 0 and 1). Identify the start of the logical port
        # offset based on core number (even or odd) and add the right offsets.
        # For eg:
        # PM core 2: core_offset = 0, lp_start_offset = 0, lp_start = 11, lp_id = 11
        # PM core 2: core_offset = 0, lp_start_offset = 0, lp_start = 11, lp_id = 15
        # PM core 3: core_offset = 0, lp_start_offset = 8, lp_start = 19, lp_id = 19
        # PM core 3: core_offset = 0, lp_start_offset = 8, lp_start = 20, lp_id = 20
        #
        # pp_start_offset:
        # Physical lane numbers are evenly distributed with 8 lanes per PM core.
        # Even cores will have pp_start_offset of 0 whereas odd cores will have
        # offset of 8.
        #
        # pp_start:
        # Provides the start physical port number for every core.
        # PM Core 2: lp_id = 11, pp_start = 17, pp_id = 17
        # PM Core 2: lp_id = 15, pp_start = 17, pp_id = 21
        # PM Core 3: lp_id = 19, pp_start = 25, pp_id = 25
        # PM Core 3: lp_id = 20, pp_start = 25, pp_id = 29

        for core_num in range(NUM_CORES):
            core_offset = 1 if core_num <= 1 else 0
            lp_start_offset = 0 if core_num % 2 == 0 else NUM_LP_PORTS_ON_EVEN_CORE
            lp_start = (
                (core_num // 2) * (NUM_LOGICAL_PORTS_PER_DATAPATH + 1)
                + lp_start_offset
                + core_offset
            )
            pp_start_offset = 0 if core_num % 2 == 0 else NUM_LANES_PER_CORE
            pp_start = (
                ((core_num // 2) * NUM_PHYSICAL_PORTS_PER_DATAPATH)
                + 1
                + pp_start_offset
            )

            for i in range(self.num_ports_per_core):
                lp_offset = i * (num_lanes_per_port if core_num % 2 == 0 else 1)
                pp_offset = i * num_lanes_per_port
                lp_id = lp_start + lp_offset
                pp_id = pp_start + pp_offset
                logical_to_physical_port_mapping.append([lp_id, pp_id])

        return logical_to_physical_port_mapping

    def get_cpu_logical_port_to_physical_port_mapping(self) -> List[int]:
        return [0, 0]

    def generate_logical_port_to_physical_port_mapping(
        self, mgmt_port: bool = False
    ) -> None:
        lp_mapping = self.get_logical_port_to_physical_port_mapping()
        lp_mapping.append(self.get_cpu_logical_port_to_physical_port_mapping())
        for lp_map in lp_mapping:
            pm_key = ("PORT_ID: {}".format(lp_map[0]),)
            pm_value = ("PC_PHYS_PORT_ID: {}".format(lp_map[1]),)
            self.values["PC_PORT_PHYS_MAP"][pm_key] = pm_value
        if mgmt_port:
            pm_key = ("PORT_ID: {}".format(self.MGMT_PORT_LOGICAL_ID),)
            pm_value = ("PC_PHYS_PORT_ID: {}".format(self.MGMT_PORT_PHYSICAL_ID),)
            self.values["PC_PORT_PHYS_MAP"][pm_key] = pm_value

    def generate_autoload_board_settings(self) -> None:
        self.values["DEVICE_CONFIG"]["AUTOLOAD_BOARD_SETTINGS"] = 0

    def generate_flex_counter_settings(self) -> None:
        # for debugging purpose
        self.values["global"]["global_flexctr_ing_action_num_reserved"] = 20
        self.values["global"]["global_flexctr_ing_pool_num_reserved"] = 8
        self.values["global"]["global_flexctr_ing_op_profile_num_reserved"] = 20
        self.values["global"]["global_flexctr_ing_group_num_reserved"] = 2
        self.values["global"]["global_flexctr_egr_action_num_reserved"] = 8
        self.values["global"]["global_flexctr_egr_pool_num_reserved"] = 5
        self.values["global"]["global_flexctr_egr_op_profile_num_reserved"] = 10
        self.values["global"]["global_flexctr_egr_group_num_reserved"] = 1
        self.values["CTR_EFLEX_CONFIG"]["CTR_ING_EFLEX_OPERMODE_PIPEUNIQUE"] = 1
        self.values["CTR_EFLEX_CONFIG"][
            "CTR_ING_EFLEX_OPERMODE_PIPE_INSTANCE_UNIQUE"
        ] = 1
        self.values["CTR_EFLEX_CONFIG"]["CTR_EGR_EFLEX_OPERMODE_PIPEUNIQUE"] = 1
        self.values["CTR_EFLEX_CONFIG"][
            "CTR_EGR_EFLEX_OPERMODE_PIPE_INSTANCE_UNIQUE"
        ] = 1

    def generate_asic_vendor_config(self) -> None:
        asic_vendor_config = self.get_asic_vendor_config()
        for (
            key,
            value,
        ) in asic_vendor_config.get_asic_vendor_common_config().items():
            self.values[key] = value

        config = asic_vendor_config.get_asic_vendor_config(
            # pyre-ignore
            self.asic_config_params.configGenType
        )
        for key, value in config.items():
            self.values["global"][key] = value
