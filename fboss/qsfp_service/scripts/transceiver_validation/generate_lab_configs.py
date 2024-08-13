import os
import re
from typing import Dict

from configerator.client import ConfigeratorClient, ConfigeratorMissingConfigException
from libfb.py.pyre import none_throws

from neteng.fboss.qsfp_service_config import ttypes as qsfp_config_types
from neteng.fboss.switch_config.ttypes import PortProfileID
from neteng.netcastle.teams.fboss.utils.qsfp_service_utils import _QSFP_TEST_CONFIG_DIR


"""
parse_wedge_dump_direct_i2c: Parses text dump from `sudo wedge_qsfp_util --direct-i2c`.

:param wedge_data: text dump in one string
:return: dict mapping port to another dict containing vendor name, vendor part number
"""


def parse_wedge_dump_direct_i2c(wedge_data: str) -> Dict[str, Dict[str, str]]:
    port_re = r"Logical Ports: (.*)"
    vendor_re = r"Vendor: (.*)"
    vendor_pn_re = r"Vendor PN: (.*)"

    port_to_vendor_info = {}
    all_port_text = wedge_data.split("Port ")

    for port_text in all_port_text:
        match = re.search(port_re, port_text)
        if not match:
            continue
        all_ports = match.group(1).strip()
        port = all_ports.split(",")[0]

        match = re.search(vendor_re, port_text)
        if not match:
            continue
        vendor_name = match.group(1).strip()

        match = re.search(vendor_pn_re, port_text)
        if not match:
            continue
        vendor_pn = match.group(1).strip()

        port_to_vendor_info[port] = {
            "vendor_name": vendor_name,
            "vendor_pn": vendor_pn,
        }

    return port_to_vendor_info


"""
fetch_cabled_port_pairs: For a given platform hw_config string, fetch the associated cabledPortPairs from the qsfp test config file.

:param platform: platform hw config to fetch (e.g. 'darwin_mix')

:return: map of port names -> profileIDs
"""


def fetch_cabled_port_pairs(platform: str) -> Dict[str, int]:
    config_path = "{baseDir}/{platform}".format(
        baseDir=_QSFP_TEST_CONFIG_DIR,
        platform=platform,
    )
    qsfp_config = ConfigeratorClient().get_config_contents_as_thrift(
        config_path, qsfp_config_types.QsfpServiceConfig
    )

    cabled_port_profiles = {}
    for port_info in none_throws(qsfp_config.qsfpTestConfig).cabledPortPairs:
        cabled_port_profiles[port_info.aPortName] = port_info.profileID
        cabled_port_profiles[port_info.zPortName] = port_info.profileID

    return cabled_port_profiles


"""
parse_platform_lab_devices: Parses all text wedge_qsfp_util dumps for a given platform and writes all unique transceiver configs, 
both a cabledPortPair-only list and a full list, to output files.

:param data_folder_path: full path to folder that contains text dumps for given platform
:param out_folder_path: full path to folder that output files are written to
:param platform: platform to parse (e.g. darwin)

:return: none. writes output to out_folder_path as a txt file.
"""


def parse_platform_lab_devices(
    data_folder_path: str,
    out_folder_path: str,
    platform: str,
) -> None:
    cabled_tuples = set()
    all_tuples = set()
    for file in os.listdir(data_folder_path):
        if not re.match(".*out", file):
            continue

        last_hw_config = ""
        full_file_path = os.path.join(data_folder_path, file)
        with open(full_file_path) as f:
            try:
                text_dump = f.read()
            except Exception:
                print(f"Error reading file {full_file_path}")
                continue
            hw_config = text_dump.split("\n")[0]
            port_to_vendor_info = parse_wedge_dump_direct_i2c(text_dump)

            # this adds some default behavior for when certain basset devices don't have a listed hw_config attribute (e.g. fboss315069340.snc1 for darwin)
            try:
                port_to_profile = fetch_cabled_port_pairs(hw_config)
                last_hw_config = hw_config
            except ConfigeratorMissingConfigException:
                if not last_hw_config:
                    continue
                port_to_profile = fetch_cabled_port_pairs(last_hw_config)

            for port, vendor_info in port_to_vendor_info.items():
                tcvr_tuple = (
                    vendor_info["vendor_name"].upper(),
                    vendor_info["vendor_pn"],
                    # pyre-fixme[6]: Expected `PortProfileID` for 1st param but got `int`.
                    PortProfileID._VALUES_TO_NAMES[port_to_profile.get(port, 0)],
                )
                all_tuples.add(tcvr_tuple)
                if port in port_to_profile:
                    cabled_tuples.add(tcvr_tuple)

    with open(f"{out_folder_path}{platform}_full.txt", "w") as f:
        f.write("\n".join([",".join(tup) for tup in sorted(all_tuples)]))
    with open(f"{out_folder_path}{platform}_cabled.txt", "w") as f:
        f.write("\n".join([",".join(tup) for tup in sorted(cabled_tuples)]))


"""
parse_all_platform_devices: Parses all lab device wedge_qsfp_util dumps.

:param data_folder_path: path to folder that contains subfolders with wedge_qsfp_util dumps per platform
:param out_folder_path: path to folder that output files are written to

:return: none. writes output to out_folder_path as a txt file for each platform.
"""


def parse_all_platform_lab_devices(data_folder_path: str, out_folder_path: str) -> None:
    expanded_path = os.path.expanduser(data_folder_path)
    expanded_out_path = os.path.expanduser(out_folder_path)
    for subdir in os.listdir(expanded_path):
        joined_path = os.path.join(expanded_path, subdir)
        if os.path.isdir(joined_path):
            parse_platform_lab_devices(joined_path, expanded_out_path, subdir)


def main() -> None:
    parse_all_platform_lab_devices("~/lab_configs/", "~/lab_out/")


if __name__ == "__main__":
    main()
