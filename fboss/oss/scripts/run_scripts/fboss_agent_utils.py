#!/usr/bin/env python3
# @noautodeps
# Copyright Meta Platforms, Inc. and affiliates.

import os
import subprocess
import typing as t

_DEFAULT_OSS_LOG_DIR = "/opt/fboss/logs/"
_DEFAULT_OSS_HW_AGENT_SERVICE_PATH = "/opt/fboss/bin/fboss_hw_agent-sai_impl"
_PLATFORM_MAPPING_OVERRIDE_PATH_ARG = "--platform_mapping_override_path"

# Default values synced from fboss/agent/AgentDirectoryUtil.cpp
FBOSS_AGENT_VOLATILE_STATE_DIR = "/dev/shm/fboss"
FBOSS_AGENT_WB_FLAG_FILE = f"{FBOSS_AGENT_VOLATILE_STATE_DIR}/warm_boot/can_warm_boot"

# These two hw agent services are running in prod and testing environment.
_HW_AGENT_SERVICE_PROD = "fboss_hw_agent@"
_HW_AGENT_SERVICE_FOR_TESTING = "fboss_hw_agent_for_testing_"

_HW_AGENT_SERVICE_OSS = "fboss_hw_agent_oss@"
_HW_AGENT_SERVICE_UNIT_FILE_PATH = (
    f"/tmp/{_HW_AGENT_SERVICE_OSS}{{switch_index}}.service"
)
_HW_AGENT_SERVICE_UNIT_FILE_TEMPLATE = rf"""
[Unit]
Description=FBOSS HW Agent Service For OSS

[Service]
LimitNOFILE=10000000
LimitCORE=32G
# Ensure memory usage is limited and oomd kills the process if it goes over
MemoryMax=3.75G
MemorySwapMax=0

Environment=TSAN_OPTIONS=die_after_fork=0
Environment=LD_LIBRARY_PATH=/opt/fboss/lib
ExecStart={{hw_agent_service_cmd}}
SyslogIdentifier={_HW_AGENT_SERVICE_OSS}{{switch_index}}
Restart=no

[Install]
WantedBy=multi-user.target
"""

_HW_AGENT_SERVICE_RSYSLOG_CONF_PATH = (
    f"/etc/rsyslog.d/{_HW_AGENT_SERVICE_OSS}{{switch_index}}.conf"
)
_HW_AGENT_SERVICE_RSYSLOG_CONF_CONTENT = rf"""
if $programname == "{_HW_AGENT_SERVICE_OSS}{{switch_index}}" then {_DEFAULT_OSS_LOG_DIR}/{_HW_AGENT_SERVICE_OSS}{{switch_index}}.log
& stop
"""


def agent_can_warm_boot_file_path(switch_index: t.Optional[int] = None) -> str:
    if switch_index is None:
        return FBOSS_AGENT_WB_FLAG_FILE
    return f"{FBOSS_AGENT_WB_FLAG_FILE}_{switch_index}"


def _cleanup_hw_agent_service_rsyslog_conf(switch_index: int) -> None:
    subprocess.run(
        f"rm {_HW_AGENT_SERVICE_RSYSLOG_CONF_PATH.format(switch_index=switch_index)}",
        shell=True,
    )
    subprocess.run("systemctl restart rsyslog", shell=True)


def cleanup_hw_agent_service(switch_indexes: t.List[int]) -> None:
    for switch_index in switch_indexes:
        print(f"Cleaning up FBOSS HW Agent Service for index={switch_index}...")
        subprocess.run(
            f"systemctl stop {_HW_AGENT_SERVICE_PROD}{switch_index}", shell=True
        )
        subprocess.run(
            f"systemctl stop {_HW_AGENT_SERVICE_FOR_TESTING}{switch_index}", shell=True
        )
        subprocess.run(
            f"systemctl stop {_HW_AGENT_SERVICE_OSS}{switch_index}", shell=True
        )
        subprocess.run(
            f"systemctl disable {_HW_AGENT_SERVICE_OSS}{switch_index}", shell=True
        )
        subprocess.run("systemctl daemon-reload", shell=True)
        # Just wanna be safe, also use pkill in case systemctl stop gets stuck
        subprocess.run(f"pkill -f {_HW_AGENT_SERVICE_PROD}{switch_index}", shell=True)
        subprocess.run(
            f"pkill -f {_HW_AGENT_SERVICE_FOR_TESTING}{switch_index}", shell=True
        )
        subprocess.run(f"pkill -f {_HW_AGENT_SERVICE_OSS}{switch_index}", shell=True)

        _cleanup_hw_agent_service_rsyslog_conf(switch_index)


def _manage_hw_agent_service(switch_indexes: t.List[int], isStart: bool) -> t.List[int]:
    return_codes = []
    op = "start" if isStart else "stop"
    op_ing = "Starting" if isStart else "Stopping"
    for switch_index in switch_indexes:
        print(f"{op_ing} FBOSS HW Agent Service for index={switch_index}...")
        ret = subprocess.run(
            f"systemctl {op} {_HW_AGENT_SERVICE_OSS}{switch_index}", shell=True
        ).returncode
        return_codes.append(ret)
        if ret != 0:
            print(
                f"Failed to {op} FBOSS HW Agent Service for index={switch_index} with return code {ret}"
            )
    return return_codes


def warm_boot_hw_agent(
    switch_indexes: t.List[int],
    skipStopping: bool = False,
) -> t.List[int]:
    # Stop HW Agent service
    if not skipStopping:
        return_codes = _manage_hw_agent_service(switch_indexes, False)
        if any(return_codes):
            print("Error while stopping HW Agent service")
            return return_codes

    # Check warmboot flag is set
    return_codes = []
    for switch_index in switch_indexes:
        ret = subprocess.run(
            f"stat {agent_can_warm_boot_file_path(switch_index)}", shell=True
        ).returncode
        if ret != 0:
            print(f"Error while checking warmboot flag for index={switch_index}")
    if any(return_codes):
        print("Error while checking warmboot flag")
        return return_codes

    # Start HW Agent service
    return_codes = _manage_hw_agent_service(switch_indexes, True)
    if any(return_codes):
        print("Error while starting HW Agent service")
    return return_codes


def cold_boot_hw_agent(
    switch_indexes: t.List[int],
    skipStopping: bool = False,
) -> t.List[int]:
    # Stop HW Agent service
    if not skipStopping:
        return_codes = _manage_hw_agent_service(switch_indexes, False)
        if any(return_codes):
            print("Error while stopping HW Agent service")
            return return_codes

    # Remove warmboot flag
    return_codes = []
    for switch_index in switch_indexes:
        ret = subprocess.run(
            f"rm -f {agent_can_warm_boot_file_path(switch_index)}", shell=True
        ).returncode
        if ret != 0:
            print(f"Error while removing warmboot flag for index={switch_index}")
    if any(return_codes):
        print("Error while removing warmboot flag")
        return return_codes

    # Start HW Agent service
    return_codes = _manage_hw_agent_service(switch_indexes, True)
    if any(return_codes):
        print("Error while starting HW Agent service")
    return return_codes


def _setup_hw_agent_service(
    switch_indexes: t.List[int],
    fboss_agent_config_path: str,
    hw_agent_service_bin_path: t.Optional[str] = None,
    platform_mapping_override_path: t.Optional[str] = None,
    sai_replayer_log_path: t.Optional[str] = None,
) -> None:
    if not hw_agent_service_bin_path:
        hw_agent_service_bin_path = _DEFAULT_OSS_HW_AGENT_SERVICE_PATH
    if not os.path.exists(hw_agent_service_bin_path):
        raise Exception(
            f"HW Agent Service binary path {hw_agent_service_bin_path} does not exist"
        )
    print(f"HW Agent Service binary path: {hw_agent_service_bin_path}")

    if not os.path.exists(fboss_agent_config_path):
        raise Exception(f"Agent config path {fboss_agent_config_path} does not exist")
    print(f"FBOSS Agent config path: {fboss_agent_config_path}")

    cleanup_hw_agent_service(switch_indexes)

    for switch_index in switch_indexes:
        print(f"Setting up FBOSS HW Agent Service for index={switch_index}...")

        # Prepare HW Agent Service unit file
        extra_args = ""
        if platform_mapping_override_path:
            if not os.path.exists(platform_mapping_override_path):
                raise Exception(
                    f"Platform mapping override path {platform_mapping_override_path} does not exist"
                )
            extra_args += f"{_PLATFORM_MAPPING_OVERRIDE_PATH_ARG} {platform_mapping_override_path}"
        if sai_replayer_log_path:
            extra_args += f"--sai_log {sai_replayer_log_path}"
        hw_agent_service_cmd = f"{hw_agent_service_bin_path} --config {fboss_agent_config_path} --switchIndex {switch_index} {extra_args}"
        with open(
            _HW_AGENT_SERVICE_UNIT_FILE_PATH.format(switch_index=switch_index), "w"
        ) as f:
            f.write(
                _HW_AGENT_SERVICE_UNIT_FILE_TEMPLATE.format(
                    hw_agent_service_cmd=hw_agent_service_cmd, switch_index=switch_index
                )
            )
            f.flush()

        # Prepare HW Agent Service rsyslog conf
        with open(
            _HW_AGENT_SERVICE_RSYSLOG_CONF_PATH.format(switch_index=switch_index), "w"
        ) as f:
            f.write(
                _HW_AGENT_SERVICE_RSYSLOG_CONF_CONTENT.format(switch_index=switch_index)
            )
            f.flush()

    subprocess.run("systemctl restart rsyslog; sleep 5", shell=True)
    for switch_index in switch_indexes:
        hw_agent_unit_file = _HW_AGENT_SERVICE_UNIT_FILE_PATH.format(
            switch_index=switch_index
        )
        subprocess.run(f"systemctl enable {hw_agent_unit_file}", shell=True)
    subprocess.run("systemctl daemon-reload", shell=True)


def setup_and_start_hw_agent_service(
    switch_indexes: t.List[int],
    fboss_agent_config_path: str,
    hw_agent_service_bin_path: t.Optional[str] = None,
    platform_mapping_override_path: t.Optional[str] = None,
    sai_replayer_log_path: t.Optional[str] = None,
    is_warm_boot: bool = False,
) -> None:
    # First setup hw agent service unit file for systemd and rsyslog config for logging
    _setup_hw_agent_service(
        switch_indexes,
        fboss_agent_config_path,
        hw_agent_service_bin_path,
        platform_mapping_override_path,
        sai_replayer_log_path,
    )

    # Then start hw agent service
    return_codes = (
        warm_boot_hw_agent(switch_indexes)
        if is_warm_boot
        else cold_boot_hw_agent(switch_indexes)
    )
    if any(return_codes):
        raise Exception(
            "Error while starting HW Agent service in {} mode".format(
                "warm-boot" if is_warm_boot else "cold-boot"
            )
        )
    print(
        "Successfully started HW Agent service in {} mode".format(
            "warm-boot" if is_warm_boot else "cold-boot"
        )
    )
