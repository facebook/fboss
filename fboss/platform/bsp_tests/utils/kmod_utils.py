import shutil
from typing import List

from fboss.platform.bsp_tests.utils.cmd_utils import check_cmd, run_cmd


def fbsp_remove() -> bool:
    if shutil.which("fbsp-remove.sh") is None:
        return False
    check_cmd(["fbsp-remove.sh", "-f"])
    return True


def get_loaded_kmods(expected_kmods: List[str]) -> List[str]:
    result = run_cmd("lsmod").stdout.decode()
    kmods = []
    for line in result.split("\n"):
        parts = line.strip().split()
        if len(parts) == 0:
            continue
        kmod = parts[0]
        if kmod in expected_kmods:
            kmods.append(kmod)
    return kmods
