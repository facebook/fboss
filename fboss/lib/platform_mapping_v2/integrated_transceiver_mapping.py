# pyre-strict
from typing import Dict, List, Optional, Tuple

import neteng.fboss.platform_mapping_config.thrift_types as pm_types


class IntegratedTransceiverMapping:
    def __init__(
        self,
        connections: List[pm_types.IntegratedTransceiverConnection],
    ) -> None:
        self._connections = connections
        self._lookup: Optional[
            Dict[Tuple[int, int, int], pm_types.IntegratedTransceiverConnection]
        ] = None

    def _build_lookup(self) -> None:
        self._lookup = {}
        for conn in self._connections:
            key = (
                conn.transceiver.chip.chip_id,
                conn.transceiver.chip.core_id,
                conn.transceiver.lane.logical_id,
            )
            self._lookup[key] = conn

    def get_connection(
        self, chip_id: int, core_id: int, lane: int
    ) -> Optional[pm_types.IntegratedTransceiverConnection]:
        if self._lookup is None:
            self._build_lookup()
        assert self._lookup is not None
        return self._lookup.get((chip_id, core_id, lane))

    def get_chips(self) -> List[pm_types.Chip]:
        unique_chips: List[pm_types.Chip] = []
        unique_chip_strs: set[str] = set()
        for conn in self._connections:
            for chip in [conn.opticalEngine.chip, conn.laserSource.chip]:
                chip_str = str(chip)
                if chip_str not in unique_chip_strs:
                    unique_chip_strs.add(chip_str)
                    unique_chips.append(chip)
        return unique_chips

    def get_connections(self) -> List[pm_types.IntegratedTransceiverConnection]:
        return self._connections
