import fcntl
import os
from typing import Dict, List

from fboss.platform.bsp_tests.utils.cdev_types import (
    AuxDevice,
    fbiob_aux_data,
    FpgaSpec,
    get_aux_data,
)
from ioctl_opt import IOW


FBIOB_IOC_MAGIC = ord("F")
FBIOB_IOC_NEW_DEVICE = IOW(FBIOB_IOC_MAGIC, 1, fbiob_aux_data)
FBIOB_IOC_DEL_DEVICE = IOW(FBIOB_IOC_MAGIC, 2, fbiob_aux_data)  # noqa


def create_new_device(fpga: FpgaSpec, device: AuxDevice, id: int = 1) -> None:
    aux_data = get_aux_data(fpga, device, id)
    path = make_cdev_path(fpga)
    fd = os.open(path, os.O_RDWR)
    try:
        fcntl.ioctl(fd, FBIOB_IOC_NEW_DEVICE, aux_data)
    except Exception as e:
        print(f"Failed to create device {device.deviceName}, error: {e}")
        raise
    finally:
        os.close(fd)
    return


def delete_device(fpga: FpgaSpec, device: AuxDevice, id: int = 1) -> None:
    aux_data = get_aux_data(fpga, device, id)
    path = make_cdev_path(fpga)
    fd = os.open(path, os.O_RDWR)
    try:
        fcntl.ioctl(fd, FBIOB_IOC_DEL_DEVICE, aux_data)
    except Exception as e:
        print(f"Failed to delete device {device.deviceName} at {path}, error: {e}")
        raise
    finally:
        os.close(fd)
    return


def make_cdev_path(fpga: FpgaSpec) -> str:
    # strip the leading 0x
    vendor = fpga.vendorId[2:]
    device = fpga.deviceId[2:]
    subsystemVendor = fpga.subSystemVendorId[2:]
    subsystemDevice = fpga.subSystemDeviceId[2:]
    return f"/dev/fbiob_{vendor}.{device}.{subsystemVendor}.{subsystemDevice}"


def find_fpga_dirs(fpgas: list[FpgaSpec]) -> dict[str, str]:
    ret: dict[str, str] = {}
    for fpga in fpgas:
        found = False
        for subdir in os.listdir("/sys/bus/pci/devices/"):
            subdir_path = os.path.join("/sys/bus/pci/devices/", subdir)
            if not os.path.isdir(subdir_path):
                continue
            if check_files_for_fpga(fpga, subdir_path):
                found = True
                ret[fpga.name] = subdir_path
                break
        if not found:
            raise Exception(f"Could not find dir for fpga {fpga}")
    return ret


def check_files_for_fpga(fpga: FpgaSpec, dirPath: str) -> bool:
    fileValues = {
        "vendor": fpga.vendorId,
        "device": fpga.deviceId,
        "subsystem_vendor": fpga.subSystemVendorId,
        "subsystem_device": fpga.subSystemDeviceId,
    }

    for filename, value in fileValues.items():
        file_path = os.path.join(dirPath, filename)
        if not os.path.isfile(file_path):
            return False
        with open(file_path) as f:
            content = f.read().strip()
            if content != value:
                return False
    return True
