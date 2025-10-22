# pyre-unsafe
import copy
import json
from enum import IntEnum
from typing import Any, Dict, Optional

from fboss.lib.platform_mapping_v2.asic_vendor_config import AsicVendorConfig
from fboss.lib.platform_mapping_v2.port_profile_mapping import PortProfileMapping
from fboss.lib.platform_mapping_v2.profile_settings import ProfileSettings
from fboss.lib.platform_mapping_v2.si_settings import SiSettings
from fboss.lib.platform_mapping_v2.static_mapping import StaticMapping
from neteng.fboss.asic_config_v2.ttypes import AsicVendorConfigParams
from neteng.fboss.phy.ttypes import (
    FecMode,
    InterfaceType,
    IpModulation,
    RxSettings,
    TxSettings,
)
from neteng.fboss.platform_mapping_config.ttypes import (
    Chip,
    ChipSetting,
    ChipType,
    ConnectionEnd,
    ConnectionPair,
    CoreType,
    Lane,
    Port,
    SiSettingFactor,
    SiSettingPinConnection,
    SiSettingRow,
    SpeedSetting,
    TransceiverOverrideSetting,
)

from neteng.fboss.switch_config.ttypes import PortProfileID, PortSpeed, PortType
from neteng.fboss.transceiver.ttypes import (
    MediaInterfaceCode,
    TransmitterTechnology,
    Vendor,
)


def get_content(directory: Dict[str, str], filename: str) -> str:
    if filename not in directory:
        raise FileNotFoundError(
            f"File {filename} not found in directory with keys {directory.keys()}"
        )
    return directory[filename]


def column_int_enum_generator(string_list: str):
    # pyre-ignore
    return IntEnum(
        "Column", {item: idx for idx, item in enumerate(string_list.split())}
    )


def read_static_mapping(directory: Dict[str, str], prefix: str) -> StaticMapping:
    STATIC_MAPPING_SUFFIX = "_static_mapping.csv"
    Column = column_int_enum_generator(
        "A_SLOT_ID A_CHIP_ID A_CHIP_TYPE A_CORE_ID A_CORE_TYPE A_CORE_LANE A_PHYSICAL_TX_LANE "
        + "A_PHYSICAL_RX_LANE A_TX_POLARITY_SWAP A_RX_POLARITY_SWAP Z_SLOT_ID "
        + "Z_CHIP_ID Z_CHIP_TYPE Z_CORE_ID Z_CORE_TYPE Z_CORE_LANE Z_PHYSICAL_TX_LANE "
        + "Z_PHYSICAL_RX_LANE Z_TX_POLARITY_SWAP Z_RX_POLARITY_SWAP",
    )
    connections = []
    for index, line in enumerate(
        get_content(directory, prefix + STATIC_MAPPING_SUFFIX).splitlines()
    ):
        if index < 1:
            # Skip the header
            continue
        row = line.split(",")
        if row[Column.A_PHYSICAL_TX_LANE]:
            # If there this a physical tx lane, there should be information
            # about its and the corresponding rx lane
            a_tx_physical_lane = int(row[Column.A_PHYSICAL_TX_LANE])
            a_rx_physical_lane = int(row[Column.A_PHYSICAL_RX_LANE])
            a_tx_polarity_swap = bool(row[Column.A_TX_POLARITY_SWAP] == "Y")
            a_rx_polarity_swap = bool(row[Column.A_RX_POLARITY_SWAP] == "Y")
        else:
            a_tx_physical_lane = None
            a_rx_physical_lane = None
            a_tx_polarity_swap = None
            a_rx_polarity_swap = None
        a_lane = Lane(
            logical_id=int(row[Column.A_CORE_LANE]),
            tx_physical_lane=a_tx_physical_lane,
            rx_physical_lane=a_rx_physical_lane,
            tx_polarity_swap=a_tx_polarity_swap,
            rx_polarity_swap=a_rx_polarity_swap,
        )
        a_chip = Chip(
            slot_id=int(row[Column.A_SLOT_ID]),
            chip_id=int(row[Column.A_CHIP_ID]),
            chip_type=ChipType._NAMES_TO_VALUES[row[Column.A_CHIP_TYPE]],
            core_id=int(row[Column.A_CORE_ID]),
            core_type=CoreType._NAMES_TO_VALUES[row[Column.A_CORE_TYPE]],
        )
        a_connection_end = ConnectionEnd(
            chip=a_chip,
            lane=a_lane,
        )
        # Z end is optional for ports like recycle ports
        if row[Column.Z_SLOT_ID]:
            z_chip = Chip(
                slot_id=int(row[Column.Z_SLOT_ID]),
                chip_id=int(row[Column.Z_CHIP_ID]),
                chip_type=ChipType._NAMES_TO_VALUES[row[Column.Z_CHIP_TYPE]],
                core_id=int(row[Column.Z_CORE_ID]),
                core_type=CoreType._NAMES_TO_VALUES[row[Column.Z_CORE_TYPE]],
            )
            z_lane = Lane(
                logical_id=int(row[Column.Z_CORE_LANE]),
                tx_physical_lane=int(row[Column.Z_PHYSICAL_TX_LANE]),
                rx_physical_lane=int(row[Column.Z_PHYSICAL_RX_LANE]),
                tx_polarity_swap=bool(row[Column.Z_TX_POLARITY_SWAP] == "Y"),
                rx_polarity_swap=bool(row[Column.Z_RX_POLARITY_SWAP] == "Y"),
            )
            z_connection_end = ConnectionEnd(
                chip=z_chip,
                lane=z_lane,
            )
        else:
            z_connection_end = None

        connections.append(ConnectionPair(a=a_connection_end, z=z_connection_end))

    return StaticMapping(az_connections=connections)


def read_port_profile_mapping(
    directory: Dict[str, str], prefix: str, multi_npu: bool
) -> PortProfileMapping:
    PORT_PROFILE_MAPPING_SUFFIX = "_port_profile_mapping.csv"
    Column = column_int_enum_generator(
        "GLOBAL_PORT_ID LOGICAL_PORT_ID PORT_NAME SUPPORTED_PROFILES ATTACHED_COREID ATTACHED_CORE_PORTID VIRTUAL_DEVICE_ID PORT_TYPE SCOPE PARENT_PORT",
    )
    ports = {}
    for index, line in enumerate(
        get_content(directory, prefix + PORT_PROFILE_MAPPING_SUFFIX).splitlines()
    ):
        if index < 1:
            # Skip the header
            continue
        row = line.split(",")
        global_port_id = int(row[Column.GLOBAL_PORT_ID])
        logical_port_id = int(row[Column.LOGICAL_PORT_ID])

        # If global_port_id is not the same as logical_port_id then this is a multi_npu system.
        # Only include second NPU ports if multi_npu option is used
        if not multi_npu and global_port_id != logical_port_id:
            continue

        port_name = row[Column.PORT_NAME]
        if row[Column.ATTACHED_COREID]:
            attached_coreid = int(row[Column.ATTACHED_COREID])
        else:
            attached_coreid = None
        if row[Column.ATTACHED_CORE_PORTID]:
            attached_core_portid = int(row[Column.ATTACHED_CORE_PORTID])
        else:
            attached_core_portid = None
        if row[Column.VIRTUAL_DEVICE_ID]:
            virtual_device_id = int(row[Column.VIRTUAL_DEVICE_ID])
        else:
            virtual_device_id = None
        supported_profiles = []
        for profile in row[Column.SUPPORTED_PROFILES].split("-"):
            if int(profile) not in PortProfileID._VALUES_TO_NAMES:
                raise Exception("Don't understand profile ", profile)
            supported_profiles.append(int(profile))
        port_type = row[Column.PORT_TYPE]
        if int(port_type) not in PortType._VALUES_TO_NAMES:
            raise Exception("Don't understand port type", port_type)
        scope = row[Column.SCOPE]
        if Column.PARENT_PORT < len(row) and row[Column.PARENT_PORT]:
            parent_port_id = int(row[Column.PARENT_PORT])
        else:
            parent_port_id = None
        assert global_port_id not in ports
        ports[global_port_id] = Port(
            global_port_id=global_port_id,
            logical_port_id=logical_port_id,
            port_name=port_name,
            supported_profiles=supported_profiles,
            attached_coreid=attached_coreid,
            attached_core_portid=attached_core_portid,
            virtual_device_id=virtual_device_id,
            # pyre-fixme[6]: Expected `PortType` for 7th param but got `int`.
            port_type=int(port_type),
            # pyre-fixme[6]: Expected `Scope` for 8th param but got `int`.
            scope=int(scope),
            parent_port_id=parent_port_id,
        )
    return PortProfileMapping(ports=ports)


def read_profile_settings(directory: Dict[str, str], prefix: str) -> ProfileSettings:
    PROFILE_SETTINGS_SUFFIX = "_profile_settings.csv"
    Column = column_int_enum_generator(
        "PORT_SPEED_MBPS A_CHIP_TYPE Z_CHIP_TYPE NUM_LANES MODULATION FEC MEDIA_TYPE A_INTERFACE_TYPE Z_INTERFACE_TYPE"
    )
    profiles = []
    for index, line in enumerate(
        get_content(directory, prefix + PROFILE_SETTINGS_SUFFIX).splitlines()
    ):
        if index < 1:
            # Skip the header
            continue
        row = line.split(",")
        speed = int(row[Column.PORT_SPEED_MBPS])
        if speed not in PortSpeed._VALUES_TO_NAMES:
            raise Exception("Invalid speed ", speed)
        a_chip_type = ChipType._NAMES_TO_VALUES[row[Column.A_CHIP_TYPE]]
        if row[Column.Z_CHIP_TYPE]:
            z_chip_type = ChipType._NAMES_TO_VALUES[row[Column.Z_CHIP_TYPE]]
        else:
            z_chip_type = None
        num_lanes = int(row[Column.NUM_LANES])
        modulation = IpModulation._NAMES_TO_VALUES[row[Column.MODULATION]]
        fec = FecMode._NAMES_TO_VALUES[row[Column.FEC]]
        media_type = TransmitterTechnology._NAMES_TO_VALUES[row[Column.MEDIA_TYPE]]
        a_interface_type = (
            InterfaceType._NAMES_TO_VALUES[row[Column.A_INTERFACE_TYPE]]
            if row[Column.A_INTERFACE_TYPE]
            else InterfaceType.NONE
        )
        z_interface_type = (
            InterfaceType._NAMES_TO_VALUES[row[Column.Z_INTERFACE_TYPE]]
            if row[Column.Z_INTERFACE_TYPE]
            else InterfaceType.NONE
        )
        profiles.append(
            SpeedSetting(
                speed=speed,
                a_chip_settings=ChipSetting(
                    chip_type=a_chip_type, chip_interface_type=a_interface_type
                ),
                z_chip_settings=ChipSetting(
                    chip_type=z_chip_type,
                    chip_interface_type=z_interface_type,
                ),
                num_lanes=num_lanes,
                modulation=modulation,
                fec=fec,
                media_type=media_type,
            )
        )
    return ProfileSettings(speed_settings=profiles)


def read_si_settings(
    directory: Dict[str, str], prefix: str, version: Optional[str] = None
) -> SiSettings:
    si_suffix = f"_{version}" if version is not None else ""
    SI_SETTINGS_SUFFIX = f"_si_settings{si_suffix}.csv"

    si_settings = []
    Column = None
    column_names = ""
    for index, line in enumerate(
        get_content(directory, prefix + SI_SETTINGS_SUFFIX).splitlines()
    ):
        row = line.split(",")
        if index < 1:
            column_names = " ".join(f"{name}" for name in row)
            column_names = column_names.replace("(mbps)", "")
            column_names = column_names.replace("(m)", "")
            Column = column_int_enum_generator(column_names)
            continue
        chip = Chip(
            slot_id=int(row[Column.SLOT_ID]),
            chip_id=int(row[Column.CHIP_ID]),
            chip_type=ChipType._NAMES_TO_VALUES[row[Column.CHIP_TYPE]],
            core_type=CoreType._NAMES_TO_VALUES[row[Column.CORE_TYPE]],
            core_id=int(row[Column.CORE_ID]),
        )
        pin_connection = SiSettingPinConnection(
            chip=chip,
            logical_lane_id=int(row[Column.CORE_LANE]),
        )

        lane_speed = None
        if row[Column.LANE_SPEED]:
            if int(row[Column.LANE_SPEED]) not in PortSpeed._VALUES_TO_NAMES:
                raise Exception("Invalid speed ", row[Column.LANE_SPEED])
            lane_speed = int(row[Column.LANE_SPEED])

        media_type = None
        if row[Column.MEDIA_TYPE]:
            if row[Column.MEDIA_TYPE] not in TransmitterTechnology._NAMES_TO_VALUES:
                raise Exception("Invalid media type ", row[Column.MEDIA_TYPE])
            media_type = TransmitterTechnology._NAMES_TO_VALUES[row[Column.MEDIA_TYPE]]

        cable_length = None
        if row[Column.CABLE_LENGTH]:
            cable_length = float(row[Column.MEDIA_TYPE])

        # Add optics settings if present in the csv file.
        tcvr_setting = None
        tcvr_vendor = row[Column.TCVR_VENDOR]
        if tcvr_vendor:
            tcvr_media = row[Column.TCVR_MEDIA]
            if not tcvr_media:
                raise Exception(
                    "Invalid media media type not populated ", row[Column.TCVR_VENDOR]
                )
            if tcvr_media not in MediaInterfaceCode._NAMES_TO_VALUES:
                raise Exception("Invalid module media type ", tcvr_media)

            tcvr_part_num = row[Column.TCVR_PART_NUM]
            if not tcvr_media:
                raise Exception(
                    "Invalid transceiver part number type not populated ",
                    row[Column.TCVR_PART_NUM],
                )
            tcvr_media_type = MediaInterfaceCode._NAMES_TO_VALUES[tcvr_media]
            vendor = Vendor(
                name=str(row[Column.TCVR_VENDOR]), partNumber=str(tcvr_part_num)
            )
            tcvr_setting = TransceiverOverrideSetting(
                vendor=vendor, media_interface_code=tcvr_media_type
            )
        si_setting_factor = SiSettingFactor(
            # pyre-fixme[6]: Expected `Optional[PortSpeed]` for 1st param but got `Optional[int]`.
            lane_speed=lane_speed,
            media_type=media_type,
            cable_length=cable_length,
            tcvr_override_setting=tcvr_setting,
        )

        tx_setting = TxSettings()
        if chip.core_type == CoreType.G200:
            if "TX_PRE3" in column_names and row[Column.TX_PRE3]:
                tx_setting.firPre3 = int(row[Column.TX_PRE3])
            if "TX_PRE2" in column_names and row[Column.TX_PRE2]:
                tx_setting.firPre2 = int(row[Column.TX_PRE2])
            if "TX_PRE1" in column_names and row[Column.TX_PRE1]:
                tx_setting.firPre1 = int(row[Column.TX_PRE1])
            if "TX_MAIN" in column_names and row[Column.TX_MAIN]:
                tx_setting.firMain = int(row[Column.TX_MAIN])
            if "TX_POST1" in column_names and row[Column.TX_POST1]:
                tx_setting.firPost1 = int(row[Column.TX_POST1])
            if "TX_POST2" in column_names and row[Column.TX_POST2]:
                tx_setting.firPost2 = int(row[Column.TX_POST2])
            if "TX_POST3" in column_names and row[Column.TX_POST3]:
                tx_setting.firPost3 = int(row[Column.TX_POST3])
        else:
            if "TX_PRE3" in column_names and row[Column.TX_PRE3]:
                tx_setting.pre3 = int(row[Column.TX_PRE3])
            if "TX_PRE2" in column_names and row[Column.TX_PRE2]:
                tx_setting.pre2 = int(row[Column.TX_PRE2])
            if "TX_PRE1" in column_names and row[Column.TX_PRE1]:
                tx_setting.pre = int(row[Column.TX_PRE1])
            if "TX_MAIN" in column_names and row[Column.TX_MAIN]:
                tx_setting.main = int(row[Column.TX_MAIN])
            if "TX_POST1" in column_names and row[Column.TX_POST1]:
                tx_setting.post = int(row[Column.TX_POST1])
            if "TX_POST2" in column_names and row[Column.TX_POST2]:
                tx_setting.post2 = int(row[Column.TX_POST2])
            if "TX_POST3" in column_names and row[Column.TX_POST3]:
                tx_setting.post3 = int(row[Column.TX_POST3])
        if "TX_DIFF_ENCODER_EN" in column_names and row[Column.TX_DIFF_ENCODER_EN]:
            tx_setting.diffEncoderEn = int(row[Column.TX_DIFF_ENCODER_EN])
        if "TX_DIG_GAIN" in column_names and row[Column.TX_DIG_GAIN]:
            tx_setting.digGain = int(row[Column.TX_DIG_GAIN])
        if "TX_DRIVER_SWING" in column_names and row[Column.TX_DRIVER_SWING]:
            tx_setting.driverSwing = int(row[Column.TX_DRIVER_SWING])
        if "TX_DRIVE_CURRENT" in column_names and row[Column.TX_DRIVE_CURRENT]:
            tx_setting.driveCurrent = int(row[Column.TX_DRIVE_CURRENT])
        if "TX_FFE_COEFF_0" in column_names and row[Column.TX_FFE_COEFF_0]:
            tx_setting.ffeCoeff0 = int(row[Column.TX_FFE_COEFF_0])
        if "TX_FFE_COEFF_1" in column_names and row[Column.TX_FFE_COEFF_1]:
            tx_setting.ffeCoeff1 = int(row[Column.TX_FFE_COEFF_1])
        if "TX_FFE_COEFF_2" in column_names and row[Column.TX_FFE_COEFF_2]:
            tx_setting.ffeCoeff2 = int(row[Column.TX_FFE_COEFF_2])
        if "TX_FFE_COEFF_3" in column_names and row[Column.TX_FFE_COEFF_3]:
            tx_setting.ffeCoeff3 = int(row[Column.TX_FFE_COEFF_3])
        if "TX_FFE_COEFF_4" in column_names and row[Column.TX_FFE_COEFF_4]:
            tx_setting.ffeCoeff4 = int(row[Column.TX_FFE_COEFF_4])
        if "TX_INNER_EYE_NEG" in column_names and row[Column.TX_INNER_EYE_NEG]:
            tx_setting.innerEyeNeg = int(row[Column.TX_INNER_EYE_NEG])
        if "TX_INNER_EYE_POS" in column_names and row[Column.TX_INNER_EYE_POS]:
            tx_setting.innerEyePos = int(row[Column.TX_INNER_EYE_POS])
        if "TX_LDO_BYPASS" in column_names and row[Column.TX_LDO_BYPASS]:
            tx_setting.ldoBypass = int(row[Column.TX_LDO_BYPASS])
        if "TX_FFE_COEFF_5" in column_names and row[Column.TX_FFE_COEFF_5]:
            tx_setting.ffeCoeff5 = int(row[Column.TX_FFE_COEFF_5])

        rx_setting = RxSettings()
        if "RX_CTLE_CODE" in column_names and row[Column.RX_CTLE_CODE]:
            rx_setting.ctlCode = int(row[Column.RX_CTLE_CODE])
        if "RX_DSP_MODE" in column_names and row[Column.RX_DSP_MODE]:
            rx_setting.dspMode = int(row[Column.RX_DSP_MODE])
        if "RX_AFE_TRIM" in column_names and row[Column.RX_AFE_TRIM]:
            rx_setting.afeTrim = int(row[Column.RX_AFE_TRIM])
        if "RX_INSTG_BOOST1_STRT" in column_names and row[Column.RX_INSTG_BOOST1_STRT]:
            rx_setting.instgBoost1Start = int(row[Column.RX_INSTG_BOOST1_STRT])
        if "RX_INSTG_BOOST1_STEP" in column_names and row[Column.RX_INSTG_BOOST1_STEP]:
            rx_setting.instgBoost1Step = int(row[Column.RX_INSTG_BOOST1_STEP])
        if "RX_INSTG_BOOST1_STOP" in column_names and row[Column.RX_INSTG_BOOST1_STOP]:
            rx_setting.instgBoost1Stop = int(row[Column.RX_INSTG_BOOST1_STOP])
        if (
            "RX_INSTG_BOOST2_OR_HR_STRT" in column_names
            and row[Column.RX_INSTG_BOOST2_OR_HR_STRT]
        ):
            rx_setting.instgBoost2OrHrStart = int(
                row[Column.RX_INSTG_BOOST2_OR_HR_STRT]
            )
        if (
            "RX_INSTG_BOOST2_OR_HR_STEP" in column_names
            and row[Column.RX_INSTG_BOOST2_OR_HR_STEP]
        ):
            rx_setting.instgBoost2OrHrStep = int(row[Column.RX_INSTG_BOOST2_OR_HR_STEP])
        if (
            "RX_INSTG_BOOST2_OR_HR_STOP" in column_names
            and row[Column.RX_INSTG_BOOST2_OR_HR_STOP]
        ):
            rx_setting.instgBoost2OrHrStop = int(row[Column.RX_INSTG_BOOST2_OR_HR_STOP])
        if (
            "RX_INSTG_C1_START_1P7" in column_names
            and row[Column.RX_INSTG_C1_START_1P7]
        ):
            rx_setting.instgC1Start1p7 = int(row[Column.RX_INSTG_C1_START_1P7])
        if "RX_INSTG_C1_STEP_1P7" in column_names and row[Column.RX_INSTG_C1_STEP_1P7]:
            rx_setting.instgC1Step1p7 = int(row[Column.RX_INSTG_C1_STEP_1P7])
        if "RX_INSTG_C1_STOP_1P7" in column_names and row[Column.RX_INSTG_C1_STOP_1P7]:
            rx_setting.instgC1Stop1p7 = int(row[Column.RX_INSTG_C1_STOP_1P7])
        if (
            "RX_INSTG_DFE_START_1P7" in column_names
            and row[Column.RX_INSTG_DFE_START_1P7]
        ):
            rx_setting.instgDfeStart1p7 = int(row[Column.RX_INSTG_DFE_START_1P7])
        if (
            "RX_INSTG_DFE_STEP_1P7" in column_names
            and row[Column.RX_INSTG_DFE_STEP_1P7]
        ):
            rx_setting.instgDfeStep1p7 = int(row[Column.RX_INSTG_DFE_STEP_1P7])
        if (
            "RX_INSTG_DFE_STOP_1P7" in column_names
            and row[Column.RX_INSTG_DFE_STOP_1P7]
        ):
            rx_setting.instgDfeStop1p7 = int(row[Column.RX_INSTG_DFE_STOP_1P7])
        if "RX_DIFF_ENCODER_EN" in column_names and row[Column.RX_DIFF_ENCODER_EN]:
            rx_setting.diffEncoderEn = int(row[Column.RX_DIFF_ENCODER_EN])
        if (
            "RX_ENABLE_SCAN_SELECTION" in column_names
            and row[Column.RX_ENABLE_SCAN_SELECTION]
        ):
            rx_setting.enableScanSelection = int(row[Column.RX_ENABLE_SCAN_SELECTION])
        if (
            "RX_INSTG_SCAN_USE_SR_SETTINGS" in column_names
            and row[Column.RX_INSTG_SCAN_USE_SR_SETTINGS]
        ):
            rx_setting.instgScanUseSrSettings = int(
                row[Column.RX_INSTG_SCAN_USE_SR_SETTINGS]
            )
        if "RX_CDR_CFG_OV_EN" in column_names and row[Column.RX_CDR_CFG_OV_EN]:
            rx_setting.cdrCfgOvEn = int(row[Column.RX_CDR_CFG_OV_EN])
        if (
            "RX_CDR_TDET_1ST_ORD_STEP_OV_VAL" in column_names
            and row[Column.RX_CDR_TDET_1ST_ORD_STEP_OV_VAL]
        ):
            rx_setting.cdrTdet1stOrdStepOvVal = int(
                row[Column.RX_CDR_TDET_1ST_ORD_STEP_OV_VAL]
            )
        if (
            "RX_CDR_TDET_2ND_ORD_STEP_OV_VAL" in column_names
            and row[Column.RX_CDR_TDET_2ND_ORD_STEP_OV_VAL]
        ):
            rx_setting.cdrTdet2ndOrdStepOvVal = int(
                row[Column.RX_CDR_TDET_2ND_ORD_STEP_OV_VAL]
            )
        if (
            "RX_CDR_TDET_FINE_STEP_OV_VAL" in column_names
            and row[Column.RX_CDR_TDET_FINE_STEP_OV_VAL]
        ):
            rx_setting.cdrTdetFineStepOvVal = int(
                row[Column.RX_CDR_TDET_FINE_STEP_OV_VAL]
            )
        if "RX_LDO_BYPASS" in column_names and row[Column.RX_LDO_BYPASS]:
            rx_setting.ldoBypass = int(row[Column.RX_LDO_BYPASS])
        if "RX_FFE_LENGTH_BITMAP" in column_names and row[Column.RX_FFE_LENGTH_BITMAP]:
            rx_setting.ffeLengthBitmap = int(row[Column.RX_FFE_LENGTH_BITMAP])
        if "RX_INSTG_ENABLE_SCAN" in column_names and row[Column.RX_INSTG_ENABLE_SCAN]:
            rx_setting.instgEnableScan = int(row[Column.RX_INSTG_ENABLE_SCAN])
        if "RX_DCW_EN" in column_names and row[Column.RX_DCW_EN]:
            rx_setting.dcwEn = int(row[Column.RX_DCW_EN])
        if (
            "RX_DCW_STEP_COARSE_OV_VAL" in column_names
            and row[Column.RX_DCW_STEP_COARSE_OV_VAL]
        ):
            rx_setting.dcwStepCoarseOvVal = int(row[Column.RX_DCW_STEP_COARSE_OV_VAL])
        if (
            "RX_DCW_STEP_FINE_OV_VAL" in column_names
            and row[Column.RX_DCW_STEP_FINE_OV_VAL]
        ):
            rx_setting.dcwStepFineOvVal = int(row[Column.RX_DCW_STEP_FINE_OV_VAL])
        if "RX_DCW_OV_EN" in column_names and row[Column.RX_DCW_OV_EN]:
            rx_setting.dcwOvEn = int(row[Column.RX_DCW_OV_EN])
        if (
            "RX_FFE_LMS_DYNAMIC_GATING_EN" in column_names
            and row[Column.RX_FFE_LMS_DYNAMIC_GATING_EN]
        ):
            rx_setting.ffeLmsDynamicGatingEn = int(
                row[Column.RX_FFE_LMS_DYNAMIC_GATING_EN]
            )

        si_settings.append(
            SiSettingRow(
                pin_connection=pin_connection,
                factor=si_setting_factor,
                tx_setting=tx_setting,
                rx_setting=rx_setting,
            )
        )

    return SiSettings(si_settings=si_settings)


def read_asic_vendor_config(directory: Dict[str, str], prefix: str) -> AsicVendorConfig:
    VENDOR_CONFIG_SUFFIX = "_vendor_config.json"
    asic_vendor_config_json_str = get_content(directory, prefix + VENDOR_CONFIG_SUFFIX)
    asic_vendor_config_json = json.loads(asic_vendor_config_json_str)

    def stringify_map_values(map: Dict[str, Any]) -> Dict[str, str]:
        return {key: json.dumps(value) for key, value in map.items()}

    common_config = copy.deepcopy(asic_vendor_config_json["config"]["common_config"])
    prod_config = copy.deepcopy(asic_vendor_config_json["config"]["prod_config_only"])
    hw_test_config = copy.deepcopy(
        asic_vendor_config_json["config"]["hw_test_config_only"]
    )
    link_test_config = copy.deepcopy(
        asic_vendor_config_json["config"]["link_test_config_only"]
    )
    benchmark_config = copy.deepcopy(
        asic_vendor_config_json["config"]["benchmark_config_only"]
    )
    port_map_config = copy.deepcopy(
        asic_vendor_config_json["config"]["port_map_config"]
    )
    if "multistage_config" in asic_vendor_config_json["config"]:
        multistage_config = copy.deepcopy(
            asic_vendor_config_json["config"]["multistage_config"]
        )
    else:
        multistage_config = {}
    asic_vendor_config_params = AsicVendorConfigParams(
        commonConfig=stringify_map_values(common_config),
        prodConfig=stringify_map_values(prod_config),
        hwTestConfig=stringify_map_values(hw_test_config),
        linkTestConfig=stringify_map_values(link_test_config),
        benchmarkConfig=stringify_map_values(benchmark_config),
        portMapConfig=stringify_map_values(port_map_config),
        multistageConfig=stringify_map_values(multistage_config),
    )

    return AsicVendorConfig(asic_vendor_config_params=asic_vendor_config_params)
