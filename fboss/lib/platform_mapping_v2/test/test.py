#!/usr/bin/env python3

# pyre-strict

import unittest
from typing import Dict, List

from fboss.lib.platform_mapping_v2.gen import read_vendor_data
from fboss.lib.platform_mapping_v2.platform_mapping_v2 import PlatformMappingV2

from neteng.fboss.phy.ttypes import (
    DataPlanePhyChip,
    DataPlanePhyChipType,
    FecMode,
    InterfaceType,
    IpModulation,
    Pin,
    PinConfig,
    PinConnection,
    PinID,
    PortPinConfig,
    PortProfileConfig,
    ProfileSideConfig,
)
from neteng.fboss.platform_config.ttypes import (
    PlatformPortConfig,
    PlatformPortConfigFactor,
    PlatformPortEntry,
    PlatformPortMapping,
    PlatformPortProfileConfigEntry,
)
from neteng.fboss.switch_config.ttypes import PortProfileID, PortSpeed, PortType, Scope
from neteng.fboss.transceiver.ttypes import TransmitterTechnology


class TestPlatformMappingGeneration(unittest.TestCase):
    def _get_test_vendor_data(self) -> Dict[str, Dict[str, str]]:
        input_dir = "fboss/lib/platform_mapping_v2/test/test_data"
        return {"test": read_vendor_data(input_dir)}

    def _get_expected_single_npu_test_ports(self) -> Dict[int, PlatformPortEntry]:
        port_one_mapping = PlatformPortEntry(
            mapping=PlatformPortMapping(
                id=1,
                name="eth1/2/1",
                controllingPort=1,
                pins=[
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=0),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=0)),
                    ),
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=1),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=1)),
                    ),
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=2),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=2)),
                    ),
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=3),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=3)),
                    ),
                ],
                portType=PortType.INTERFACE_PORT,
                scope=Scope.LOCAL,
            ),
            supportedProfiles={
                PortProfileID.PROFILE_100G_4_NRZ_RS528_OPTICAL: PlatformPortConfig(
                    subsumedPorts=[2],
                    pins=PortPinConfig(
                        iphy=[
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=0)
                            ),
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=1)
                            ),
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=2)
                            ),
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=3)
                            ),
                        ],
                        transceiver=[
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=0)
                            ),
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=1)
                            ),
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=2)
                            ),
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=3)
                            ),
                        ],
                    ),
                ),
                PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL: PlatformPortConfig(
                    pins=PortPinConfig(
                        iphy=[
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=0)
                            )
                        ],
                        transceiver=[
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=0)
                            )
                        ],
                    )
                ),
            },
        )
        port_two_mapping = PlatformPortEntry(
            mapping=PlatformPortMapping(
                id=2,
                name="eth1/2/2",
                controllingPort=1,
                pins=[
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=1),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=1)),
                    )
                ],
                portType=PortType.INTERFACE_PORT,
                scope=Scope.LOCAL,
            ),
            supportedProfiles={
                PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL: PlatformPortConfig(
                    pins=PortPinConfig(
                        iphy=[
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip1/core0", lane=1)
                            )
                        ],
                        transceiver=[
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip2", lane=1)
                            )
                        ],
                    )
                )
            },
        )
        return {1: port_one_mapping, 2: port_two_mapping}

    def _get_expected_multi_npu_test_ports(self) -> Dict[int, PlatformPortEntry]:
        multi_npu_port_one = PlatformPortEntry(
            mapping=PlatformPortMapping(
                id=102,
                name="eth1/3/1",
                controllingPort=102,
                pins=[
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=0),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=0)),
                    ),
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=1),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=1)),
                    ),
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=2),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=2)),
                    ),
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=3),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=3)),
                    ),
                ],
                portType=PortType.INTERFACE_PORT,
                scope=Scope.LOCAL,
            ),
            supportedProfiles={
                PortProfileID.PROFILE_100G_4_NRZ_RS528_OPTICAL: PlatformPortConfig(
                    subsumedPorts=[103],
                    pins=PortPinConfig(
                        iphy=[
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=0)
                            ),
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=1)
                            ),
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=2)
                            ),
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=3)
                            ),
                        ],
                        transceiver=[
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=0)
                            ),
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=1)
                            ),
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=2)
                            ),
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=3)
                            ),
                        ],
                    ),
                ),
                PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL: PlatformPortConfig(
                    pins=PortPinConfig(
                        iphy=[
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=0)
                            )
                        ],
                        transceiver=[
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=0)
                            )
                        ],
                    )
                ),
            },
        )
        multi_npu_port_two = PlatformPortEntry(
            mapping=PlatformPortMapping(
                id=103,
                name="eth1/3/2",
                controllingPort=102,
                pins=[
                    PinConnection(
                        a=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=1),
                        z=Pin(end=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=1)),
                    )
                ],
                portType=PortType.INTERFACE_PORT,
                scope=Scope.LOCAL,
            ),
            supportedProfiles={
                PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL: PlatformPortConfig(
                    pins=PortPinConfig(
                        iphy=[
                            PinConfig(
                                id=PinID(chip="NPU-TH5_NIF-slot1/chip2/core0", lane=1)
                            )
                        ],
                        transceiver=[
                            PinConfig(
                                id=PinID(chip="TRANSCEIVER-OSFP-slot1/chip3", lane=1)
                            )
                        ],
                    )
                )
            },
        )
        return self._get_expected_single_npu_test_ports() | {
            102: multi_npu_port_one,
            103: multi_npu_port_two,
        }

    def _get_expected_single_npu_test_chips(self) -> List[DataPlanePhyChip]:
        chip_one = DataPlanePhyChip(
            name="NPU-TH5_NIF-slot1/chip1/core0",
            type=DataPlanePhyChipType.IPHY,
            physicalID=0,
        )
        chip_two = DataPlanePhyChip(
            name="TRANSCEIVER-OSFP-slot1/chip2",
            type=DataPlanePhyChipType.TRANSCEIVER,
            physicalID=1,
        )
        chip_three = DataPlanePhyChip(
            name="TRANSCEIVER-OSFP-slot1/chip3",
            type=DataPlanePhyChipType.TRANSCEIVER,
            physicalID=2,
        )
        return [chip_one, chip_two, chip_three]

    def _get_expected_multi_npu_test_chips(self) -> List[DataPlanePhyChip]:
        return self._get_expected_single_npu_test_chips() + [
            DataPlanePhyChip(
                name="NPU-TH5_NIF-slot1/chip2/core0",
                type=DataPlanePhyChipType.IPHY,
                physicalID=0,
            )
        ]

    def _get_expected_single_npu_supported_profiles(
        self,
    ) -> List[PlatformPortProfileConfigEntry]:
        entry_one = PlatformPortProfileConfigEntry(
            factor=PlatformPortConfigFactor(
                profileID=PortProfileID.PROFILE_100G_4_NRZ_RS528_OPTICAL
            ),
            profile=PortProfileConfig(
                speed=PortSpeed.HUNDREDG,
                iphy=ProfileSideConfig(
                    numLanes=4,
                    modulation=IpModulation.NRZ,
                    fec=FecMode.RS528,
                    medium=TransmitterTechnology.OPTICAL,
                    interfaceType=InterfaceType.SR4,
                ),
            ),
        )
        entry_two = PlatformPortProfileConfigEntry(
            factor=PlatformPortConfigFactor(
                profileID=PortProfileID.PROFILE_100G_1_PAM4_RS544_OPTICAL
            ),
            profile=PortProfileConfig(
                speed=PortSpeed.HUNDREDG,
                iphy=ProfileSideConfig(
                    numLanes=1,
                    modulation=IpModulation.PAM4,
                    fec=FecMode.RS544_2N,
                    medium=TransmitterTechnology.BACKPLANE,
                    interfaceType=InterfaceType.KR4,
                ),
            ),
        )
        return [entry_one, entry_two]

    def test_get_platform_mapping_single_npu(self) -> None:
        platform_mapping = PlatformMappingV2(
            self._get_test_vendor_data(), "test", multi_npu=False
        )

        # Verify ports
        self.assertEqual(
            platform_mapping.get_platform_port_map(),
            self._get_expected_single_npu_test_ports(),
        )

        # Verify chips
        chips = platform_mapping.get_chips()
        self.assertEqual(len(chips), 3)
        self.assertTrue(
            all(chip in chips for chip in self._get_expected_single_npu_test_chips())
        )

        # Verify supportedProfiles
        supportedProfiles = platform_mapping.get_platform_profiles()
        self.assertEqual(len(supportedProfiles), 2)
        self.assertTrue(
            all(
                entry in supportedProfiles
                for entry in self._get_expected_single_npu_supported_profiles()
            )
        )

    def test_get_platform_mapping_multi_npu(self) -> None:
        platform_mapping = PlatformMappingV2(
            self._get_test_vendor_data(), "test", multi_npu=True
        )

        # Verify ports
        self.assertEqual(
            platform_mapping.get_platform_port_map(),
            self._get_expected_multi_npu_test_ports(),
        )

        # Verify chips
        chips = platform_mapping.get_chips()
        self.assertEqual(len(chips), 4)
        self.assertTrue(
            all(chip in chips for chip in self._get_expected_multi_npu_test_chips())
        )

        # Verify supportedProfiles (supportedProfiles should be the same for both single and multi NPU)
        supportedProfiles = platform_mapping.get_platform_profiles()
        self.assertEqual(len(supportedProfiles), 2)
        self.assertTrue(
            all(
                entry in supportedProfiles
                for entry in self._get_expected_single_npu_supported_profiles()
            )
        )


def main() -> None:
    unittest.main()
