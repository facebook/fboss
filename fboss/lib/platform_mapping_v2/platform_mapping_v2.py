# pyre-strict
import sys
from typing import Dict, List, Optional

from fboss.lib.platform_mapping_v2.asic_vendor_config import AsicVendorConfig
from fboss.lib.platform_mapping_v2.helpers import (
    get_backplane_chip,
    get_connection_pairs_for_profile,
    get_mapping_pins,
    get_npu_chip,
    get_pins_from_connections,
    get_platform_config_entry,
    get_transceiver_chip,
    get_unique_connection_pairs,
    is_backplane,
    is_npu,
    is_transceiver,
)
from fboss.lib.platform_mapping_v2.port_profile_mapping import PortProfileMapping
from fboss.lib.platform_mapping_v2.profile_settings import ProfileSettings
from fboss.lib.platform_mapping_v2.read_files_utils import (
    read_asic_vendor_config,
    read_port_profile_mapping,
    read_profile_settings,
    read_si_settings,
    read_static_mapping,
)
from fboss.lib.platform_mapping_v2.si_settings import SiSettings
from fboss.lib.platform_mapping_v2.static_mapping import StaticMapping

from neteng.fboss.phy.ttypes import DataPlanePhyChip, DataPlanePhyChipType

from neteng.fboss.platform_config.ttypes import (
    PlatformMapping,
    PlatformPortConfig,
    PlatformPortEntry,
    PlatformPortMapping,
    PlatformPortProfileConfigEntry,
)


class PlatformMappingParser:
    def __init__(
        self,
        directory_map: Dict[str, Dict[str, str]],
        platform: str,
        multi_npu: bool = False,
        version: Optional[str] = None,
    ) -> None:
        self.platform = platform
        self.version = version
        self._directory_map = directory_map
        self._multi_npu = multi_npu
        self._static_mapping: StaticMapping
        self._port_profile_mapping: PortProfileMapping
        self._profile_settings: ProfileSettings
        self._si_settings: SiSettings
        self._asic_vendor_config: Optional[AsicVendorConfig] = None
        self._read_csvs()

    def get_directory(self, port_profile: bool = False) -> Dict[str, str]:
        if (
            self.platform
            in [
                "meru800bia_100g_nif_port_breakout",
                "meru800bia_800g",
                "meru800bia_dual_stage_edsw",
                "meru800bia_dual_stage_rdsw",
            ]
            and port_profile is False
        ):
            return self._directory_map["meru800bia"]
        return self._directory_map[self.platform]

    def get_mapping_prefix(self, port_profile: bool = False) -> str:
        if port_profile:
            prefix = (
                self.platform
                if self.platform
                in [
                    "meru800bia_100g_nif_port_breakout",
                    "meru800bia_800g",
                    "meru800bia_dual_stage_edsw",
                    "meru800bia_dual_stage_rdsw",
                ]
                else self.platform + "_port"
            )
        else:
            prefix = (
                "meru800bia"
                if self.platform
                in [
                    "meru800bia_100g_nif_port_breakout",
                    "meru800bia_800g",
                    "meru800bia_dual_stage_edsw",
                    "meru800bia_dual_stage_rdsw",
                ]
                else self.platform
            )
        if self.platform in ["tahan800bc_test_fixture", "tahan800bc_chassis"]:
            prefix = "tahan800bc_port" if port_profile else "tahan800bc"
        return prefix

    def _read_csvs(self) -> None:
        if self.platform == "yangra":
            # TODO(pshaikh): add processing for yangra platform csv processing
            self._static_mapping = StaticMapping(az_connections=[])
            self._port_profile_mapping = PortProfileMapping(ports={})
            self._profile_settings = ProfileSettings(speed_settings=[])
            self._si_settings = SiSettings(si_settings=[])
            return
        self._port_profile_mapping = read_port_profile_mapping(
            directory=self.get_directory(port_profile=True),
            prefix=self.get_mapping_prefix(port_profile=True),
            multi_npu=self._multi_npu,
        )

        self._static_mapping = read_static_mapping(
            directory=self.get_directory(), prefix=self.get_mapping_prefix()
        )
        self._profile_settings = read_profile_settings(
            directory=self.get_directory(), prefix=self.get_mapping_prefix()
        )
        self._si_settings = read_si_settings(
            directory=self.get_directory(),
            prefix=self.get_mapping_prefix(),
            version=self.version,
        )
        try:
            self._asic_vendor_config = read_asic_vendor_config(
                directory=self.get_directory(), prefix=self.get_mapping_prefix()
            )
        except FileNotFoundError:
            print("No asic vendor config found...", file=sys.stderr)

    def get_static_mapping(self) -> StaticMapping:
        return self._static_mapping

    def get_port_profile_mapping(self) -> PortProfileMapping:
        return self._port_profile_mapping

    def get_profile_settings(self) -> ProfileSettings:
        return self._profile_settings

    def get_si_settings(self) -> SiSettings:
        return self._si_settings

    def get_asic_vendor_config(self) -> Optional[AsicVendorConfig]:
        return self._asic_vendor_config


class PlatformMappingV2:
    def __init__(
        self,
        directory_map: Dict[str, Dict[str, str]],
        platform: str,
        multi_npu: bool = False,
        version: Optional[str] = None,
    ) -> None:
        self.platform = platform
        self._multi_npu = multi_npu
        self.pm_parser = PlatformMappingParser(
            directory_map, platform, multi_npu, version
        )
        self.platform_mapping: PlatformMapping = self._generate_platform_mapping()

    def get_platform_mapping(self) -> PlatformMapping:
        return self.platform_mapping

    def get_platform_profiles(self) -> List[PlatformPortProfileConfigEntry]:
        return self.platform_mapping.platformSupportedProfiles

    def get_platform_port_map(self) -> Dict[int, PlatformPortEntry]:
        return self.platform_mapping.ports

    def get_chips(self) -> List[DataPlanePhyChip]:
        return self.platform_mapping.chips

    def _generate_platform_mapping(self) -> PlatformMapping:
        return PlatformMapping(
            ports=self._generate_ports(),
            chips=self._generate_chips(),
            platformSupportedProfiles=self._generate_platform_supported_profiles(),
        )

    def _generate_chips(self) -> List[DataPlanePhyChip]:
        parsed_chips = self.pm_parser.get_static_mapping().get_chips()
        chips = []
        if self.platform == "yangra":
            # TODO(pshaikh): add logic to generate chips for yangra
            return chips

        for chip in parsed_chips:
            if is_npu(chip.chip_type):
                # Skip adding NPUs other than the first if it's not multi npu
                if not self._multi_npu and chip.chip_id != 1:
                    continue
                chips.append(get_npu_chip(chip))
            elif is_transceiver(chip.chip_type):
                chips.append(get_transceiver_chip(chip))
            elif is_backplane(chip.chip_type):
                chips.append(get_backplane_chip(chip))
            else:
                raise Exception("Unhandled chip_type ", chip.chip_type)
        chips = sorted(chips, key=lambda chip: (chip.type, chip.physicalID, chip.name))
        return chips

    def _generate_platform_supported_profiles(
        self,
    ) -> List[PlatformPortProfileConfigEntry]:
        platform_config_entry = []
        if self.platform == "yangra":
            # TODO(pshaikh): add logic to generate platform supported profiles for yangra
            return platform_config_entry

        all_profiles = self.pm_parser.get_port_profile_mapping().get_all_profiles()

        for profile in all_profiles:
            entry = get_platform_config_entry(
                profile,
                self.pm_parser.get_profile_settings().get_speed_setting(
                    profile, DataPlanePhyChipType.IPHY
                ),
            )
            if entry:
                platform_config_entry.append(entry)

        return platform_config_entry

    def _generate_ports(self) -> Dict[int, PlatformPortEntry]:
        ports = {}
        if self.platform == "yangra":
            # TODO(pshaikh): add logic to generate ports for yangra
            return ports

        for port_id, port_detail in (
            self.pm_parser.get_port_profile_mapping().get_ports().items()
        ):
            port_entry = PlatformPortEntry()
            port_entry.supportedProfiles = {}
            all_connection_pairs = []
            # Get pins for all supported profiles
            for profile in port_detail.supported_profiles:
                platform_port_config = PlatformPortConfig()

                speed_setting = self.pm_parser.get_profile_settings().get_speed_setting(
                    profile=profile, phy_chip_type=DataPlanePhyChipType.IPHY
                )
                profile_connections = get_connection_pairs_for_profile(
                    static_mapping=self.pm_parser.get_static_mapping(),
                    port=port_detail,
                    num_lanes=speed_setting.num_lanes,
                    profile=profile,
                )
                all_connection_pairs = all_connection_pairs + profile_connections
                platform_port_config.pins = get_pins_from_connections(
                    connections=profile_connections,
                    si_settings=self.pm_parser.get_si_settings(),
                    profile=profile,
                    # pyre-fixme[6]: Expected `PortSpeed` for 4th param, but got `float`.
                    lane_speed=speed_setting.speed / speed_setting.num_lanes,
                )
                port_entry.supportedProfiles[profile] = platform_port_config

            # We now know pins used for each of the supported profiles.
            # Combine all the pins and populate mapping.pins
            mapping = PlatformPortMapping()
            mapping.id = port_id
            mapping.name = port_detail.port_name
            all_connection_pairs = get_unique_connection_pairs(all_connection_pairs)
            mapping.pins = get_mapping_pins(all_connection_pairs)
            # Sort pins by lane id of the 'a' end
            mapping.pins = sorted(mapping.pins, key=lambda pin: pin.a.lane)
            # Mark the controllingPort as self here. We'll override it later when
            # we figure out which ports this port needs to subsume
            mapping.controllingPort = port_id
            mapping.portType = port_detail.port_type
            mapping.scope = port_detail.scope
            if port_detail.attached_coreid is not None:
                mapping.attachedCoreId = port_detail.attached_coreid
            if port_detail.attached_core_portid is not None:
                mapping.attachedCorePortIndex = port_detail.attached_core_portid
            if port_detail.virtual_device_id is not None:
                mapping.virtualDeviceId = port_detail.virtual_device_id
            port_entry.mapping = mapping

            ports[port_id] = port_entry

        # Populate subsumedPorts and controllingPort now that we have
        # information about every port.
        # The logic is - when a port is left with no iphy pins to use after some
        # other port uses its iphy pins, then the former port needs to be a
        # subsumed port controlled by the later port
        for port_id, port_entry in ports.items():
            for port_config in port_entry.supportedProfiles.values():
                all_iphy_pins_needed = port_config.pins.iphy
                for other_port_id, other_port_entry in ports.items():
                    if port_id == other_port_id:
                        continue
                    for (
                        other_port_config
                    ) in other_port_entry.supportedProfiles.values():
                        if all(
                            needed_iphy_pin in other_port_config.pins.iphy
                            for needed_iphy_pin in all_iphy_pins_needed
                        ):
                            if not other_port_config.subsumedPorts:
                                other_port_config.subsumedPorts = []
                            if port_id not in other_port_config.subsumedPorts:
                                other_port_config.subsumedPorts.append(port_id)
                            port_entry.mapping.controllingPort = other_port_id
        return ports
