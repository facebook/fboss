import subprocess
from collections import namedtuple
from typing import Set, Tuple

from fboss.platform.bsp_tests.utils.cdev_types import FpgaSpec, I2CAdapter
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
) -> Tuple[Set[I2CBus], int]:
    assert adapter.auxDevice.i2cInfo
    numBusses = adapter.auxDevice.i2cInfo.numChannels
    # record the current existing busses
    existingAdapters = find_i2c_busses()
    create_new_device(fpga, adapter.auxDevice, id)
    newBusses = find_i2c_busses() - existingAdapters
    minBusNum = min(a.busNum for a in newBusses)
    assert len(newBusses) == numBusses
    return (newBusses, minBusNum)


def find_i2c_busses() -> Set[I2CBus]:
    output = run_cmd(["i2cdetect", "-l"]).stdout.decode()
    adapters: Set[I2CBus] = set()
    for line in output.split("\n"):
        if line:
            adapters.add(parse_i2cdetect_line(line))
    return adapters
