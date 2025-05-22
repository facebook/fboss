# pyre-strict
import re
from typing import List, Optional

from fboss.lib.platform_mapping_v2.si_settings import SiSettings
from fboss.lib.platform_mapping_v2.static_mapping import StaticMapping
from neteng.fboss.phy.ttypes import (
    DataPlanePhyChip,
    DataPlanePhyChipType,
    Pin,
    PinConfig,
    PinConnection,
    PinID,
    PortPinConfig,
    PortProfileConfig,
    ProfileSideConfig,
    RxSettings,
    TxSettings,
)

from neteng.fboss.platform_config.ttypes import (
    PlatformPortConfigFactor,
    PlatformPortProfileConfigEntry,
)
from neteng.fboss.platform_mapping_config.ttypes import (
    Chip,
    ChipType,
    ConnectionEnd,
    ConnectionPair,
    CoreType,
    Port,
    SiSettingPinConnection,
    SpeedSetting,
)

from neteng.fboss.switch_config.ttypes import PortProfileID, PortSpeed

from neteng.fboss.transceiver.ttypes import TransmitterTechnology


def profile_to_port_speed(profile: PortProfileID) -> List[PortSpeed]:
    if profile in [
        PortProfileID.PROFILE_53POINT125G_1_PAM4_RS545_COPPER,
        PortProfileID.PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL,
    ]:
        return [PortSpeed.FIFTYTHREEPOINTONETWOFIVEG]
    if profile in [
        PortProfileID.PROFILE_106POINT25G_1_PAM4_RS544_OPTICAL,
        PortProfileID.PROFILE_106POINT25G_1_PAM4_RS544_COPPER,
    ]:
        return [PortSpeed.HUNDREDANDSIXPOINTTWOFIVEG]
    if profile in [
        PortProfileID.PROFILE_100G_4_NRZ_RS528_COPPER,
        PortProfileID.PROFILE_100G_4_NRZ_RS528_OPTICAL,
        PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL,
        PortProfileID.PROFILE_100G_1_PAM4_NOFEC_COPPER,
    ]:
        return [PortSpeed.HUNDREDG]
    if profile in [
        PortProfileID.PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_200G_4_PAM4_RS544X2N_COPPER,
    ]:
        return [PortSpeed.TWOHUNDREDG]
    if profile in [
        PortProfileID.PROFILE_400G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_400G_4_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_400G_8_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_400G_2_PAM4_RS544X2N_OPTICAL,
    ]:
        return [PortSpeed.FOURHUNDREDG]
    if profile in [
        PortProfileID.PROFILE_800G_8_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_800G_8_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_800G_4_PAM4_RS544X2N_OPTICAL,
    ]:
        return [PortSpeed.EIGHTHUNDREDG]
    if profile in [
        PortProfileID.PROFILE_10G_1_NRZ_NOFEC_COPPER,
        PortProfileID.PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
    ]:
        return [PortSpeed.XG]
    if profile in [PortProfileID.PROFILE_50G_2_NRZ_RS528_OPTICAL]:
        return [PortSpeed.FIFTYG]
    if profile in [PortProfileID.PROFILE_25G_1_NRZ_NOFEC_OPTICAL]:
        return [PortSpeed.TWENTYFIVEG]
    raise Exception("Can't convert profile to speed for profileID ", profile)


def num_lanes_from_profile(profile: PortProfileID) -> int:
    if profile in [
        PortProfileID.PROFILE_10G_1_NRZ_NOFEC_COPPER,
        PortProfileID.PROFILE_53POINT125G_1_PAM4_RS545_COPPER,
        PortProfileID.PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL,
        PortProfileID.PROFILE_106POINT25G_1_PAM4_RS544_OPTICAL,
        PortProfileID.PROFILE_106POINT25G_1_PAM4_RS544_COPPER,
        PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL,
        PortProfileID.PROFILE_100G_1_PAM4_NOFEC_COPPER,
        PortProfileID.PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
        PortProfileID.PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
    ]:
        return 1
    if profile in [
        PortProfileID.PROFILE_50G_2_NRZ_RS528_OPTICAL,
        PortProfileID.PROFILE_400G_2_PAM4_RS544X2N_OPTICAL,
    ]:
        return 2
    if profile in [
        PortProfileID.PROFILE_100G_4_NRZ_RS528_COPPER,
        PortProfileID.PROFILE_100G_4_NRZ_RS528_OPTICAL,
        PortProfileID.PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_200G_4_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_400G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_400G_4_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_800G_4_PAM4_RS544X2N_OPTICAL,
    ]:
        return 4
    if profile in [
        PortProfileID.PROFILE_400G_8_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_800G_8_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_800G_8_PAM4_RS544X2N_COPPER,
    ]:
        return 8
    raise Exception("Can't find num lanes for profile ", profile)


def is_npu(chip_type: ChipType) -> bool:
    return chip_type == ChipType.NPU


def is_transceiver(chip_type: ChipType) -> bool:
    return chip_type == ChipType.TRANSCEIVER


def is_backplane(chip_type: ChipType) -> bool:
    return chip_type == ChipType.BACKPLANE


# Any terminal connection (front panel transceivers or backplane connectors)
def is_terminal_chip(chip_type: ChipType) -> bool:
    return is_transceiver(chip_type) or is_backplane(chip_type)


def get_npu_chip_name(chip: Chip) -> str:
    return f"{ChipType._VALUES_TO_NAMES[chip.chip_type]}-{CoreType._VALUES_TO_NAMES[chip.core_type]}-slot{chip.slot_id}/chip{chip.chip_id}/core{chip.core_id}"


def get_terminal_chip_name(chip: Chip) -> str:
    return f"{ChipType._VALUES_TO_NAMES[chip.chip_type]}-{CoreType._VALUES_TO_NAMES[chip.core_type]}-slot{chip.slot_id}/chip{chip.chip_id}"


def get_npu_chip(chip: Chip) -> DataPlanePhyChip:
    if not is_npu(chip.chip_type):
        raise Exception(chip.chip_type, " is not a NPU")
    return DataPlanePhyChip(
        name=get_npu_chip_name(chip=chip),
        type=DataPlanePhyChipType.IPHY,
        physicalID=chip.core_id,
    )


def get_transceiver_chip(chip: Chip) -> DataPlanePhyChip:
    if not is_transceiver(chip.chip_type):
        raise Exception(chip.chip_type, " is not a Transceiver")
    return DataPlanePhyChip(
        name=get_terminal_chip_name(chip=chip),
        type=DataPlanePhyChipType.TRANSCEIVER,
        physicalID=chip.chip_id - 1,  # We need 0 indexed physicalIDs
    )


def get_backplane_chip(chip: Chip) -> DataPlanePhyChip:
    if not is_backplane(chip.chip_type):
        raise Exception(chip.chip_type, " is not backplane")
    return DataPlanePhyChip(
        name=get_terminal_chip_name(chip=chip),
        type=DataPlanePhyChipType.BACKPLANE,
        physicalID=chip.chip_id - 1,  # We need 0 indexed physicalIDs
    )


# Creates a PlatformPortProfileConfigEntry for a given profileID, numlanes,
# modulation, fec mode and interface type
def get_platform_config_entry(
    profile: PortProfileID, speed_setting: SpeedSetting
) -> Optional[PlatformPortProfileConfigEntry]:
    factor = PlatformPortConfigFactor()
    profile_config = PortProfileConfig()
    factor.profileID = profile

    profile_config.speed = speed_setting.speed
    profile_side_config = ProfileSideConfig()
    if is_npu(speed_setting.a_chip_settings.chip_type) or is_npu(
        speed_setting.z_chip_settings.chip_type
    ):
        profile_side_config.numLanes = speed_setting.num_lanes
        profile_side_config.modulation = speed_setting.modulation
        profile_side_config.fec = speed_setting.fec
        if is_npu(speed_setting.a_chip_settings.chip_type):
            profile_side_config.interfaceType = (
                speed_setting.a_chip_settings.chip_interface_type
            )
        else:
            profile_side_config.interfaceType = (
                speed_setting.z_chip_settings.chip_interface_type
            )
        profile_side_config.medium = speed_setting.media_type
        profile_config.iphy = profile_side_config
        return PlatformPortProfileConfigEntry(factor=factor, profile=profile_config)
    return None


# Given a port name, find the connection end to start the topology traversal
def get_start_connection_end(
    port_name: str, static_mapping: StaticMapping
) -> ConnectionEnd:
    match = re.match(r"(rcy|eth|fab|evt)(\d+)/(\d+)/(\d+)", port_name)
    if not match:
        raise Exception(f"Port name {port_name} is not a valid format.")
    if match[1] == "rcy" or match[1] == "evt":
        # Recycle ports are named "rcy{slot_id}/{chip_id}/{core_id}"
        return static_mapping.find_connection_end(
            slot_id=int(match[2]),
            chip_id=int(match[3]),
            chip_types={ChipType.NPU},
            core_id=int(match[4]),
            logical_lane_id=0,
        )
    # Front panel ports are named "eth|fab{slot_id}/{transceiver_chip_id}/{transceiver_lane_id}"
    # Backplane ports are named "eth{slot_id}/{backplane_chip_id}/{backplane_lane_id}"
    return static_mapping.find_connection_end(
        slot_id=int(match[2]),
        chip_id=int(match[3]),
        chip_types={ChipType.TRANSCEIVER, ChipType.BACKPLANE},
        core_id=0,
        logical_lane_id=int(match[4]) - 1,  # Lanes are 0 indexed in CSV
    )


# Given a port and its speed, find all the connection end points required
def get_connection_pairs_for_profile(
    static_mapping: StaticMapping, port: Port, num_lanes: int, profile: PortProfileID
) -> List[ConnectionPair]:
    start_connection_end = get_start_connection_end(
        port_name=port.port_name, static_mapping=static_mapping
    )
    connections = []
    for lane in range(num_lanes):
        # Start with given lane
        current_connection_end = static_mapping.find_connection_end(
            slot_id=start_connection_end.chip.slot_id,
            chip_id=start_connection_end.chip.chip_id,
            chip_types={start_connection_end.chip.chip_type},
            core_id=start_connection_end.chip.core_id,
            logical_lane_id=start_connection_end.lane.logical_id + lane,
        )
        # Iterate until we reach NPU
        while True:
            next_connection_end = static_mapping.get_other_connection_end(
                one_connection_end=current_connection_end
            )
            if next_connection_end:
                connections.append(
                    ConnectionPair(
                        a=next_connection_end,
                        z=current_connection_end,
                    )
                )
            else:
                connections.append(
                    ConnectionPair(
                        a=current_connection_end,
                    )
                )
            current_connection_end = next_connection_end
            if (
                not current_connection_end
                or not current_connection_end.chip
                or not current_connection_end.chip.chip_type
                or is_npu(current_connection_end.chip.chip_type)
            ):
                # Iterate until we reach NPU
                break
    return connections


# Creates a pin config from ConnectionEnd type
def get_pin_config(connection_end: ConnectionEnd) -> PinConfig:
    pin_config = PinConfig()
    if is_npu(connection_end.chip.chip_type):
        pin_name = get_npu_chip_name(connection_end.chip)
    elif is_terminal_chip(connection_end.chip.chip_type):
        pin_name = get_terminal_chip_name(connection_end.chip)
    else:
        raise Exception("Don't understand chip type ", connection_end.chip.chip_type)

    pin_config.id = PinID(chip=pin_name, lane=connection_end.lane.logical_id)
    return pin_config


# Helper function that creates a PortPinConfig from the types that store
# the parsed information from the CSVs
def get_pins_from_connections(
    connections: List[ConnectionPair],
    si_settings: SiSettings,
    profile: PortProfileID,
    lane_speed: PortSpeed,
) -> PortPinConfig:
    port_pin_config_iphy = []
    port_pin_config_tcvr = []
    for connection_pair in connections:
        for connection in [
            connection_pair.a,
            connection_pair.z,
        ]:
            if connection and is_terminal_chip(connection.chip.chip_type):
                port_pin_config_tcvr.append(get_pin_config(connection_end=connection))
            if connection and is_npu(connection.chip.chip_type):
                pin_config = get_pin_config(connection_end=connection)
                pin_connection = SiSettingPinConnection(
                    chip=connection.chip,
                    logical_lane_id=connection.lane.logical_id,
                )
                si_setting_and_factor = si_settings.get_factor_and_setting(
                    pin_connection=pin_connection,
                    lane_speed=lane_speed,
                    media_type=transmitter_tech_from_profile(profile),
                )
                if si_setting_and_factor and si_setting_and_factor.factor is None:
                    # TODO: FIXME Address additional factors
                    # If there is an additional factor, the SI setting will be added as a port override later
                    if si_setting_and_factor.tx_setting != TxSettings():
                        pin_config.tx = si_setting_and_factor.tx_setting
                    if si_setting_and_factor.rx_setting != RxSettings():
                        pin_config.rx = si_setting_and_factor.rx_setting
                port_pin_config_iphy.append(pin_config)

    return PortPinConfig(
        iphy=port_pin_config_iphy,
        transceiver=port_pin_config_tcvr or None,
    )


# Helper function that creates a list of PinConnections that go into platform mapping
# given the list of ConnectionPairs (how the CSV information is stored internally)
def get_mapping_pins(connection_pairs: List[ConnectionPair]) -> List[PinConnection]:
    pin_connections = []
    for connection in connection_pairs:
        pin_connection = PinConnection()
        if is_npu(connection.a.chip.chip_type):
            pin_name = get_npu_chip_name(connection.a.chip)
            pin_connection.a = PinID(chip=pin_name, lane=connection.a.lane.logical_id)
        if connection.z:
            # Required linter type hint
            connection_z: ConnectionEnd = connection.z
            if is_terminal_chip(connection_z.chip.chip_type):
                pin_name = get_terminal_chip_name(connection_z.chip)
                pin_connection.z = Pin(
                    end=PinID(
                        chip=pin_name,
                        lane=connection_z.lane.logical_id,
                    )
                )
            else:
                raise Exception("Unsupported chip type ", connection_z.chip.chip_type)
        pin_connections.append(pin_connection)
    return pin_connections


def transmitter_tech_from_profile(
    profile: PortProfileID,
) -> List[TransmitterTechnology]:
    if profile in [
        PortProfileID.PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL,
        PortProfileID.PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_400G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_800G_8_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_106POINT25G_1_PAM4_RS544_OPTICAL,
        PortProfileID.PROFILE_100G_4_NRZ_RS528_OPTICAL,
        PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL,
        PortProfileID.PROFILE_50G_2_NRZ_RS528_OPTICAL,
        PortProfileID.PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
        PortProfileID.PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
        PortProfileID.PROFILE_800G_4_PAM4_RS544X2N_OPTICAL,
        PortProfileID.PROFILE_400G_2_PAM4_RS544X2N_OPTICAL,
    ]:
        return [TransmitterTechnology.OPTICAL, TransmitterTechnology.BACKPLANE]
    if profile in [
        PortProfileID.PROFILE_53POINT125G_1_PAM4_RS545_COPPER,
        PortProfileID.PROFILE_200G_4_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_106POINT25G_1_PAM4_RS544_COPPER,
        PortProfileID.PROFILE_10G_1_NRZ_NOFEC_COPPER,
        PortProfileID.PROFILE_100G_4_NRZ_RS528_COPPER,
        PortProfileID.PROFILE_400G_4_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_400G_8_PAM4_RS544X2N_COPPER,
        PortProfileID.PROFILE_100G_1_PAM4_NOFEC_COPPER,
        PortProfileID.PROFILE_800G_8_PAM4_RS544X2N_COPPER,
    ]:
        return [TransmitterTechnology.COPPER]
    raise Exception("Can't figure out transmitter tech for profile ", profile)


def get_unique_connection_pairs(
    connection_pairs: List[ConnectionPair],
) -> List[ConnectionPair]:
    connection_pair_strs = set()
    unique_connection_pairs = []

    for connection_pair in connection_pairs:
        connection_pair_str = str(connection_pair)
        if connection_pair_str not in connection_pair_strs:
            connection_pair_strs.add(connection_pair_str)
            unique_connection_pairs.append(connection_pair)
    return unique_connection_pairs
