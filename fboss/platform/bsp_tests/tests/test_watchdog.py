from typing import List

import pytest

from fboss.platform.bsp_tests.test_runner import FpgaSpec, TestBase
from fboss.platform.bsp_tests.utils.cdev_utils import delete_device
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
)
from fboss.platform.bsp_tests.utils.watchdog_utils import (
    get_watchdog_timeout,
    get_watchdogs,
    set_watchdog_timeout,
)


class TestWatchdog(TestBase):
    fpgas: List[FpgaSpec] = []

    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.fpgas = cls.config.fpgas

    def setup_method(self):
        self.load_kmods()

    def test_watchdog_start(self):
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
                try:
                    for device in adapter.i2cDevices:
                        if not device.watchdogTestData:
                            continue
                        existingWdts = get_watchdogs()
                        busNum = baseBusNum + device.channel
                        create_i2c_device(device, busNum)
                        newWdts = get_watchdogs() - existingWdts
                        assert len(newWdts) == device.watchdogTestData.numWatchdogs
                        for watchdog in newWdts:
                            try:
                                with open(watchdog, "w") as f:
                                    timeout = get_watchdog_timeout(f.fileno())
                                    assert (
                                        timeout > 0
                                    ), "Timeout expected to be greater than 0"
                            except Exception as e:
                                pytest.fail(
                                    f"Failed to get timeout for {watchdog}: {e}"
                                )
                finally:
                    delete_device(fpga, adapter.auxDevice)

    def test_watchdog_ping(self):
        pass

    def test_watchdog_set_timeout(self):
        for fpga in self.fpgas:
            for adapter in fpga.i2cAdapters:
                newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
                try:
                    for device in adapter.i2cDevices:
                        if not device.watchdogTestData:
                            continue
                        existingWdts = get_watchdogs()
                        busNum = baseBusNum + device.channel
                        create_i2c_device(device, busNum)
                        newWdts = get_watchdogs() - existingWdts
                        for watchdog in newWdts:
                            try:
                                with open(watchdog, "w") as f:
                                    set_watchdog_timeout(f.fileno(), 10)
                                    t = get_watchdog_timeout(f.fileno())
                                    assert (
                                        t == 10
                                    ), f"Timeout expected to be 10, got {t}"
                                    set_watchdog_timeout(f.fileno(), 100)
                                    t = get_watchdog_timeout(f.fileno())
                                    assert (
                                        t == 100
                                    ), f"Timeout expected to be 100, got {t}"
                            except Exception as e:
                                pytest.fail(
                                    f"Failed to set timeout for {watchdog}: {e}"
                                )

                finally:
                    delete_device(fpga, adapter.auxDevice)

    def test_watchdog_magic_close(self):
        pass

    def test_watchdog_driver_unload(self):
        pass
