# pyre-strict
from typing import List

from fboss.lib.platform_mapping_v2.helpers import (
    is_backplane,
    is_npu,
    is_transceiver,
    num_lanes_from_profile,
    profile_to_port_speed,
    transmitter_tech_from_profile,
)
from neteng.fboss.phy.ttypes import DataPlanePhyChipType

from neteng.fboss.platform_mapping_config.ttypes import SpeedSetting

from neteng.fboss.switch_config.ttypes import PortProfileID


class ProfileSettings:
    def __init__(self, speed_settings: List[SpeedSetting]) -> None:
        self._speed_settings = speed_settings

    # Given a port profile and a chip type (iphy/xphy/transceiver/backplane),
    # return the row in Profile Settings matching that.
    def get_speed_setting(
        self, profile: PortProfileID, phy_chip_type: DataPlanePhyChipType
    ) -> SpeedSetting:
        speeds = profile_to_port_speed(profile)
        media_type = transmitter_tech_from_profile(profile)
        num_lanes = num_lanes_from_profile(profile)
        for setting in self._speed_settings:
            if (
                setting.speed in speeds
                and setting.media_type in media_type
                and setting.num_lanes == num_lanes
            ):
                if phy_chip_type == DataPlanePhyChipType.IPHY and (
                    is_npu(setting.a_chip_settings.chip_type)
                    or is_npu(setting.z_chip_settings.chip_type)
                ):
                    return setting
                if phy_chip_type == DataPlanePhyChipType.TRANSCEIVER and (
                    is_transceiver(setting.a_chip_settings.chip_type)
                    or is_transceiver(setting.z_chip_settings.chip_type)
                ):
                    return setting
                if phy_chip_type == DataPlanePhyChipType.BACKPLANE and (
                    is_backplane(setting.a_chip_settings.chip_type)
                    or is_backplane(setting.z_chip_settings.chip_type)
                ):
                    return setting
        raise Exception("Can't find speed setting for profile ", profile)
