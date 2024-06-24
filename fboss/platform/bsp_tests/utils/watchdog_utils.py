import ctypes
import fcntl
import glob
import struct
from typing import Set

from ioctl_opt import IOR, IOWR

# Watchog IOCTL commands defined here:
# https://github.com/torvalds/linux/blob/master/include/uapi/linux/watchdog.h
WATCHDOG_IOCTL_BASE = ord("W")
WDIOC_SETTIMEOUT = IOWR(WATCHDOG_IOCTL_BASE, 6, ctypes.c_int)
WDIOC_GETTIMEOUT = IOR(WATCHDOG_IOCTL_BASE, 7, ctypes.c_int)


def get_watchdogs() -> Set[str]:
    return set(glob.glob("/dev/watchdog*"))


def get_watchdog_timeout(fd: int) -> int:
    timeout = ctypes.c_int()
    result = fcntl.ioctl(fd, WDIOC_GETTIMEOUT, timeout)
    if not result == 0:
        raise Exception(f"Failed to get watchdog timeout {result}")
    return timeout.value


def set_watchdog_timeout(fd: int, timeout: int) -> None:
    buf = struct.pack("I", timeout)
    fcntl.ioctl(fd, WDIOC_SETTIMEOUT, buf)
    return
