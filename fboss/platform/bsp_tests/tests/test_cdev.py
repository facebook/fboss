import fcntl
import os
from time import sleep

import pytest

from fboss.platform.bsp_tests.test_runner import TestBase

from fboss.platform.bsp_tests.utils.cdev_types import getEmptyAuxData, getInvalidAuxData
from fboss.platform.bsp_tests.utils.cdev_utils import (
    create_new_device,
    delete_device,
    FBIOB_IOC_DEL_DEVICE,
    FBIOB_IOC_NEW_DEVICE,
    make_cdev_path,
)


class TestCdev(TestBase):

    def test_cdev_is_created(self, platform_fpgas) -> None:
        for fpga in platform_fpgas:
            path = make_cdev_path(fpga)
            assert os.path.exists(path)

    def test_empty_aux_data(self, platform_fpgas) -> None:
        """
        Tests that cdev file is behaving appropriately in
        a failure case
        """
        for fpga in platform_fpgas:
            aux_data = getEmptyAuxData()
            path = make_cdev_path(fpga)
            fd = os.open(path, os.O_RDWR)
            try:
                res = fcntl.ioctl(fd, FBIOB_IOC_NEW_DEVICE, aux_data)
                assert res == 0
                res = fcntl.ioctl(fd, FBIOB_IOC_DEL_DEVICE, aux_data)
                assert res == 0
            finally:
                os.close(fd)

    def test_cdev_rejects_invalid_data(self, platform_fpgas) -> None:
        """
        Tests that cdev file is behaving appropriately in
        a failure case
        """
        for fpga in platform_fpgas:
            aux_data = getInvalidAuxData()
            path = make_cdev_path(fpga)
            fd = os.open(path, os.O_RDWR)
            with pytest.raises(OSError):
                fcntl.ioctl(fd, FBIOB_IOC_NEW_DEVICE, aux_data)
            os.close(fd)

    def test_cdev_create_and_delete(self, platform_fpgas) -> None:
        """
        Tests that ioctl commands to create and delete
        a device work
        """
        for fpga in platform_fpgas:
            for device in fpga.auxDevices:
                create_new_device(fpga, device, 1)
                sleep(1)
                delete_device(fpga, device, 1)
