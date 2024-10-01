import glob
import os
from typing import Dict, Optional

import pytest

from fboss.platform.bsp_tests.utils.cdev_types import AuxDevice, FpgaSpec
from fboss.platform.bsp_tests.utils.cdev_utils import (
    create_new_device,
    delete_device,
    find_fpga_dirs,
)


def test_xcvr_names(platform_fpgas) -> None:
    for fpga in platform_fpgas:
        for xcvr in fpga.xcvrCtrls:
            assert xcvr.deviceName == "xcvr_ctrl"


def test_xcvr_creates_sysfs_files(platform_fpgas, platform_config) -> None:
    platform = platform_config.platform
    if platform == "meru800bfa" or platform == "meru800bia":
        pytest.skip("DSF fails xcvr test currently.")
    fpgaDirMap = find_fpga_dirs(platform_fpgas)
    for fpga in platform_fpgas:
        for xcvr in fpga.xcvrCtrls:
            assert xcvr.xcvrInfo
            port = xcvr.xcvrInfo.portNumber
            expected_files = [
                f"xcvr_reset_{port}",
                f"xcvr_low_power_{port}",
                f"xcvr_present_{port}",
            ]
            assert not find_subdevice(
                fpga, xcvr, port, fpgaDirMap
            ), "xcvr exists before creation"
            try:
                create_new_device(fpga, xcvr, port)
                devPath = find_subdevice(fpga, xcvr, port, fpgaDirMap)
                assert devPath, "Failed to create xcvr device"
                results = [(f, file_exists(devPath, f)) for f in expected_files]
                assert all(
                    result[1] for result in results
                ), f"Expected sysfs files do not exist: {results}"
            finally:
                delete_device(fpga, xcvr, port)
                assert not find_subdevice(
                    fpga, xcvr, port, fpgaDirMap
                ), "Failed to delete xcvr device"


def find_subdevice(
    fpga: FpgaSpec, dev: AuxDevice, id: int, fpgaDirMap: dict[str, str]
) -> str | None:
    pciDir = fpgaDirMap[fpga.name]
    path_pattern = f"{pciDir}/*{dev.deviceName}.{id}"
    matching_files = glob.glob(path_pattern)
    if len(matching_files) == 1:
        return matching_files[0]
    else:
        return ""


def file_exists(dir: str, filename: str) -> bool:
    return os.path.exists(os.path.join(dir, filename))
