import concurrent.futures
import os
import re
from collections import defaultdict
from typing import Dict, List

import pytest

from fboss.platform.bsp_tests.utils.cdev_types import I2CDevice

from fboss.platform.bsp_tests.utils.cdev_utils import delete_device, make_cdev_path
from fboss.platform.bsp_tests.utils.cmd_utils import run_cmd
from fboss.platform.bsp_tests.utils.i2c_utils import (
    create_i2c_adapter,
    create_i2c_device,
    detect_i2c_device,
    i2cget,
    i2cset,
    parse_i2cdump_data,
)
from fboss.platform.bsp_tests.utils.kmod_utils import load_kmods, unload_kmods


def test_cdev_is_created(platform_fpgas) -> None:
    for fpga in platform_fpgas:
        path = make_cdev_path(fpga)
        assert os.path.exists(path)


def test_i2c_adapter_names(fpga_with_adapters) -> None:
    for fpga, adapter in fpga_with_adapters:
        pattern = r"i2c_master(_.+)?"
        assert re.search(
            pattern, adapter.auxDevice.deviceName
        ), "I2C Adapter name {adapter.auxDevice.deviceName} does not match expected pattern"


def test_i2c_adapter_creates_busses(fpga_with_adapters) -> None:
    for fpga, adapter in fpga_with_adapters:
        # Creates adapter, checks expected number of busses created
        newAdapters, _ = create_i2c_adapter(fpga, adapter)

        # Check each bus has a unique name
        try:
            names = set()
            for a in newAdapters:
                names.add(a.name)
            assert len(names) == len(newAdapters)
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_i2c_adapter_devices_exist(fpga_with_adapters) -> None:
    """
    Tests that each expected device is detectable
    """

    for fpga, adapter in fpga_with_adapters:
        assert adapter.auxDevice.i2cInfo
        # record the current existing busses
        newAdapters, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)

        try:
            for device in adapter.i2cDevices:
                print(
                    f"\nChecking for device {device.address} on bus {adapterBaseBusNum + device.channel}"
                )
                assert detect_i2c_device(
                    adapterBaseBusNum + device.channel, device.address
                )
        finally:
            delete_device(fpga, adapter.auxDevice)


def test_i2c_bus_with_devices_can_be_unloaded(platform_fpgas, platform_config) -> None:
    """
    Create bus, create devices on that bus, ensure that the bus
    driver can be unloaded successfully.
    """
    for fpga in platform_fpgas[0:1]:
        for adapter in reversed(fpga.i2cAdapters):
            load_kmods(platform_config.kmods)
            _, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)
            try:
                for device in adapter.i2cDevices:
                    busNum = adapterBaseBusNum + device.channel
                    assert detect_i2c_device(
                        adapterBaseBusNum + device.channel, device.address
                    ), f"i2c device {busNum}-{device.address[2:].zfill(4)} not detected"
                    assert create_i2c_device(
                        device, busNum
                    ), f"i2c device {busNum}-{device.address[2:].zfill(4)} not created"
            except Exception as e:
                delete_device(fpga, adapter.auxDevice)
                raise e
            unload_kmods(platform_config.kmods)


def test_i2c_transactions(platform_fpgas) -> None:
    """
    Create bus, create devices on that bus, ensure that the bus
    driver can be unloaded successfully.
    """
    for fpga in platform_fpgas:
        for adapter in fpga.i2cAdapters:
            # if any of the i2cDevices has testData
            if not any(device.testData for device in adapter.i2cDevices):
                continue
            newAdapters, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)
            try:
                for device in adapter.i2cDevices:
                    run_i2c_test_transactions(
                        device, adapterBaseBusNum + device.channel
                    )
            finally:
                delete_device(fpga, adapter.auxDevice)


def run_i2c_test_transactions(device: I2CDevice, busNum: int, repeat: int = 1) -> None:
    if not device.testData:
        return
    for _ in range(repeat):
        run_i2c_dump_test(device, busNum)
    for _ in range(repeat):
        run_i2c_get_test(device, busNum)
    for _ in range(repeat):
        run_i2c_set_test(device, busNum)


def run_i2c_test_transactions_concurrent(device: I2CDevice, busNum: int) -> None:
    if not device.testData:
        return
    futures = []
    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures.append(executor.submit(run_i2c_dump_test, device, busNum))
        futures.append(executor.submit(run_i2c_get_test, device, busNum))
        futures.append(executor.submit(run_i2c_set_test, device, busNum))
    concurrent.futures.wait(futures)


def run_i2c_dump_test(device: I2CDevice, busNum: int) -> None:
    if not device.testData:
        return
    for tc in device.testData.i2cDumpData:
        output = run_cmd(
            [
                "i2cdump",
                "-y",
                "-r",
                f"{tc.start}-{tc.end}",
                str(busNum),
                device.address,
            ]
        ).stdout.decode()
        result = parse_i2cdump_data(output)
        assert (
            result == tc.expected
        ), f"i2cdump output {result} did not match expected: {tc.expected} at {tc.start}-{tc.end} on {device.address}"


def run_i2c_get_test(device: I2CDevice, busNum: int) -> None:
    if not device.testData:
        return
    for tc in device.testData.i2cGetData:
        output = i2cget(str(busNum), device.address, tc.reg)
        assert (
            output == tc.expected
        ), f"Output: {output} did not match expected: {tc.expected} at {tc.reg} on {device.address}"


def run_i2c_set_test(device: I2CDevice, busNum: int) -> None:
    if not device.testData:
        return
    for tc in device.testData.i2cSetData:
        original = i2cget(str(busNum), device.address, tc.reg)

        i2cset(str(busNum), device.address, tc.reg, tc.value)
        output = i2cget(str(busNum), device.address, tc.reg)
        assert (
            output == tc.value
        ), f"Output: {output} did not match expected value of {tc.value} at {tc.reg} on {device.address}"

        i2cset(str(busNum), device.address, tc.reg, original)
        output = i2cget(str(busNum), device.address, tc.reg)
        assert output == original, "Did not successfully set value back to original"


def test_simultaneous_transactions(platform_fpgas) -> None:
    # for each adapter, check if at least 2 internal channels have devices with testData
    # if so, run transaction tests simultaneously on all channels
    for fpga in platform_fpgas:
        for adapter in fpga.i2cAdapters:
            devicesByChannel: dict[int, list[I2CDevice]] = defaultdict(list)
            for device in adapter.i2cDevices:
                if device.testData:
                    devicesByChannel[device.channel].append(device)
            if len(devicesByChannel) < 2:
                continue

            busses, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)
            try:
                futures = []
                with concurrent.futures.ThreadPoolExecutor() as executor:
                    for channel, devices in devicesByChannel.items():
                        for device in devices:
                            futures.append(
                                executor.submit(
                                    run_i2c_test_transactions_concurrent,
                                    device,
                                    adapterBaseBusNum + channel,
                                )
                            )
                concurrent.futures.wait(futures)
            except Exception as e:
                print(f"Failed to run concurrent i2c transactions, error {e}")
                pytest.fail()
            finally:
                delete_device(fpga, adapter.auxDevice)


@pytest.mark.stress
def test_looped_transactions(fpga_with_adapters) -> None:
    """
    Create bus, create devices on that bus, ensure that the bus
    driver can be unloaded successfully.
    """
    for fpga, adapter in fpga_with_adapters:
        # if any of the i2cDevices has testData
        if not any(device.testData for device in adapter.i2cDevices):
            continue
        newAdapters, adapterBaseBusNum = create_i2c_adapter(fpga, adapter)
        dmesg_line_cnt_before = int(
            run_cmd("dmesg | wc -l", shell=True).stdout.decode().strip()
        )
        try:
            # Stress test, only run on one device per adapter
            for device in adapter.i2cDevices[1:]:
                run_i2c_test_transactions(
                    device, adapterBaseBusNum + device.channel, 1000
                )
            dmesg_line_cnt_after = int(
                run_cmd("dmesg | wc -l", shell=True).stdout.decode().strip()
            )
            assert (
                dmesg_line_cnt_after - dmesg_line_cnt_before < 10
            ), "Too many dmesg log lines. Kernel log spew during i2c transactions."
        finally:
            delete_device(fpga, adapter.auxDevice)
