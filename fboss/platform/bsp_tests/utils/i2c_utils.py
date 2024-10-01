import os
import subprocess
from collections import namedtuple
from typing import List, Set, Tuple

from fboss.platform.bsp_tests.utils.cdev_types import FpgaSpec, I2CAdapter, I2CDevice
from fboss.platform.bsp_tests.utils.cdev_utils import create_new_device
from fboss.platform.bsp_tests.utils.cmd_utils import run_cmd

# structures the output of `i2cdetect -l`
I2CBus = namedtuple("I2CBus", ["busNum", "type", "name", "description"])


def parse_i2cdetect_line(line):
    parts = line.split("\t")
    return I2CBus(
        busNum=int(parts[0].split("-")[1]),
        type=parts[1],
        name=parts[2],
        description=parts[3],
    )


def parse_i2cdump_data(data: str) -> list[str]:
    # first line is header
    data_lines = data.split("\n")[1:]
    data_bytes = []

    for line in data_lines:
        parts = line.split()
        if len(parts) < 2:
            continue
        # Byte data will only contain these characters, any other string will be ignored
        line_bytes = [
            part for part in parts[1:] if all(c in "0123456789abcdef" for c in part)
        ]
        data_bytes.extend(line_bytes)
    return data_bytes


def detect_i2c_device(bus: int, hexAddr: str) -> bool:
    # If device cannot be detected with "-y", also try "-r" and finally "-q"
    for opt in [["-y"], ["-y", "-r"], ["-y", "-q"]]:
        cmd = ["i2cdetect"] + opt + [str(bus), hexAddr, hexAddr]
        output = subprocess.run(cmd, stdout=subprocess.PIPE).stdout.decode()
        for line in output.split("\n"):
            if line:
                parts = line.strip().split()
                if hexAddr[2:] in parts:
                    return True
    return False


def create_i2c_adapter(
    fpga: FpgaSpec, adapter: I2CAdapter, id: int = 1
) -> tuple[set[I2CBus], int]:
    assert adapter.auxDevice.i2cInfo
    numBusses = adapter.auxDevice.i2cInfo.numChannels
    # record the current existing busses
    existingAdapters = find_i2c_busses()
    create_new_device(fpga, adapter.auxDevice, id)
    newBusses = find_i2c_busses() - existingAdapters
    minBusNum = min(a.busNum for a in newBusses)
    assert len(newBusses) == numBusses
    return (newBusses, minBusNum)


def find_i2c_busses() -> set[I2CBus]:
    output = run_cmd(["i2cdetect", "-l"]).stdout.decode()
    adapters: set[I2CBus] = set()
    for line in output.split("\n"):
        if line:
            adapters.add(parse_i2cdetect_line(line))
    return adapters


def create_i2c_device(dev: I2CDevice, bus: int) -> bool:
    try:
        cmd = f"echo {dev.deviceName} {dev.address} > /sys/bus/i2c/devices/i2c-{bus}/new_device"
        run_cmd(cmd, shell=True).stdout.decode()
        # find the device and verify it is there
        dev_dir = "/sys/bus/i2c/devices"
        assert os.path.exists(
            f"{dev_dir}/i2c-{bus}"
        ), f"Device {dev.address} on bus {bus} not found"
        # read the "name" file and verify it is correct
        with open(f"{dev_dir}/{bus}-{dev.address[2:].zfill(4)}/name") as f:
            name = f.read().strip()
            assert (
                name == dev.deviceName
            ), f"Device {dev.address} on bus {bus}: wrong device name: expected '{dev.deviceName}' got '{name}'"
        return True
    except Exception as e:
        print(f"Failed to create device {dev.deviceName}, error: {e}")
        return False


def i2cget(bus: str, addr: str, reg: str) -> str:
    cmd = ["i2cget", "-y", bus, addr, reg]
    return run_cmd(cmd).stdout.decode().strip()


def i2cset(bus: str, addr: str, reg: str, data: str) -> None:
    run_cmd(["i2cset", "-y", bus, addr, reg, data])
