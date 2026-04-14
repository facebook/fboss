# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import os
import subprocess
import typing as t

_DEFAULT_OSS_FSDB_SERVICE_PATH = "/opt/fboss/bin/fsdb"
# TODO: Add file.
_DEFAULT_OSS_FSDB_SERVICE_CONFIG_PATH = "/opt/fboss/share/link_test_configs/fsdb.conf"
_DEFAULT_OSS_LOG_DIR = "/opt/fboss/logs/"
_DEFAULT_FSDB_SERVICE_VOLATILE_DIR = "/tmp/fboss/fsdb_service"
_FSDB_SERVICE_COLD_BOOT_FILE = "cold_boot_once_fsdb_service"

# These two fsdb services are running in prod and testing environment.
_FSDB_SERVICE_PROD = "fsdb.service"
_FSDB_SERVICE_FOR_TESTING = "fsdb_service_for_testing"
# Introduce an oss version of fsdb_service to run in oss environment.
_FSDB_SERVICE_OSS = "fsdb_service_oss"

_FSDB_SERVICE_RSYSLOG_CONF_PATH = f"/etc/rsyslog.d/{_FSDB_SERVICE_OSS}.conf"
_FSDB_SERVICE_UNIT_FILE_PATH = f"/tmp/{_FSDB_SERVICE_OSS}.service"
_FSDB_SERVICE_RSYSLOG_CONF_CONTENT = rf"""
if $programname == "{_FSDB_SERVICE_OSS}" then {_DEFAULT_OSS_LOG_DIR}/{_FSDB_SERVICE_OSS}.log
& stop
"""
_FSDB_SERVICE_UNIT_FILE_TEMPLATE = rf"""
[Unit]
Description=FSDB Service For Testing

[Service]
LimitNOFILE=10000000
LimitCORE=32G
# Ensure memory usage is limited and oomd kills the process if it goes over
MemoryMax=2.4G
MemorySwapMax=0

Environment=TSAN_OPTIONS=die_after_fork=0
Environment=LD_LIBRARY_PATH=/opt/fboss/lib
ExecStart={{fsdb_service_cmd}}
SyslogIdentifier={_FSDB_SERVICE_OSS}
Restart=no

[Install]
WantedBy=multi-user.target
"""


def _cleanup_fsdb_service_rsyslog_conf() -> None:
    subprocess.run(f"rm {_FSDB_SERVICE_RSYSLOG_CONF_PATH}", check=False, shell=True)
    subprocess.run("systemctl restart rsyslog", check=False, shell=True)


def _setup_fsdb_service(
    fsdb_service_config_path: t.Optional[str] = None,
) -> None:
    print(f"Setting up {_FSDB_SERVICE_OSS}")

    # Prepare fsdb_service binary and config file
    fsdb_service_bin_path = _DEFAULT_OSS_FSDB_SERVICE_PATH
    if not os.path.exists(fsdb_service_bin_path):
        raise Exception(f"fsdb_service binary: {fsdb_service_bin_path} does not exist")

    print(f"fsdb_service binary path: {fsdb_service_bin_path}")

    if not fsdb_service_config_path:
        fsdb_service_config_path = _DEFAULT_OSS_FSDB_SERVICE_CONFIG_PATH
    if not os.path.exists(fsdb_service_config_path):
        raise Exception(
            f"fsdb_service config path: {fsdb_service_config_path} does not exist"
        )
    print(f"fsdb_service config path: {fsdb_service_config_path}")

    cleanup_fsdb_service()

    # Prepare fsdb
    fsdb_arg_for_config = f"--fsdb_config={fsdb_service_config_path}"
    fsdb_args_for_testing = "--checkOperOwnership=false  --ssl_policy=disabled"
    fsdb_args_for_prod_features = "--useIdPathsForSubs=false --serveHeartbeats"

    fsdb_service_cmd = f"{fsdb_service_bin_path} {fsdb_arg_for_config} {fsdb_args_for_testing} {fsdb_args_for_prod_features}"

    with open(_FSDB_SERVICE_UNIT_FILE_PATH, "w") as f:
        f.write(
            _FSDB_SERVICE_UNIT_FILE_TEMPLATE.format(
                fsdb_service_cmd=fsdb_service_cmd,
            )
        )
        f.flush()

    # Prepare fsdb_service rsyslog conf
    with open(_FSDB_SERVICE_RSYSLOG_CONF_PATH, "w") as f:
        f.write(_FSDB_SERVICE_RSYSLOG_CONF_CONTENT)
        f.flush()
    subprocess.run("systemctl restart rsyslog; sleep 5", check=False, shell=True)


def _setup_fsdb_service_coldboot() -> None:
    subprocess.run(
        f"mkdir -p {_DEFAULT_FSDB_SERVICE_VOLATILE_DIR}", check=False, shell=True
    )
    subprocess.run(
        f"touch {_DEFAULT_FSDB_SERVICE_VOLATILE_DIR}/{_FSDB_SERVICE_COLD_BOOT_FILE}",
        check=False,
        shell=True,
    )


def _start_fsdb_service(is_warm_boot: bool = False) -> None:
    print(
        f"Starting {_FSDB_SERVICE_OSS} with "
        + ("WARMBOOT" if is_warm_boot else "COLDBOOT")
    )
    if not is_warm_boot:
        _setup_fsdb_service_coldboot()

    subprocess.run(
        f"systemctl enable {_FSDB_SERVICE_UNIT_FILE_PATH}", check=False, shell=True
    )
    subprocess.run("systemctl daemon-reload", check=False, shell=True)
    subprocess.run(
        f"systemctl start {_FSDB_SERVICE_OSS}; sleep 10", check=False, shell=True
    )


def setup_and_start_fsdb_service(
    fsdb_service_config_path: t.Optional[str] = None,
    is_warm_boot: bool = False,
) -> None:
    _setup_fsdb_service(fsdb_service_config_path)
    _start_fsdb_service(is_warm_boot)


def cleanup_fsdb_service() -> None:
    """
    Make sure no fsdb_service service/binary is running and also disable
    fsdb_service_for_testing
    """
    print(f"Cleaning up {_FSDB_SERVICE_OSS}")

    subprocess.run(f"systemctl stop {_FSDB_SERVICE_PROD}", check=False, shell=True)
    subprocess.run(
        f"systemctl stop {_FSDB_SERVICE_FOR_TESTING}", check=False, shell=True
    )
    subprocess.run(f"systemctl stop {_FSDB_SERVICE_OSS}", check=False, shell=True)
    subprocess.run(f"systemctl disable {_FSDB_SERVICE_OSS}", check=False, shell=True)
    subprocess.run("systemctl daemon-reload", check=False, shell=True)

    # Just wanna be safe, also use pkill in case systemctl stop gets stuck
    subprocess.run(f"pkill -f {_FSDB_SERVICE_PROD}", check=False, shell=True)
    subprocess.run(f"pkill -f {_FSDB_SERVICE_FOR_TESTING}", check=False, shell=True)
    subprocess.run(f"pkill -f {_FSDB_SERVICE_OSS}", check=False, shell=True)

    _cleanup_fsdb_service_rsyslog_conf()
