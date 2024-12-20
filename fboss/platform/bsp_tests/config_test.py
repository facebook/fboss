# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

import importlib.resources
import json
import unittest

from fboss.platform.bsp_tests.cdev_types import (
    AuxDevice,
    AuxDeviceType,
    DeviceTestData,
    FpgaSpec,
    I2CAdapter,
    I2CDevice,
    I2CGetData,
    I2cInfo,
    I2CTestData,
)

from fboss.platform.bsp_tests.config import (
    ConfigLib,
    PMDevice,
    RuntimeConfig,
    TestConfig,
)

from fboss.platform.platform_manager.platform_manager_config.types import (
    BspKmodsFile,
    PlatformConfig,
)

from thrift.py3 import Protocol, serializer


TEST_CONFIG = """
{
    "platform": "meru800bfa",
    "vendor": "arista",
    "testDevices": [
        {
            "pmUnit": "SCM",
            "pmUnitScopedName": "SCM_MPS_PMBUS",
            "deviceTestData": {
                "i2cTestData": {
                    "i2cGetData": [
                        {
                            "reg": "0x19",
                            "expected": "0xd0"
                        }
                    ]
                }
            }
        },
        {
            "pmUnit": "SMB",
            "pmUnitScopedName": "SMB_CPLD",
            "deviceTestData": {
                "i2cTestData": {
                    "i2cGetData": [
                        {
                            "reg": "0x00",
                            "expected": "0x01"
                        }
                    ]
                }
            }
        }
    ]
}
"""

PM_CONFIG = """
{
    "platformName": "meru800bfa",
    "rootPmUnitName": "SCM",
    "rootSlotType": "SCM_SLOT",
    "slotTypeConfigs": {},
    "pmUnitConfigs": {
        "SCM": {
            "pluggedInSlotType": "SCM_SLOT",
            "i2cDeviceConfigs": [
                {
                    "busName": "SCM_I2C_MASTER0@0",
                    "address": "0x40",
                    "kernelDeviceName": "pmbus",
                    "pmUnitScopedName": "SCM_MPS_PMBUS"
                }
            ],
            "outgoingSlotConfigs": {
                "SMB_SLOT@0": {
                    "slotType": "SMB_SLOT",
                    "outgoingI2cBusNames": [
                        "SCM_I2C_MASTER1@0",
                        "SCM_I2C_MASTER1@1",
                        "SCM_I2C_MASTER1@2",
                        "SCM_I2C_MASTER1@3"
                    ]
                }
            },
            "pciDeviceConfigs": [
                {
                    "pmUnitScopedName": "SCM_FPGA",
                    "vendorId": "0x3475",
                    "deviceId": "0x0001",
                    "subSystemVendorId": "0x3475",
                    "subSystemDeviceId": "0x0008",
                    "i2cAdapterConfigs": [
                        {
                            "fpgaIpBlockConfig": {
                                "pmUnitScopedName": "SCM_I2C_MASTER0",
                                "deviceName": "i2c_master",
                                "csrOffset": "0x8000"
                            },
                        "numberOfAdapters": 8
                        },
                        {
                            "fpgaIpBlockConfig": {
                                "pmUnitScopedName": "SCM_I2C_MASTER1",
                                "deviceName": "i2c_master",
                                "csrOffset": "0x8080"
                            },
                            "numberOfAdapters": 8
                        }
                    ],
                    "spiMasterConfigs": [],
                    "ledCtrlConfigs": [],
                    "xcvrCtrlConfigs": [],
                    "infoRomConfigs": []
                }
            ],
            "embeddedSensorConfigs": []
        },
        "SMB": {
            "pluggedInSlotType": "SMB_SLOT",
            "i2cDeviceConfigs": [
                {
                    "busName": "INCOMING@0",
                    "address": "0x23",
                    "kernelDeviceName": "decker_cpld",
                    "pmUnitScopedName": "SMB_CPLD"
                }
            ],
            "outgoingSlotConfigs": {},
            "pciDeviceConfigs": [],
            "embeddedSensorConfigs": []
        }
    },
    "i2cAdaptersFromCpu": [],
    "symbolicLinkToDevicePath": {},
    "bspKmodsRpmName": "arista_bsp_kmods",
    "bspKmodsRpmVersion": "1.0.0",
    "bspKmodsToReload": [],
    "sharedKmodsToReload": [],
    "upstreamKmodsToLoad": []
}
"""

EXPECTED_RUNTIME_CONFIG = RuntimeConfig(
    "meru800bfa",
    "arista",
    [
        PMDevice(
            "SCM",
            "SCM_FPGA",
            FpgaSpec(
                "0x3475",
                "0x0001",
                "0x3475",
                "0x0008",
                [
                    I2CAdapter(
                        "SCM_I2C_MASTER0",
                        AuxDevice(
                            AuxDeviceType.I2C,
                            "i2c_master",
                            "0x8000",
                            i2cInfo=I2cInfo(numChannels=8),
                        ),
                        [
                            I2CDevice(
                                "SCM_MPS_PMBUS",
                                0,
                                "pmbus",
                                "0x40",
                                testData=DeviceTestData(
                                    i2cTestData=I2CTestData(
                                        i2cGetData=[I2CGetData("0x19", "0xd0")]
                                    ),
                                ),
                            ),
                        ],
                    ),
                    I2CAdapter(
                        "SCM_I2C_MASTER1",
                        AuxDevice(
                            AuxDeviceType.I2C,
                            "i2c_master",
                            "0x8080",
                            i2cInfo=I2cInfo(numChannels=8),
                        ),
                        [
                            I2CDevice(
                                "SMB_CPLD",
                                0,
                                "decker_cpld",
                                "0x23",
                                testData=DeviceTestData(
                                    i2cTestData=I2CTestData(
                                        i2cGetData=[I2CGetData("0x00", "0x01")]
                                    ),
                                ),
                            )
                        ],
                    ),
                ],
            ),
        )
    ],
    BspKmodsFile(),
)


class ConfigTest(unittest.TestCase):
    def test_example_configs_are_valid(self) -> None:
        self.assertIsNotNone(
            serializer.deserialize(
                PlatformConfig,
                json.dumps(json.loads(PM_CONFIG)).encode("utf-8"),
                protocol=Protocol.JSON,
            )
        )
        self.assertIsNotNone(TestConfig.from_json(TEST_CONFIG))  # pyre-ignore

    def test_build_runtime_config(self) -> None:
        expected_kmods = BspKmodsFile()
        pm_config = serializer.deserialize(
            PlatformConfig,
            json.dumps(json.loads(PM_CONFIG)).encode("utf-8"),
            protocol=Protocol.JSON,
        )
        test_config = TestConfig.from_json(TEST_CONFIG)  # pyre-ignore
        rt_config = ConfigLib().build_runtime_config(
            test_config, pm_config, expected_kmods
        )
        self.assertEqual(rt_config, EXPECTED_RUNTIME_CONFIG)

    def test_real_config(self) -> None:
        platform_to_vendor = {
            "janga800bic": "fboss",
            "meru800bfa": "arista",
            "meru800bia": "arista",
            "montblanc": "fboss",
            "morgan800cc": "cisco",
            "tahan800bc": "fboss",
        }

        base_package = f"{__package__}.fboss_config_files.configs"

        for platform in platform_to_vendor.keys():
            package_name = f"{base_package}.{platform}"
            try:
                with importlib.resources.open_binary(
                    package_name, "platform_manager.json"
                ) as file:
                    content = file.read().decode("utf-8")
                pm_config = serializer.deserialize(
                    PlatformConfig,
                    json.dumps(json.loads(content)).encode("utf-8"),
                    protocol=Protocol.JSON,
                )

                test_config = TestConfig(
                    platform=platform,
                    vendor=platform_to_vendor[platform],
                    testDevices=[],
                )

                expected_kmods = BspKmodsFile()
                rt_config = ConfigLib().build_runtime_config(
                    test_config, pm_config, expected_kmods
                )
                self.assertIsNotNone(rt_config)
            except Exception as e:
                raise RuntimeError(f"Failed to generate config for {platform}: {e}")
