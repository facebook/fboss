import glob
import os

import pytest

from fboss.platform.bsp_tests.utils.cdev_types import LedCtrlInfo
from fboss.platform.bsp_tests.utils.cdev_utils import create_new_device, delete_device


# TODO: Enable testing of LEDs that are created by other drivers, e.g. fan_led
# created by fan driver. Idea: Make ledTestInfo pluggable to any i2cDevice config
# so all the information will be available.


def test_leds_created(platform_fpgas) -> None:
    for fpga in platform_fpgas:
        for led in fpga.ledCtrls:
            led_dev = led.auxDevice
            try:
                create_new_device(fpga, led_dev)
                assert find_expected_leds(led)
            finally:
                delete_device(fpga, led_dev)


def find_expected_leds(led: LedCtrlInfo) -> bool:
    assert led.auxDevice.ledInfo
    led_info = led.auxDevice.ledInfo
    led_type = led.auxDevice.deviceName
    led_id = led_info.ledId
    port_num = led_info.portNumber

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

    led_files = [
        os.path.basename(f)
        for f in glob.glob(f"/sys/class/leds/{expected_led_name_prefix}*")
    ]
    for color in led.ledTestInfo.expectedColors:
        assert f"{expected_led_name_prefix}:{color}:status" in led_files
    return True
