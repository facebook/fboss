# pyre-strict
import json

from fboss.platform.bsp_tests.utils.cmd_utils import check_cmd, run_cmd
from fboss.platform.platform_manager.platform_manager_config.types import BspKmodsFile
from thrift.py3 import deserialize, Protocol


def build_util_path(vendor_name: str) -> str:
    kern_ver = run_cmd(["uname", "-r"]).stdout.decode().strip()
    return f"/usr/local/{vendor_name}_bsp/{kern_ver}"


def read_kmods(vendor_name: str) -> BspKmodsFile:
    kmod_path = f"{build_util_path(vendor_name)}/kmods.json"

    with open(kmod_path, "r") as f:
        json_data = json.load(f)

    return deserialize(
        BspKmodsFile, json.dumps(json_data).encode("utf-8"), protocol=Protocol.JSON
    )


def load_kmods(kmods: BspKmodsFile) -> None:
    for kmod in kmods.sharedKmods:
        check_cmd(["modprobe", kmod])
    for kmod in kmods.bspKmods:
        check_cmd(["modprobe", kmod])


def unload_kmods(kmods: BspKmodsFile) -> None:
    for kmod in kmods.bspKmods:
        check_cmd(["modprobe", "-r", kmod])
    for kmod in kmods.sharedKmods:
        check_cmd(["modprobe", "-r", kmod])


def fbsp_remove(vendor_name: str) -> None:
    check_cmd([f"{build_util_path(vendor_name)}/fbsp-remove.sh", "-f"])


def get_loaded_kmods(expected_kmods: BspKmodsFile) -> list[str]:
    result = run_cmd("lsmod").stdout.decode()
    expected_kmod_list = set(
        list(expected_kmods.bspKmods) + list(expected_kmods.sharedKmods)
    )
    kmods = []
    for line in result.split("\n"):
        parts = line.strip().split()
        if len(parts) == 0:
            continue
        kmod = parts[0]
        if kmod in expected_kmod_list:
            kmods.append(kmod)
    return kmods
