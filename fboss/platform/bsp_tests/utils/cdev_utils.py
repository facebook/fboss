import fcntl
import os

from fboss.platform.bsp_tests.utils.cdev_types import (
    AuxDevice,
    fbiob_aux_data,
    FpgaSpec,
    getAuxData,
)
from ioctl_opt import IOW


FBIOB_IOC_MAGIC = ord("F")
FBIOB_IOC_NEW_DEVICE = IOW(FBIOB_IOC_MAGIC, 1, fbiob_aux_data)
FBIOB_IOC_DEL_DEVICE = IOW(FBIOB_IOC_MAGIC, 2, fbiob_aux_data)  # noqa


def create_new_device(fpga: FpgaSpec, device: AuxDevice, id: int = 1) -> None:
    aux_data = getAuxData(fpga, device, id)
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
    aux_data = getAuxData(fpga, device, id)
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
