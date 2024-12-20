# pyre-unsafe
import glob
import os

import pytest

from fboss.platform.bsp_tests.cdev_types import LedTestData
from fboss.platform.bsp_tests.utils.cdev_utils import create_new_device, delete_device
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
)


# For top-level LEDs, e.g port_led
def test_leds_created(platform_fpgas) -> None:
    for fpga in platform_fpgas:
        for led in fpga.ledCtrls:
            led_dev = led.auxDevice
            try:
                create_new_device(fpga, led_dev)
                assert find_expected_leds(
                    led.ledTestData,
                    led_dev.deviceName,
                    led_dev.ledInfo.ledId,
                    led_dev.ledInfo.portNumber,
                )
            finally:
                delete_device(fpga, led_dev)


# For devices which create LEDs
def test_device_leds_created(fpga_with_adapters) -> None:
    for fpga, adapter in fpga_with_adapters:
        for device in adapter.i2cDevices:
            if not device.testData or not device.testData.ledTestData:
                continue
            try:
                newAdapters, baseBusNum = create_i2c_adapter(fpga, adapter)
                busNum = baseBusNum + device.channel
                assert create_i2c_device(device, busNum)
                for led_data in device.testData.ledTestData:
                    assert find_expected_leds(
                        led_data, led_data.ledType, led_data.ledId
                    )
            finally:
                delete_device(fpga, adapter.auxDevice)


def find_expected_leds(
    testData: LedTestData, led_type: str, led_id: int = -1, port_num: int = -1
) -> bool:
    expected_led_name_prefix = ""
    if led_type == "port_led":
        assert port_num > 0, "Port Index must be 1-based"
        assert led_id > 0, "Port LED ID must be 1-based"
        expected_led_name_prefix = f"port{port_num}_led{led_id}"
    elif led_type == "fan_led":
        assert led_id != 0, "Fan LED ID must be 1-based"
        if led_id < 0:
            expected_led_name_prefix = "fan_led"
        else:
            expected_led_name_prefix = f"fan{led_id}_led"
    elif led_type == "psu_led":
        assert led_id != 0, "PSU LED ID must be 1-based"
        if led_id < 0:
            expected_led_name_prefix = "psu_led"
        else:
            expected_led_name_prefix = f"psu{led_id}_led"
    elif led_type == "sys_led":
        expected_led_name_prefix = "sys_led"
    elif led_type == "smb_led":
        expected_led_name_prefix = "smb_led"
    else:
        pytest.fail(f"Unsupported LED type {led_type}")

    print(os.listdir("/sys/class/leds"))
    print(f"Expected prefix: {expected_led_name_prefix}")
    led_files = [
        os.path.basename(f)
        for f in glob.glob(f"/sys/class/leds/{expected_led_name_prefix}*")
    ]
    for color in testData.expectedColors:
        assert f"{expected_led_name_prefix}:{color}:status" in led_files
    if len(testData.expectedColors) == 0:
        assert f"{expected_led_name_prefix}::status" in led_files
    return True
