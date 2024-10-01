import ctypes
import fcntl
import glob
import os
import struct
from typing import Set

from ioctl_opt import IOR, IOWR

# Watchog IOCTL commands defined here:
# https://github.com/torvalds/linux/blob/master/include/uapi/linux/watchdog.h
WATCHDOG_IOCTL_BASE = ord("W")
WDIOC_KEEPALIVE = IOR(WATCHDOG_IOCTL_BASE, 5, ctypes.c_int)
WDIOC_SETTIMEOUT = IOWR(WATCHDOG_IOCTL_BASE, 6, ctypes.c_int)
WDIOC_GETTIMEOUT = IOR(WATCHDOG_IOCTL_BASE, 7, ctypes.c_int)


def get_watchdogs() -> set[str]:
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


def ping_watchdog(fd: int) -> None:
    result = fcntl.ioctl(fd, WDIOC_KEEPALIVE)
    if not result == 0:
        raise Exception(f"Failed to ping watchdog {result}")


def magic_close_watchdog(fd: int) -> None:
    # write "V" to the fd
    os.write(fd, b"V")
