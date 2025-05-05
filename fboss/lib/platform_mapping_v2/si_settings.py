# pyre-strict
from typing import Dict, List, Optional

from neteng.fboss.platform_mapping_config.ttypes import (
    SiFactorAndSetting,
    SiSettingPinConnection,
    SiSettingRow,
)

from neteng.fboss.switch_config.ttypes import PortSpeed
from neteng.fboss.transceiver.ttypes import TransmitterTechnology


class SiSettings:
    def __init__(self, si_settings: List[SiSettingRow]) -> None:
        # Thrift-py types are not hashable, so use string representation as dict keys
        self._si_settings: Dict[str, List[SiSettingRow]] = {}
        for setting in si_settings:
            pin_connection_str = str(setting.pin_connection)
            if pin_connection_str not in self._si_settings:
                self._si_settings[pin_connection_str] = []
            self._si_settings[pin_connection_str].append(setting)

    # Given some factors, return the matching setting from the SI Settings CSV
    def get_factor_and_setting(
        self,
        pin_connection: SiSettingPinConnection,
        lane_speed: PortSpeed,
        media_type: List[TransmitterTechnology],
    ) -> Optional[SiFactorAndSetting]:
        pin_connection_str = str(pin_connection)
        if pin_connection_str not in self._si_settings:
            return None
        for setting in self._si_settings[pin_connection_str]:
            if (
                setting.factor.lane_speed == lane_speed
                and setting.factor.media_type in media_type
            ):
                if setting.factor.cable_length:
                    # There is an extra factor specified other than lane speed and media
                    return SiFactorAndSetting(
                        factor=setting.factor,
                        tx_setting=setting.tx_setting,
                        rx_setting=setting.rx_setting,
                    )
                else:
                    return SiFactorAndSetting(
                        factor=None,
                        tx_setting=setting.tx_setting,
                        rx_setting=setting.rx_setting,
                    )

        return None
