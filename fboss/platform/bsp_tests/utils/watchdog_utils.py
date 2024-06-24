import ctypes
import fcntl
import glob
from typing import Set

from ioctl_opt import IOR

# Watchog IOCTL commands defined here:
# https://github.com/torvalds/linux/blob/master/include/uapi/linux/watchdog.h
WATCHDOG_IOCTL_BASE = ord("W")
WDIOC_GETTIMEOUT = IOR(WATCHDOG_IOCTL_BASE, 7, ctypes.c_int)


def get_watchdogs() -> Set[str]:
    return set(glob.glob("/dev/watchdog*"))


def get_watchdog_timeout(fd: int) -> int:
    timeout = ctypes.c_int()
    result = fcntl.ioctl(fd, WDIOC_GETTIMEOUT, timeout)
    if not result == 0:
        raise Exception(f"Failed to get watchdog timeout {result}")
    return timeout.value
