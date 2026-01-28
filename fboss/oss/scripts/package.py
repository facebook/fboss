#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

import argparse
import concurrent.futures
import os
import pathlib
import sys
import tarfile
from typing import Dict, List

SRC_DIR = pathlib.Path("/var/FBOSS/fboss")
OSS_DIR = SRC_DIR / "fboss/oss"
RUN_SCRIPTS_DIR = SRC_DIR / "fboss/oss/scripts/run_scripts"
RUN_CONFIGS_DIR = SRC_DIR / "fboss/oss/scripts/run_configs"

BUILD_DIR = "--build-dir"
TARGET_NAMES = ("agent-benchmarks", "forwarding-stack", "platform-stack")


# Global definitions describing what we package for each target.

FORWARDING_BINARIES = [
    "diag_shell_client",
    "fboss2",
    "fboss_hw_agent-sai_impl",
    "fboss_sw_agent",
    "fsdb",
    "qsfp_service",
    "wedge_agent-sai_impl",
    "wedge_qsfp_util",
]

FORWARDING_EXTRA = {
    OSS_DIR / "hw_sanity_tests": "share/hw_sanity_tests",
    RUN_SCRIPTS_DIR / "fboss_agent_utils.py": "bin/fboss_agent_utils.py",
    RUN_SCRIPTS_DIR / "qsfp_service_utils.py": "bin/qsfp_service_utils.py",
    RUN_SCRIPTS_DIR / "run_test.py": "bin/run_test.py",
    RUN_SCRIPTS_DIR / "setup.py": "bin/setup.py",
    RUN_SCRIPTS_DIR / "setup_fboss_env": "bin/setup_fboss_env",
    RUN_SCRIPTS_DIR / "topology.cdf": "bin/topology.cdf",
    RUN_CONFIGS_DIR / "j2cp": "share/j2cp",
    RUN_CONFIGS_DIR / "j3b": "share/j3b",
    RUN_CONFIGS_DIR / "janga800bic": "share/janga800bic",
    RUN_CONFIGS_DIR / "r3": "share/r3",
    RUN_CONFIGS_DIR / "tahan800bc": "share/tahan800bc",
    RUN_CONFIGS_DIR / "th": "share/th",
    RUN_CONFIGS_DIR / "th3": "share/th3",
    RUN_CONFIGS_DIR / "th4": "share/th4",
    RUN_CONFIGS_DIR / "th5": "share/th5",
}

FORWARDING_TEST_BINARIES = [
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

FORWARDING_TEST_EXTRA = {
    OSS_DIR / "hw_known_bad_tests": "share/hw_known_bad_tests",
    OSS_DIR / "hw_test_configs": "share/hw_test_configs",
    OSS_DIR / "link_known_bad_tests": "share/link_known_bad_tests",
    OSS_DIR / "link_test_configs": "share/link_test_configs",
    OSS_DIR / "production_features": "share/production_features",
    OSS_DIR / "qsfp_known_bad_tests": "share/qsfp_known_bad_tests",
    OSS_DIR / "qsfp_test_configs": "share/qsfp_test_configs",
    OSS_DIR / "qsfp_unsupported_tests": "share/qsfp_unsupported_tests",
    OSS_DIR / "sai_hw_unsupported_tests": "share/sai_hw_unsupported_tests",
    RUN_SCRIPTS_DIR / "brcmsim.py": "bin/brcmsim.py",
}

AGENT_BENCHMARK_BINARIES = [
    "sai_anticipated_scale_route_add_speed-sai_impl",
    "sai_anticipated_scale_route_del_speed-sai_impl",
    "sai_fsw_scale_route_add_speed-sai_impl",
    "sai_fsw_scale_route_del_speed-sai_impl",
    "sai_hgrid_du_scale_route_add_speed-sai_impl",
    "sai_hgrid_du_scale_route_del_speed-sai_impl",
    "sai_hgrid_uu_scale_route_add_speed-sai_impl",
    "sai_hgrid_uu_scale_route_del_speed-sai_impl",
    "sai_th_alpm_scale_route_add_speed-sai_impl",
    "sai_th_alpm_scale_route_del_speed-sai_impl",
    # Performance benchmarks
    "sai_ecmp_shrink_speed-sai_impl",
    "sai_ecmp_shrink_with_competing_route_updates_speed-sai_impl",
    "sai_rib_resolution_speed-sai_impl",
    "sai_rib_sync_fib_speed-sai_impl",
    "sai_rx_slow_path_rate-sai_impl",
    "sai_stats_collection_speed-sai_impl",
    "sai_tx_slow_path_rate-sai_impl",
    "sai_ucmp_scale_benchmark-sai_impl",
    # Init and exit benchmarks
    "sai_init_and_exit_40Gx10G-sai_impl",
    "sai_init_and_exit_100Gx10G-sai_impl",
    "sai_init_and_exit_100Gx25G-sai_impl",
    "sai_init_and_exit_100Gx50G-sai_impl",
    "sai_init_and_exit_100Gx100G-sai_impl",
    "sai_init_and_exit_400Gx400G-sai_impl",
    # VOQ benchmarks (BRCM DNX only)
    "sai_init_and_exit_fabric-sai_impl",
    "sai_init_and_exit_voq-sai_impl",
    "sai_voq_remote_entity_programming-sai_impl",
    "sai_voq_scale_route_add_speed-sai_impl",
    "sai_voq_scale_route_del_speed-sai_impl",
    "sai_voq_sys_port_programming-sai_impl",
    # DNX/FAKE only benchmarks
    "sai_switch_reachability_change_speed-sai_impl",
]

PLATFORM_BINARIES = [
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

PLATFORM_EXTRA = {
    OSS_DIR
    / "hw_sanity_tests/bsp_sanity_tests.conf": "share/hw_sanity_tests/bsp_sanity_tests.conf",
}

PLATFORM_TEST_BINARIES = [
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

PLATFORM_TEST_EXTRA = {
    RUN_SCRIPTS_DIR / "run_test.py": "bin/run_test.py",
    RUN_SCRIPTS_DIR / "setup_fboss_env": "bin/setup_fboss_env",
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


def _build_target(target: str, build_dir: pathlib.Path):
    """Return mappings for a given target and build_dir

    The global lists/dicts above describe what we package. Here we only wire
    them up to the build-dir specific directory.
    """

    bins = []
    extras = {}
    test_bins = {}
    test_extras = {}

    if target == "forwarding-stack":
        bins = FORWARDING_BINARIES
        extras = FORWARDING_EXTRA
        test_bins = FORWARDING_TEST_BINARIES
        test_extras = FORWARDING_TEST_EXTRA
    elif target == "platform-stack":
        bins = PLATFORM_BINARIES
        extras = PLATFORM_EXTRA
        test_bins = PLATFORM_TEST_BINARIES
        test_extras = PLATFORM_TEST_EXTRA
    elif target == "agent-benchmarks":
        bins = AGENT_BENCHMARK_BINARIES
        extras = {
            OSS_DIR / "hw_benchmark_tests": "share/hw_benchmark_tests",
        }

    fboss_build_dir = build_dir / "build" / "fboss"

    prod_files = {fboss_build_dir / bin_name: f"bin/{bin_name}" for bin_name in bins}
    prod_files.update(extras)

    test_files = {
        fboss_build_dir / bin_name: f"bin/{bin_name}" for bin_name in test_bins
    }
    test_files.update(test_extras)

    return (prod_files, test_files)


def package_fboss(target_name: str, target: List) -> None:
    with concurrent.futures.ProcessPoolExecutor() as executor:
        executor.submit(write_tar, f"{target_name}.tar", target[0])
        if len(target) == 2:
            executor.submit(write_tar, f"{target_name}-tests.tar", target[1])


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=(
            "Package FBOSS forwarding/platform stacks from a getdeps build tree."
        )
    )
    parser.add_argument(
        BUILD_DIR,
        dest="build_dir",
        type=pathlib.Path,
        required=True,
        help=(
            "Root scratch path used by getdeps. "
            "The fboss build tree is expected under SCRATCH_PATH/build/fboss."
        ),
    )
    parser.add_argument(
        "target",
        choices=list(TARGET_NAMES),
        help="Packaging target to create.",
    )
    return parser.parse_args(argv)


def main(argv=None) -> None:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    target_mappings = _build_target(args.target, args.build_dir)
    package_fboss(args.target, target_mappings)


if __name__ == "__main__":
    main()
