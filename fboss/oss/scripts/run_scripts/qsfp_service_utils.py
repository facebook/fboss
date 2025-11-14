#!/usr/bin/env python3
# @noautodeps
# Copyright Meta Platforms, Inc. and affiliates.

import os
import subprocess
import typing as t


_DEFAULT_OSS_QSFP_SERVICE_PATH = "/opt/fboss/bin/qsfp_service"
_DEFAULT_OSS_LOG_DIR = "/opt/fboss/logs/"
_PLATFORM_MAPPING_OVERRIDE_PATH_ARG = "--platform_mapping_override_path"
_BSP_PLATFORM_MAPPING_OVERRIDE_PATH_ARG = "--bsp_platform_mapping_override_path"
_DEFAULT_QSFP_SERVICE_VOLATIRE_DIR = "/dev/shm/fboss/qsfp_service"
_QSFP_SERVICE_COLD_BOOT_FILE = "cold_boot_once_qsfp_service"

# These two qsfp services are running in prod and testing environment.
_QSFP_SERVICE_PROD = "qsfp_service"
_QSFP_SERVICE_FOR_TESTING = "qsfp_service_for_testing"
# Introduce an oss version of qsfp_service to run in oss environment.
_QSFP_SERVICE_OSS = "qsfp_service_oss"
_QSFP_SERVICE_UNIT_FILE_PATH = f"/tmp/{_QSFP_SERVICE_OSS}.service"
_QSFP_SERVICE_UNIT_FILE_TEMPLATE = rf"""
[Unit]
Description=QSFP Service For OSS

[Service]
LimitNOFILE=10000000
LimitCORE=32G
# Ensure memory usage is limited and oomd kills the process if it goes over
MemoryMax=3.75G
MemorySwapMax=0

Environment=TSAN_OPTIONS="die_after_fork=0 halt_on_error=1"
Environment=LD_LIBRARY_PATH=/opt/fboss/lib
ExecStart={{qsfp_service_cmd}}
SyslogIdentifier={_QSFP_SERVICE_OSS}
Restart=no

[Install]
WantedBy=multi-user.target
"""

_QSFP_SERVICE_RSYSLOG_CONF_PATH = f"/etc/rsyslog.d/{_QSFP_SERVICE_OSS}.conf"
_QSFP_SERVICE_RSYSLOG_CONF_CONTENT = rf"""
if $programname == "{_QSFP_SERVICE_OSS}" then {_DEFAULT_OSS_LOG_DIR}/{_QSFP_SERVICE_OSS}.log
& stop
"""


def _cleanup_qsfp_service_rsyslog_conf() -> None:
    subprocess.run(f"rm {_QSFP_SERVICE_RSYSLOG_CONF_PATH}", shell=True)
    subprocess.run("systemctl restart rsyslog", shell=True)


def _setup_qsfp_service(
    qsfp_service_bin_path: t.Optional[str] = None,
    qsfp_service_config_path: t.Optional[str] = None,
    platform_mapping_override_path: t.Optional[str] = None,
    bsp_platform_mapping_override_path: t.Optional[str] = None,
) -> None:
    print(f"Setting up {_QSFP_SERVICE_OSS}")

    # Prepare qsfp_service binary and config file
    if not qsfp_service_bin_path:
        qsfp_service_bin_path = _DEFAULT_OSS_QSFP_SERVICE_PATH
    if not os.path.exists(qsfp_service_bin_path):
        raise Exception(f"qsfp_service binary: {qsfp_service_bin_path} does not exist")
    print(f"qsfp_service binary path: {qsfp_service_bin_path}")

    if not qsfp_service_config_path:
        raise Exception(f"qsfp_service config path: {qsfp_service_config_path} is None")
    if not os.path.exists(qsfp_service_config_path):
        raise Exception(
            f"qsfp_service config path: {qsfp_service_config_path} does not exist"
        )
    print(f"qsfp_service config path: {qsfp_service_config_path}")

    cleanup_qsfp_service()

    # Prepare qsfp_service unit file
    extra_args = ""
    if platform_mapping_override_path:
        if not os.path.exists(platform_mapping_override_path):
            raise Exception(
                f"platform_mapping override path: {platform_mapping_override_path} does not exist"
            )
        extra_args += (
            f"{_PLATFORM_MAPPING_OVERRIDE_PATH_ARG} {platform_mapping_override_path}"
        )
    if bsp_platform_mapping_override_path:
        if not os.path.exists(bsp_platform_mapping_override_path):
            raise Exception(
                f"bsp_platform_mapping override path: {bsp_platform_mapping_override_path} does not exist"
            )
        extra_args += f" {_BSP_PLATFORM_MAPPING_OVERRIDE_PATH_ARG} {bsp_platform_mapping_override_path}"
    qsfp_service_cmd = (
        f"{qsfp_service_bin_path} --qsfp-config {qsfp_service_config_path} {extra_args}"
    )
    with open(_QSFP_SERVICE_UNIT_FILE_PATH, "w") as f:
        f.write(
            _QSFP_SERVICE_UNIT_FILE_TEMPLATE.format(
                qsfp_service_cmd=qsfp_service_cmd,
            )
        )
        f.flush()

    # Prepare qsfp_service rsyslog conf
    with open(_QSFP_SERVICE_RSYSLOG_CONF_PATH, "w") as f:
        f.write(_QSFP_SERVICE_RSYSLOG_CONF_CONTENT)
        f.flush()
    subprocess.run("systemctl restart rsyslog; sleep 5", shell=True)


def _setup_qsfp_service_coldboot() -> None:
    subprocess.run(f"mkdir -p {_DEFAULT_QSFP_SERVICE_VOLATIRE_DIR}", shell=True)
    subprocess.run(
        f"touch {_DEFAULT_QSFP_SERVICE_VOLATIRE_DIR}/{_QSFP_SERVICE_COLD_BOOT_FILE}",
        shell=True,
    )


def _start_qsfp_service(is_warm_boot: bool = False) -> None:
    print(
        f"Starting {_QSFP_SERVICE_OSS} with "
        + ("WARMBOOT" if is_warm_boot else "COLDBOOT")
    )
    if not is_warm_boot:
        _setup_qsfp_service_coldboot()

    subprocess.run(f"systemctl enable {_QSFP_SERVICE_UNIT_FILE_PATH}", shell=True)
    subprocess.run("systemctl daemon-reload", shell=True)
    subprocess.run(f"systemctl start {_QSFP_SERVICE_OSS}; sleep 10", shell=True)


def setup_and_start_qsfp_service(
    qsfp_service_bin_path: t.Optional[str] = None,
    qsfp_service_config_path: t.Optional[str] = None,
    platform_mapping_override_path: t.Optional[str] = None,
    bsp_platform_mapping_override_path: t.Optional[str] = None,
    is_warm_boot: bool = False,
) -> None:
    # First setup qsfp_service unit file for systemd and rsyslog config for logging
    _setup_qsfp_service(
        qsfp_service_bin_path,
        qsfp_service_config_path,
        platform_mapping_override_path,
        bsp_platform_mapping_override_path,
    )

    # Then start qsfp_service using systemctl
    _start_qsfp_service(is_warm_boot)


def cleanup_qsfp_service() -> None:
    """
    Make sure no qsfp_service(PROD) or qsfp_service_for_testing(TESTING) is running and also disable
    qsfp_service_oss(OSS) if it is enabled.
    """
    print(f"Cleaning up {_QSFP_SERVICE_OSS}")

    subprocess.run(f"systemctl stop {_QSFP_SERVICE_PROD}", shell=True)
    subprocess.run(f"systemctl stop {_QSFP_SERVICE_FOR_TESTING}", shell=True)
    subprocess.run(f"systemctl stop {_QSFP_SERVICE_OSS}", shell=True)
    subprocess.run(f"systemctl disable {_QSFP_SERVICE_OSS}", shell=True)
    subprocess.run("systemctl daemon-reload", shell=True)
    # Just wanna be safe, also use pkill in case systemctl stop gets stuck
    subprocess.run(f"pkill -f {_QSFP_SERVICE_PROD}", shell=True)
    subprocess.run(f"pkill -f {_QSFP_SERVICE_FOR_TESTING}", shell=True)
    subprocess.run(f"pkill -f {_QSFP_SERVICE_OSS}", shell=True)

    _cleanup_qsfp_service_rsyslog_conf()
