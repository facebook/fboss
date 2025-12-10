# pyre-strict
from typing import List

from fboss.lib.platform_mapping_v2.helpers import (
    fec_from_profile,
    is_backplane,
    is_npu,
    is_transceiver,
    is_xphy,
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

    def _matches_chip_type(
        self, setting: SpeedSetting, phy_chip_type: DataPlanePhyChipType
    ) -> bool:
        """Check if setting matches the requested chip type."""
        if phy_chip_type == DataPlanePhyChipType.IPHY:
            return is_npu(setting.a_chip_settings.chip_type) or is_npu(
                setting.z_chip_settings.chip_type
            )
        if phy_chip_type == DataPlanePhyChipType.TRANSCEIVER:
            return is_transceiver(setting.a_chip_settings.chip_type) or is_transceiver(
                setting.z_chip_settings.chip_type
            )
        if phy_chip_type == DataPlanePhyChipType.BACKPLANE:
            return is_backplane(setting.a_chip_settings.chip_type) or is_backplane(
                setting.z_chip_settings.chip_type
            )
        if phy_chip_type == DataPlanePhyChipType.XPHY:
            return is_xphy(setting.a_chip_settings.chip_type) or is_xphy(
                setting.z_chip_settings.chip_type
            )
        return False

    # Given a port profile and a chip type (iphy/xphy/transceiver/backplane),
    # return the row in Profile Settings matching that.
    def get_speed_setting(
        self, profile: PortProfileID, phy_chip_type: DataPlanePhyChipType
    ) -> SpeedSetting:
        speeds = profile_to_port_speed(profile)
        media_type = transmitter_tech_from_profile(profile)
        num_lanes = num_lanes_from_profile(profile)
        expected_fec = fec_from_profile(profile)

        # First pass: Try to find an exact match including FEC
        for setting in self._speed_settings:
            if (
                setting.speed in speeds
                and setting.media_type in media_type
                and setting.num_lanes == num_lanes
                and setting.fec == expected_fec
                and self._matches_chip_type(setting, phy_chip_type)
            ):
                return setting

        # Second pass: Fall back to matching without FEC if no exact match found
        for setting in self._speed_settings:
            if (
                setting.speed in speeds
                and setting.media_type in media_type
                and setting.num_lanes == num_lanes
                and self._matches_chip_type(setting, phy_chip_type)
            ):
                return setting

        raise Exception("Can't find speed setting for profile ", profile)
