# pyre-strict
from typing import Dict, List

from neteng.fboss.platform_mapping_config.ttypes import Port
from neteng.fboss.switch_config.ttypes import PortProfileID


class PortProfileMapping:
    def __init__(self, ports: Dict[int, Port]) -> None:
        self._ports = ports

    # Returns all the profileIDs supported on this platform
    def get_all_profiles(self) -> List[PortProfileID]:
        all_profiles = []
        for port in self._ports.values():
            all_profiles = all_profiles + port.supported_profiles
        return list(set(all_profiles))

    def get_ports(self) -> Dict[int, Port]:
        return self._ports
