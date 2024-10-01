from typing import List, Tuple

import pytest
from fboss.platform.bsp_tests.test_runner import FpgaSpec

from fboss.platform.bsp_tests.utils.cdev_types import I2CAdapter
from fboss.platform.bsp_tests.utils.cdev_utils import delete_device
from fboss.platform.bsp_tests.utils.gpio_utils import gpiodetect, gpioget, gpioinfo
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
)
from fboss.platform.bsp_tests.utils.kmod_utils import unload_kmods


def test_gpio_is_detectable(fpga_with_adapters):
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
        try:
            for device in adapter.i2cDevices:
                if not device.gpioTestData:
                    continue
                busNum = baseBusNum + device.channel
                create_i2c_device(device, busNum)
                expectedLabel = f"{busNum}-{device.address[2:].zfill(4)}"
                gpios = gpiodetect(expectedLabel)
                assert len(gpios) == 1, f"Expected GPIO not detected: {expectedLabel}"
                assert (
                    gpios[0].lines == device.gpioTestData.numLines
                ), f"Expected {device.gpioTestData.numLines}, got: {gpios[0].lines}"
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_gpio_info(fpga_with_adapters):
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
        try:
            for device in adapter.i2cDevices:
                if not device.gpioTestData:
                    continue
                testData = device.gpioTestData
                busNum = baseBusNum + device.channel
                create_i2c_device(device, busNum)
                expectedLabel = f"{busNum}-{device.address[2:].zfill(4)}"
                gpios = gpiodetect(expectedLabel)
                assert len(gpios) == 1, f"Expected GPIO not detected: {expectedLabel}"
                info = gpioinfo(gpios[0].name)
                for i, line in enumerate(info):
                    assert (
                        line.name == testData.lines[i].name
                    ), f"Line {i} name mismatch: expected={testData.lines[i].name}, got={line.name}"
                    assert (
                        line.direction == testData.lines[i].direction
                    ), f"Line {i} direction mismatch: expected={testData.lines[i].dir}, got={line.direction}"
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_gpio_set(platform_fpgas):
    pass


def test_gpio_get(fpga_with_adapters):
    for fpga, adapter in fpga_with_adapters:
        newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
        try:
            for device in adapter.i2cDevices:
                if not device.gpioTestData:
                    continue
                busNum = baseBusNum + device.channel
                create_i2c_device(device, busNum)
                expectedLabel = f"{busNum}-{device.address[2:].zfill(4)}"
                gpios = gpiodetect(expectedLabel)
                assert len(gpios) == 1, f"Expected GPIO not detected: {expectedLabel}"
                gpioName = gpios[0].name
                for i, line in enumerate(device.gpioTestData.lines):
                    if not line.getValue:
                        continue
                    try:
                        val = gpioget(gpioName, i)
                        expectedValue = line.getValue
                        assert (
                            val == expectedValue
                        ), f"Expected {expectedValue} for {gpioName} on line {i}, got: {val}"
                    except Exception:
                        pytest.fail(f"Failed to get GPIO value for {gpioName}")
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_driver_unload(platform_fpgas, platform_config):
    adaptersToUnload: list[tuple(FpgaSpec, I2CAdapter, int)] = []
    id = 1
    for fpga in platform_fpgas:
        try:
            for adapter in fpga.i2cAdapters:
                newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter, id)
                id += 1
                adaptersToUnload.append((fpga, adapter, id))
                for device in adapter.i2cDevices:
                    if not device.gpioTestData:
                        continue
                    create_i2c_device(device, baseBusNum + device.channel)
        except Exception:
            pytest.fail("Failed to create GPIO devices")
            for fpga, adapter, adapterId in adaptersToUnload:
                delete_device(fpga, adapter.auxDevice, adapterId)
    try:
        unload_kmods(platform_config.kmods)
    except Exception:
        pytest.fail("Failed to unload kmods with GPIO devices open")
        for fpga, adapter, adapterId in adaptersToUnload:
            delete_device(fpga, adapter.auxDevice, adapterId)
