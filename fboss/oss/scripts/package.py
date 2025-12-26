#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import concurrent.futures
import os
import pathlib
import sys
import tarfile
from typing import Dict

build_dir = pathlib.Path("/var/FBOSS/tmp_bld_dir/build/fboss")
src_dir = pathlib.Path("/var/FBOSS/fboss")
oss_dir = src_dir / "fboss/oss"
run_scripts = src_dir / "fboss/oss/scripts/run_scripts"
run_configs = src_dir / "fboss/oss/scripts/run_configs"

# build/src path -> install path
forwarding_stack = {
    f"{build_dir}/{bin}": f"bin/{bin}"
    for bin in [
        "diag_shell_client",
        "fboss2",
        "fboss_hw_agent-sai_impl",
        "fboss_sw_agent",
        "fsdb",
        "qsfp_service",
        "wedge_agent-sai_impl",
        "wedge_qsfp_util",
    ]
}
forwarding_stack.update(
    {
        oss_dir / "hw_sanity_tests": "share/hw_sanity_tests",
        run_scripts / "fboss_agent_utils.py": "bin/fboss_agent_utils.py",
        run_scripts / "qsfp_service_utils.py": "bin/qsfp_service_utils.py",
        run_scripts / "run_test.py": "bin/run_test.py",
        run_scripts / "setup.py": "bin/setup.py",
        run_scripts / "setup_fboss_env": "bin/setup_fboss_env",
        run_scripts / "topology.cdf": "bin/topology.cdf",
        run_configs / "j2cp": "share/j2cp",
        run_configs / "j3b": "share/j3b",
        run_configs / "janga800bic": "share/janga800bic",
        run_configs / "r3": "share/r3",
        run_configs / "tahan800bc": "share/tahan800bc",
        run_configs / "th": "share/th",
        run_configs / "th3": "share/th3",
        run_configs / "th4": "share/th4",
        run_configs / "th5": "share/th5",
    }
)

forwarding_stack_tests = {
    f"{build_dir}/{bin}": f"bin/{bin}"
    for bin in [
        "fboss-asic-config-gen",
        "fboss-platform-mapping-gen",
        "led_service_hw_test",
        "multi_switch_agent_hw_test",
        "qsfp_hw_test",
        "sai_agent_hw_test-sai_impl",
        "sai_agent_scale_test-sai_impl",
        "sai_invariant_agent_test-sai_impl",
        "sai_link_test-sai_impl",
        "sai_mono_link_test-sai_impl",
        "sai_multi_link_test-sai_impl",
        "sai_replayer-sai_impl",
        "sai_test-sai_impl",
    ]
}
forwarding_stack_tests.update(
    {
        oss_dir / "hw_known_bad_tests": "share/hw_known_bad_tests",
        oss_dir / "hw_test_configs": "share/hw_test_configs",
        oss_dir / "link_known_bad_tests": "share/link_known_bad_tests",
        oss_dir / "link_test_configs": "share/link_test_configs",
        oss_dir / "production_features": "share/production_features",
        oss_dir / "qsfp_known_bad_tests": "share/qsfp_known_bad_tests",
        oss_dir / "qsfp_test_configs": "share/qsfp_test_configs",
        oss_dir / "qsfp_unsupported_tests": "share/qsfp_unsupported_tests",
        oss_dir / "sai_hw_unsupported_tests": "share/sai_hw_unsupported_tests",
        run_scripts / "brcmsim.py": "bin/brcmsim.py",
    }
)

agent_benchmarks = {
    f"{build_dir}/{bin}": f"bin/{bin}"
    for bin in [
        "sai_ecmp_shrink_speed-sai_impl",
        "sai_ecmp_shrink_with_competing_route_updates_speed-sai_impl",
        "sai_fsw_scale_route_add_speed-sai_impl",
        "sai_fsw_scale_route_del_speed-sai_impl",
        "sai_hgrid_du_scale_route_add_speed-sai_impl",
        "sai_hgrid_du_scale_route_del_speed-sai_impl",
        "sai_init_and_exit_100Gx100G-sai_impl",
        "sai_init_and_exit_100Gx10G-sai_impl",
        "sai_init_and_exit_100Gx25G-sai_impl",
        "sai_init_and_exit_100Gx50G-sai_impl",
        "sai_init_and_exit_40Gx10G-sai_impl",
        "sai_rib_resolution_speed-sai_impl",
        "sai_rx_slow_path_rate-sai_impl",
        "sai_stats_collection_speed-sai_impl",
        "sai_switch_reachability_change_speed-sai_impl",
        "sai_th_alpm_scale_route_add_speed-sai_impl",
        "sai_th_alpm_scale_route_del_speed-sai_impl",
        "sai_tx_slow_path_rate-sai_impl",
    ]
}

platform_stack = {
    f"{build_dir}/{bin}": f"bin/{bin}"
    for bin in [
        "data_corral_service",
        "fan_service",
        "fw_util",
        "led_service",
        "platform_manager",
        "rackmon",
        "sensor_service",
        "sensor_service_client",
        "showtech",
        "weutil",
    ]
}
platform_stack.update(
    {
        oss_dir
        / "hw_sanity_tests/bsp_sanity_tests.conf": "share/hw_sanity_tests/bsp_sanity_tests.conf",
    }
)

platform_stack_tests = {
    f"{build_dir}/{bin}": f"bin/{bin}"
    for bin in [
        "bsp_tests",
        "data_corral_service_hw_test",
        "fan_service_hw_test",
        "fboss-bspmapping-gen",
        "fixmyfboss",
        "fw_util_hw_test",
        "mac_address_check_test",
        "platform_config_lib_config_generator",
        "platform_hw_test",
        "platform_manager_hw_test",
        "rackmon_test",
        "sensor_service_hw_test",
        "sensor_service_utils_test",
        "weutil_crc16_ccitt_test",
        "weutil_fboss_eeprom_interface_test",
        "weutil_hw_test",
    ]
}
platform_stack_tests.update(
    {
        run_scripts / "run_test.py": "bin/run_test.py",
        run_scripts / "setup_fboss_env": "bin/setup_fboss_env",
    }
)

# Target to install contents and test contents
targets = {
    "agent-benchmarks": (agent_benchmarks, {}),
    "forwarding-stack": (forwarding_stack, forwarding_stack_tests),
    "platform-stack": (platform_stack, platform_stack_tests),
}


def write_tar(filename: str, contents: Dict[str, str]) -> None:
    if not contents:
        return

    print(f"Creating {filename}...")
    with tarfile.open(filename, "w") as tar:
        for src, dest in contents.items():
            if not os.path.exists(src):
                print(f"Warning: {src} does not exist")
                continue
            tar.add(src, dest)


def package_fboss(target: str) -> None:
    with concurrent.futures.ProcessPoolExecutor() as executor:
        executor.submit(write_tar, f"{target}.tar", targets[target][0])
        executor.submit(write_tar, f"{target}-tests.tar", targets[target][1])


if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.argv[1] not in targets:
        print(
            f"Usage: {sys.argv[0]} [agent-benchmarks | forwarding-stack | platform-stack]"
        )
        sys.exit(1)
    package_fboss(sys.argv[1])
