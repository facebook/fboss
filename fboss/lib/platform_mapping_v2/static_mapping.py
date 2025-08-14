# pyre-strict
from typing import Dict, List, Optional, Set

import neteng.fboss.platform_mapping_config.ttypes as pm_types

TX = "TX"
RX = "RX"


class StaticMapping:
    def __init__(self, az_connections: List[pm_types.ConnectionPair]) -> None:
        # _az_connections represents each row in the Static Mapping CSV
        self._az_connections: List[pm_types.ConnectionPair] = az_connections
        self._a_values: List[pm_types.ConnectionEnd] = [
            connection_pair.a for connection_pair in self._az_connections
        ]

    # Given a connection endpoint, return the other connected end
    def get_other_connection_end(
        self, one_connection_end: pm_types.ConnectionEnd
    ) -> Optional[pm_types.ConnectionEnd]:
        for connection_pair in self._az_connections:
            a, z = connection_pair.a, connection_pair.z
            if a == one_connection_end:
                return z
            if z == one_connection_end:
                return a

        return None

    # Goes through all the a->z connections and returns the unique chips
    def get_chips(self) -> List[pm_types.Chip]:
        unique_chips = []
        unique_chip_strs = set()
        for connection_pair in self._az_connections:
            for connection_end in [connection_pair.a, connection_pair.z]:
                if connection_end and connection_end.chip:
                    chip_str = str(connection_end.chip)
                    if chip_str not in unique_chip_strs:
                        unique_chip_strs.add(chip_str)
                        unique_chips.append(connection_end.chip)

        return unique_chips

    # Given some details about a connection, check and return if the connection exists.
    # The function throws if the connection is not found
    def find_connection_end(
        self,
        slot_id: int,
        chip_id: int,
        chip_types: Set[pm_types.ChipType],
        core_id: int,
        logical_lane_id: int,
    ) -> pm_types.ConnectionEnd:
        for connection_pair in self._az_connections:
            a, z = connection_pair.a, connection_pair.z
            if (
                a.chip.slot_id == slot_id
                and a.chip.chip_id == chip_id
                and a.chip.chip_type in chip_types
                and a.chip.core_id == core_id
                and a.lane.logical_id == logical_lane_id
            ):
                return a
            if (
                z
                and z.chip.slot_id == slot_id
                and z.chip.chip_id == chip_id
                and z.chip.chip_type in chip_types
                and z.chip.core_id == core_id
                and z.lane.logical_id == logical_lane_id
            ):
                return z
        raise Exception(
            f"Can't find a connection for slot_id={slot_id}, chip_id={chip_id}, chip_types={chip_types}, core_id={core_id}, lane_id={logical_lane_id}"
        )

    def _get_phy_lane_map_by_core(
        self, core_id: int, direction: str, sort_lanes: bool = True
    ) -> List[int]:
        lane_dict = {}
        for a in self._a_values:
            if (
                a.chip.core_id == core_id
                and direction == TX
                and a.lane.tx_physical_lane is not None
            ):
                lane_dict[a.lane.logical_id] = a.lane.tx_physical_lane
            elif (
                a.chip.core_id == core_id
                and direction == RX
                and a.lane.rx_physical_lane is not None
            ):
                lane_dict[a.lane.logical_id] = a.lane.rx_physical_lane
            else:
                continue

        lane_map = list(lane_dict.values())
        if sort_lanes:
            sorted_lane_dict = dict(sorted(lane_dict.items()))
            lane_map = list(sorted_lane_dict.values())
        return lane_map

    def gen_phy_lane_map(
        self, sort_lanes: bool = True
    ) -> Dict[int, pm_types.TxRxLaneInfo]:
        core_ids = []
        for a in self._a_values:
            core_ids.append(a.chip.core_id)
        core_ids = set(core_ids)
        lane_map = {}
        for core_id in core_ids:
            tx_lane_map = self._get_phy_lane_map_by_core(core_id, TX, sort_lanes)
            rx_lane_map = self._get_phy_lane_map_by_core(core_id, RX, sort_lanes)
            lane_map[core_id] = pm_types.TxRxLaneInfo(
                tx_lane_info=tx_lane_map, rx_lane_info=rx_lane_map
            )
        return lane_map

    def _get_pn_swap_map_by_core(
        self, core_id: int, direction: str, sort_lanes: bool = True
    ) -> List[int]:
        pn_swap_dict = {}
        for a in self._a_values:
            if a.chip.core_id == core_id and direction == TX:
                pn_swap_dict[a.lane.logical_id] = 1 if a.lane.tx_polarity_swap else 0
            elif a.chip.core_id == core_id and direction == RX:
                pn_swap_dict[a.lane.logical_id] = 1 if a.lane.rx_polarity_swap else 0
            else:
                continue

        pn_swap_map = list(pn_swap_dict.values())
        if sort_lanes:
            sorted_pn_swap_dict = dict(sorted(pn_swap_dict.items()))
            pn_swap_map = list(sorted_pn_swap_dict.values())
        return pn_swap_map

    def gen_polarity_swap_map(
        self, sort_lanes: bool = True
    ) -> Dict[int, pm_types.TxRxLaneInfo]:
        core_ids = []
        for a in self._a_values:
            core_ids.append(a.chip.core_id)
        core_ids = set(core_ids)
        pn_swap_map = {}
        for core_id in core_ids:
            tx_pn_swap_map = self._get_pn_swap_map_by_core(core_id, TX, sort_lanes)
            rx_pn_swap_map = self._get_pn_swap_map_by_core(core_id, RX, sort_lanes)
            pn_swap_map[core_id] = pm_types.TxRxLaneInfo(
                tx_lane_info=tx_pn_swap_map, rx_lane_info=rx_pn_swap_map
            )
        return pn_swap_map

    def get_az_connections(self) -> List[pm_types.ConnectionPair]:
        return self._az_connections

    def get_static_mapping(self) -> pm_types.StaticMapping:
        return pm_types.StaticMapping(
            phy_lane_map=self.gen_phy_lane_map(),
            polarity_swap_map=self.gen_polarity_swap_map(),
            az_connections=self.get_az_connections(),
        )
