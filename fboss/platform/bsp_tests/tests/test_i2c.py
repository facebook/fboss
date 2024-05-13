import os
from typing import List

from fboss.platform.bsp_tests.test_runner import FpgaSpec, TestBase

from fboss.platform.bsp_tests.utils.cdev_utils import delete_device, make_cdev_path
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    detect_i2c_device,
)


class TestI2c(TestBase):
    fpgas: List[FpgaSpec] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.fpgas = cls.config.fpgas

    def setup_method(self):
        self.load_kmods()

    def test_cdev_is_created(self) -> None:
        for fpga in self.fpgas:
            path = make_cdev_path(fpga)
            assert os.path.exists(path)

    def test_i2c_adapter_creates_busses(self) -> None:
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                # Creates adapter, checks expected number of busses created
                newAdapters, _ = create_i2c_adapter(fpga, adapter)

                # Check each bus has a unique name
                names = set()
                for a in newAdapters:
                    names.add(a.name)
                assert len(names) == len(newAdapters)
                delete_device(fpga, adapter.auxDevice)

    def test_i2c_adapter_devices_exist(self) -> None:
        """
        Tests that each expected device is detectable
        """

        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                assert adapter.auxDevice.i2cInfo
                # record the current existing busses
                newAdapters, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)

                for device in adapter.i2cDevices:
                    print(
                        f"Checking for device {device.address} on bus {adapterBaseBusNum + device.channel}"
                    )
                    assert detect_i2c_device(
                        adapterBaseBusNum + device.channel, device.address
                    )
                delete_device(fpga, adapter.auxDevice)
