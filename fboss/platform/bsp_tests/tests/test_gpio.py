from typing import List

from fboss.platform.bsp_tests.test_runner import FpgaSpec, TestBase
from fboss.platform.bsp_tests.utils.cdev_utils import delete_device
from fboss.platform.bsp_tests.utils.gpio_utils import gpiodetect
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
)


class TestGpio(TestBase):
    fpgas: List[FpgaSpec] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.fpgas = cls.config.fpgas

    def setup_method(self):
        self.load_kmods()

    def test_gpio_is_detectable(self):
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)

                try:
                    for device in adapter.i2cDevices:
                        if not device.gpioTestData:
                            continue
                        busNum = baseBusNum + device.channel
                        create_i2c_device(device, busNum)
                        expectedLabel = f"{busNum}-{device.address[2:].zfill(4)}"
                        gpios = gpiodetect(expectedLabel)
                        assert (
                            len(gpios) == 1
                        ), f"Expected GPIO not detected: {expectedLabel}"
                        assert (
                            gpios[0].lines == device.gpioTestData.numLines
                        ), f"Expected {device.gpioTestData.numLines}, got: {gpios[0].lines}"
                finally:
                    delete_device(fpga, adapter.auxDevice)

        pass

    def test_gpio_info(self):

        pass

    def test_gpio_set(self):
        pass

    def test_gpio_get(self):
        pass

    def test_driver_unload(self):
        pass
