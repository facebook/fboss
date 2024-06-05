from typing import List

import sensors

from fboss.platform.bsp_tests.test_runner import TestBase
from fboss.platform.bsp_tests.utils.cdev_types import FpgaSpec
from fboss.platform.bsp_tests.utils.cdev_utils import delete_device
from fboss.platform.bsp_tests.utils.cmd_utils import check_cmd
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
)


class TestHwmon(TestBase):
    fpgas: List[FpgaSpec] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.fpgas = cls.config.fpgas

        # TODO: Remove this once `sensors` is installed on lab
        # devices by default
        check_cmd(["dnf", "install", "lm_sensors", "-y"])

    def setup_method(self):
        self.load_kmods()

    def test_hwmon_sensors(self) -> None:
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                for i2cdevice in adapter.i2cDevices:
                    if not i2cdevice.hwmonTestData:
                        continue
                    try:
                        testData = i2cdevice.hwmonTestData
                        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
                        busNum = baseBusNum + i2cdevice.channel
                        assert create_i2c_device(
                            i2cdevice, busNum
                        ), f"failed to create device {i2cdevice.address} on bus {busNum}"
                        expectedSensorName = (
                            f"{i2cdevice.deviceName}-i2c-{busNum}-{i2cdevice.address}"
                        )
                        sensors.init()
                        foundSensors = list(
                            sensors.iter_detected_chips(chip_name=expectedSensorName)
                        )
                        assert (
                            len(foundSensors) == 1
                        ), f"Expected to find sensor {expectedSensorName}"
                        featureNames = [feature.name for feature in foundSensors[0]]
                        for feature in testData.expectedFeatures:
                            assert (
                                feature in featureNames
                            ), f"Hwmon device {busNum}-{i2cdevice.address[2:]} Expected to find sensor values: {testData.expectedFeatures}. Got: {featureNames}"
                        sensors.cleanup()
                    finally:
                        delete_device(fpga, adapter.auxDevice)

        for chip in sensors.iter_detected_chips():
            print(f"Chip: {chip}")
