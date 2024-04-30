import fcntl
import os
from time import sleep
from typing import List

import pytest

from fboss.platform.bsp_tests.cdev_types import (
    fbiob_aux_data,
    getAuxData,
    getEmptyAuxData,
    getInvalidAuxData,
)

from fboss.platform.bsp_tests.test_runner import FpgaSpec, TestBase
from ioctl_opt import IOW


FBIOB_IOC_MAGIC = ord("F")
FBIOB_IOC_NEW_DEVICE = IOW(FBIOB_IOC_MAGIC, 1, fbiob_aux_data)
FBIOB_IOC_DEL_DEVICE = IOW(FBIOB_IOC_MAGIC, 2, fbiob_aux_data)


class TestCdev(TestBase):
    fpgas: List[FpgaSpec] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.fpgas = cls.config.fpgas

    def setup_method(self):
        self.load_kmods()

    def test_cdev_is_created(self) -> None:
        for fpga in self.fpgas:
            path = self.make_cdev_path(fpga)
            assert os.path.exists(path)

    def test_empty_aux_data(self) -> None:
        """
        Tests that cdev file is behaving appropriately in
        a failure case
        """
        for fpga in self.fpgas:
            aux_data = getEmptyAuxData()
            path = self.make_cdev_path(fpga)
            fd = os.open(path, os.O_RDWR)
            res = fcntl.ioctl(fd, FBIOB_IOC_NEW_DEVICE, aux_data)
            assert res == 0
            res = fcntl.ioctl(fd, FBIOB_IOC_DEL_DEVICE, aux_data)
            os.close(fd)
            assert res == 0

    def test_cdev_rejects_invalid_data(self) -> None:
        """
        Tests that cdev file is behaving appropriately in
        a failure case
        """
        for fpga in self.fpgas:
            aux_data = getInvalidAuxData()
            path = self.make_cdev_path(fpga)
            fd = os.open(path, os.O_RDWR)
            with pytest.raises(OSError):
                fcntl.ioctl(fd, FBIOB_IOC_NEW_DEVICE, aux_data)
            os.close(fd)

    def test_cdev_create_and_delete(self) -> None:
        """
        Tests that ioctl commands to create and delete
        a device work
        """
        for fpga in self.fpgas:
            for device in fpga.testAuxDevices:
                aux_data = getAuxData(fpga, device, 1)
                path = self.make_cdev_path(fpga)
                fd = os.open(path, os.O_RDWR)
                res = fcntl.ioctl(fd, FBIOB_IOC_NEW_DEVICE, aux_data)
                assert res == 0
                sleep(1)
                res = fcntl.ioctl(fd, FBIOB_IOC_DEL_DEVICE, aux_data)
                os.close(fd)
                assert res == 0

    def make_cdev_path(self, fpga: FpgaSpec) -> str:
        # strip the leading 0x
        vendor = fpga.vendorId[2:]
        device = fpga.deviceId[2:]
        subsystemVendor = fpga.subSystemVendorId[2:]
        subsystemDevice = fpga.subSystemDeviceId[2:]
        return f"/dev/fbiob_{vendor}.{device}.{subsystemVendor}.{subsystemDevice}"
