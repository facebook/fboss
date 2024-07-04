# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import json
import os
import re
from typing import Dict, List, Set, Tuple

# We don't want this to be in the fboss directory, as we don't want to check in all of these text dumps and JSONs here.
# Please check output JSONs into configerator directly.
_DIR_PATH: str = os.path.expanduser("~/tcvr_configs/")
_JSON_DIR_PATH: str = os.path.expanduser("~/tcvr_configs_json/")
_COLLECT_FIRMWARE: bool = False
_BOX_DIVIDER_STR: str = "BOX_DIVIDER"
_CMD_DIVIDER_STR: str = "CMD_DIVIDER"


# BASE PARSERS

"""
parse_wedge_dump: Parses text dump from `sudo wedge_qsfp_util`.

:param wedge_data: text dump in one string
:return: dict mapping port to media interface code (e.g. FR4_200G)
"""


def parse_wedge_dump(wedge_data: str) -> Dict[str, str]:
    port_re = r"Logical Ports: (.*)"
    media_interface_re = r"Module Media Interface: (.*)"
    eeprom_checksum_re = r"EEPROM Checksum: (.*)"

    port_to_code = {}
    all_port_text = wedge_data.split("Port ")

    for port_text in all_port_text:
        match = re.search(port_re, port_text)
        if not match:
            continue
        all_ports = match.group(1).strip()
        port = all_ports.split(",")[0]

        match = re.search(media_interface_re, port_text)
        if not match:
            continue
        part_num = match.group(1).strip()

        match = re.search(eeprom_checksum_re, port_text)
        if not match or match.group(1).strip() == "Invalid":
            continue

        port_to_code[port] = part_num

    return port_to_code


"""
parse_tcvr_dump: Parses text dump from `fboss2 show transceiver`.

:param tcvr_data: text dump in one string
:return: dict mapping port to dict including vendor_name and vendor_part_number (and potentially firmware versions)
"""


def parse_tcvr_dump(tcvr_data: str) -> Dict[str, Dict[str, str]]:
    port_to_tcvr_info = {}
    all_lines = tcvr_data.strip().split("\n")

    header = all_lines[0]
    vendor_idx = header.index("Vendor")
    serial_idx = header.index("Serial")
    part_idx = header.index("Part Number")
    fw_idx = header.index("FW App Version")
    dsp_fw_idx = header.index("FW DSP Version")
    temp_idx = header.index("Temperature")

    # parsing non-header lines
    lines = all_lines[2:]
    for line in lines:
        substr = line[:vendor_idx]
        cols = substr.split()
        if len(cols) < 3:
            continue
        status = cols[2]

        if status == "Absent":
            continue
        port = cols[0]

        substr = line[vendor_idx:serial_idx]
        vendor = substr.strip()

        if len(vendor) == 0 or vendor == "UNKNOWN" or "\x00" in vendor:
            continue

        substr = line[part_idx:fw_idx]
        part_num = substr.strip()

        if len(part_num) == 0 or part_num == "UNKNOWN" or "\x00" in part_num:
            continue

        port_to_tcvr_info[port] = {
            "vendor_name": vendor.upper(),
            "vendor_part_number": part_num,
        }

        if _COLLECT_FIRMWARE:
            substr = line[fw_idx:dsp_fw_idx]
            fw_ver = substr.strip()
            substr = line[dsp_fw_idx:temp_idx]
            dsp_fw_ver = substr.strip()

            port_to_tcvr_info[port].update(
                {"firmware_version": fw_ver, "dsp_firmware_version": dsp_fw_ver}
            )

    return port_to_tcvr_info


"""
parse_port_dump: Parses text dump from `fboss2 show port`.

:param port_data: text dump in one string
:return: dict mapping port to PortProfileID (e.g. PROFILE_10G_1_NRZ_NOFEC)
"""


def parse_port_dump(port_data: str) -> Dict[str, str]:
    port_to_profile = {}
    all_lines = port_data.strip().split("\n")

    # parsing non-header lines
    lines = all_lines[2:]
    for line in lines:
        cols = line.split()
        raw_port = cols[1]
        status = cols[4]

        if status == "Absent":
            continue

        profile = cols[8]

        raw_port_parts = raw_port.split("/")
        port = raw_port_parts[0] + "/" + raw_port_parts[1] + "/1"

        port_to_profile[port] = profile

    return port_to_profile


"""
get_config_list: Combines results parse_wedge_dump(), parse_tcvr_dump(), and parse_port_dump() into one dict.

:param port_to_code: dict mapping port to media interface code (e.g. FR4_200G)
:param port_to_tcvr_info: dict mapping port to dict including vendor_name and vendor_part_number (and potentially firmware versions)
:param port_to_profile: dict mapping port to PortProfileID (e.g. PROFILE_10G_1_NRZ_NOFEC)

:return: list of (vendor_name, vendor_part_number, media_interface_code, port_profile, optional firmware_version, optional dsp_firmware_version) tuples 
"""


def get_config_list(
    port_to_code: Dict[str, str],
    port_to_tcvr_info: Dict[str, Dict[str, str]],
    port_to_profile: Dict[str, str],
) -> List[Tuple[str, str, str, str]]:
    all_configs = []
    for port in port_to_code:
        if port not in port_to_tcvr_info or port not in port_to_profile:
            continue
        tuple_items = [
            port_to_tcvr_info[port]["vendor_name"],
            port_to_tcvr_info[port]["vendor_part_number"],
            port_to_code[port],
            port_to_profile[port],
        ]
        if _COLLECT_FIRMWARE:
            tuple_items.extend(
                [
                    port_to_tcvr_info[port]["firmware_version"],
                    port_to_tcvr_info[port]["dsp_firmware_version"],
                ]
            )
        all_configs.append(tuple(tuple_items))

    return all_configs


"""
fold_port_tuples_into_json: Combines list of tuples corresponding to port configs into a nested JSON with vendor as key.

:param all_ports: set of tuples containing (vendor_name, vendor_part_number, media interface code, port profile) (and potentially firmware)

:return: list of (vendor_name, vendor_part_number, media_interface_code, port_profile, optional firmware_version, optional dsp_firmware_version) tuples 
"""


def fold_port_tuples_into_json(all_ports: Set[Tuple[str, str, str, str]]) -> List[Dict]:
    all_jsons, last_tup = [], ()
    for port in sorted(all_ports):
        if (port[0], port[1], port[2]) == last_tup:
            all_jsons[-1]["port_profiles"].append(port[3])
            if _COLLECT_FIRMWARE:
                if port[4] not in all_jsons[-1]["firmware_versions"]:
                    all_jsons[-1]["firmware_versions"].append(port[4])
                if port[5] not in all_jsons[-1]["dsp_firmware_versions"]:
                    all_jsons[-1]["dsp_firmware_versions"].append(port[5])
            continue
        all_jsons.append(
            {
                "vendor_name": port[0],
                "vendor_part_number": port[1],
                "media_interface_code": port[2],
                "port_profiles": [port[3]],
            }
        )
        if _COLLECT_FIRMWARE:
            all_jsons[-1].update(
                {"firmware_versions": [port[4]], "dsp_firmware_versions": [port[5]]}
            )
        last_tup = (port[0], port[1], port[2])
    old_jsons = all_jsons
    all_jsons = []
    last_vendor = ""
    for j in old_jsons:
        vendor_name = j["vendor_name"]
        del j["vendor_name"]
        if vendor_name == last_vendor:
            all_jsons[-1]["transceivers"].append(j)
            continue
        all_jsons.append(
            {
                "vendor_name": vendor_name,
                "transceivers": [j],
            }
        )
        last_vendor = vendor_name

    return all_jsons


"""
parse_platform_dump: Parses a full text file containing wedge, tcvr, and port dumps for multiple boxes, given they use the proper divider strings.

:raw_data_file_path: full path to input file
:is_multiple_files: indicate whether or not you are pointing to a directory of multiple files to aggregate or just one file
:json_data_dir: full path to output directory
:platform_name: platform to parse (used in naming output json file)

:return: none. writes output to json_data_dir as a json file.
"""


def parse_platform_dump(
    raw_data_file_path: str,
    is_multiple_files: bool,
    json_data_dir: str,
    platform_name: str,
) -> None:
    all_ports = set()

    # add all configs for a switch to the set
    all_files = (
        [
            os.path.join(raw_data_file_path, file)
            for file in os.listdir(raw_data_file_path)
        ]
        if is_multiple_files
        else [raw_data_file_path]
    )
    for file in all_files:
        with open(file, "r") as f:
            full_text_dump = f.read()
            all_switch_info = full_text_dump.split(_BOX_DIVIDER_STR)
            for switch_info in all_switch_info:
                parts = switch_info.split(_CMD_DIVIDER_STR)
                parts = [part for part in parts if part.strip() != ""]
                if len(parts) != 3:
                    continue
                port_to_code = parse_wedge_dump(parts[0])
                port_to_tcvr_info = parse_tcvr_dump(parts[1])
                port_to_profile = parse_port_dump(parts[2])

                for config in get_config_list(
                    port_to_code, port_to_tcvr_info, port_to_profile
                ):
                    all_ports.add(config)

    # fold configs based on (vendor_name, vendor_part_number, media_interface_code) and write to JSON
    with open(f"{json_data_dir}{platform_name}.json", "w") as f:
        all_jsons = fold_port_tuples_into_json(all_ports)
        f.write(f"{json.dumps(all_jsons, indent=4)}")

    print(
        f"[{platform_name}] Parsed {raw_data_file_path} into {json_data_dir + platform_name}.json."
    )


"""
parse_all_platform_dumps: Parses text dumps for all platforms in the given directory.

:raw_data_dir: full path to directory with output files
:json_data_dir: full path to directory for output jsons

:return: none. writes parsed output to json files.
"""


def parse_all_platform_dumps(raw_data_dir: str, json_data_dir: str) -> None:
    for file in os.listdir(raw_data_dir):
        if not re.match(".*out", file):
            continue
        parse_platform_dump(
            os.path.join(raw_data_dir, file), False, json_data_dir, file[:-4]
        )


# EXTRA HELPER FUNCTIONS (to determine the overlap between a partial dump of tcvr_data and a full dump of tcvr_data)

"""
:find_config_overlap: Given the name of a partial switch JSON dump file (only querying XX% of boxes in the fleet) and the name of a full JSON dump file (querying 100% of boxes in the fleet),
prints out the number of overlapping switch configs. This gives an idea of the percentage of configs a partial scrape has covered.

:json_dir_path:
:partial_dump_json_name:
:full_dump_json_name:

:return: none. prints coverage results to stdout.
"""


def find_config_overlap(
    json_dir_path: str, partial_dump_json_name: str, full_dump_json_name: str
) -> None:
    with open(f"{json_dir_path}{full_dump_json_name}.json", "r") as f:
        full_fleet_jsons = json.load(f)
    with open(f"{json_dir_path}{partial_dump_json_name}.json", "r") as f:
        partial_fleet_jsons = json.load(f)

    full_config_set = {
        (struct["vendor_name"], config["vendor_part_number"])
        for struct in full_fleet_jsons
        for config in struct["transceivers"]
    }
    partial_config_set = {
        (struct["vendor_name"], config["vendor_part_number"])
        for struct in partial_fleet_jsons
        for config in struct["transceivers"]
    }

    partial_set_size, full_set_size = len(partial_config_set), len(full_config_set)
    intersection_size = len(partial_config_set.intersection(full_config_set))

    print(
        (
            f"For {partial_dump_json_name}, partial fleet had {partial_set_size} unique configs, full fleet had {full_set_size} unique configs. "
            f"There were {intersection_size} common configs, meaning ~{(partial_set_size * 100 // full_set_size)}% of full configurations were captured. "
            "Note that this metric excludes distinct PortProfileIDs."
        )
    )


def main() -> None:
    parse_all_platform_dumps(_DIR_PATH, _JSON_DIR_PATH)


if __name__ == "__main__":
    main()
