# pyre-strict
import pytest
from fboss.platform.bsp_tests.cdev_types import I2CAdapter

from fboss.platform.bsp_tests.config import FpgaSpec, RuntimeConfig

from fboss.platform.bsp_tests.utils.cdev_utils import delete_device
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
)
from fboss.platform.bsp_tests.utils.kmod_utils import unload_kmods
from fboss.platform.bsp_tests.utils.watchdog_utils import (
    get_watchdog_timeout,
    get_watchdogs,
    magic_close_watchdog,
    ping_watchdog,
    set_watchdog_timeout,
)


def test_watchdog_start(fpga_with_adapters: list[tuple[FpgaSpec, I2CAdapter]]) -> None:
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
        try:
            for device in adapter.i2cDevices:
                if not device.testData or not device.testData.watchdogTestData:
                    continue
                testData = device.testData.watchdogTestData
                existingWdts = get_watchdogs()
                busNum = baseBusNum + device.channel
                create_i2c_device(device, busNum)
                newWdts = get_watchdogs() - existingWdts
                assert len(newWdts) == testData.numWatchdogs
                for watchdog in newWdts:
                    try:
                        with open(watchdog, "w") as f:
                            timeout = get_watchdog_timeout(f.fileno())
                            assert timeout > 0, "Timeout expected to be greater than 0"
                    except Exception as e:
                        pytest.fail(f"Failed to get timeout for {watchdog}: {e}")
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_watchdog_ping(fpga_with_adapters: list[tuple[FpgaSpec, I2CAdapter]]) -> None:
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
        try:
            for device in adapter.i2cDevices:
                if not device.testData or not device.testData.watchdogTestData:
                    continue
                testData = device.testData.watchdogTestData
                existingWdts = get_watchdogs()
                busNum = baseBusNum + device.channel
                create_i2c_device(device, busNum)
                newWdts = get_watchdogs() - existingWdts
                assert len(newWdts) == testData.numWatchdogs
                for watchdog in newWdts:
                    try:
                        with open(watchdog, "w") as f:
                            ping_watchdog(f.fileno())
                    except Exception as e:
                        pytest.fail(f"Failed to ping {watchdog}: {e}")
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_watchdog_set_timeout(platform_fpgas: list[FpgaSpec]) -> None:
    for fpga in platform_fpgas:
        for adapter in fpga.i2cAdapters:
            newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
            try:
                for device in adapter.i2cDevices:
                    if not device.testData or not device.testData.watchdogTestData:
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
                                assert t == 10, f"Timeout expected to be 10, got {t}"
                                set_watchdog_timeout(f.fileno(), 100)
                                t = get_watchdog_timeout(f.fileno())
                                assert t == 100, f"Timeout expected to be 100, got {t}"
                        except Exception as e:
                            pytest.fail(f"Failed to set timeout for {watchdog}: {e}")

            finally:
                delete_device(fpga, adapter.auxDevice)


# TODO: We need to add a requirement that the option
# get_timeleft is enabled in order to reliably
# test the magic close feature
def test_watchdog_magic_close(
    fpga_with_adapters: list[tuple[FpgaSpec, I2CAdapter]],
) -> None:
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
        try:
            for device in adapter.i2cDevices:
                if not device.testData or not device.testData.watchdogTestData:
                    continue
                testData = device.testData.watchdogTestData
                existingWdts = get_watchdogs()
                busNum = baseBusNum + device.channel
                create_i2c_device(device, busNum)
                newWdts = get_watchdogs() - existingWdts
                assert len(newWdts) == testData.numWatchdogs
                for watchdog in newWdts:
                    try:
                        with open(watchdog, "w") as f:
                            magic_close_watchdog(f.fileno())
                    except Exception as e:
                        pytest.fail(f"Failed to close {watchdog}: {e}")
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_watchdog_driver_unload(
    fpga_with_adapters: list[tuple[FpgaSpec, I2CAdapter]],
    platform_config: RuntimeConfig,
) -> None:
    existing_wdts = get_watchdogs()
    expected_wdts = 0
    id = 1
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter, id)
        id += 1
        for device in adapter.i2cDevices:
            if not device.testData or not device.testData.watchdogTestData:
                continue
            expected_wdts += device.testData.watchdogTestData.numWatchdogs
            busNum = baseBusNum + device.channel
            create_i2c_device(device, busNum)
    new_wdts = get_watchdogs() - existing_wdts
    assert len(new_wdts) == expected_wdts
    unload_kmods(platform_config.kmods)
    assert len(get_watchdogs()) == len(existing_wdts)
