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
HW_AGENT_SERVICE_PROD = "fboss_hw_agent@"
_HW_AGENT_SERVICE_FOR_TESTING = "fboss_hw_agent_for_testing_"

SW_AGENT_SERVICE_PROD = "fboss_sw_agent"
_DEFAULT_OSS_SW_AGENT_SERVICE_PATH = "/opt/fboss/bin/fboss_sw_agent"

_SW_AGENT_SERVICE_UNIT_FILE_TEMPLATE = """
[Unit]
Description=FBOSS SW Agent Service

[Service]
LimitNOFILE=10000000
LimitCORE=32G
MemoryMax=3.75G
MemorySwapMax=0

Environment=TSAN_OPTIONS=die_after_fork=0
Environment=LD_LIBRARY_PATH=/opt/fboss/lib
ExecStart={sw_agent_service_cmd}
SyslogIdentifier={service_name}
Restart=no

[Install]
WantedBy=multi-user.target
"""

FBOSS_WARMBOOT_DIR = f"{FBOSS_AGENT_VOLATILE_STATE_DIR}/warm_boot"

_HW_AGENT_SERVICE_OSS = "fboss_hw_agent_oss@"
_HW_AGENT_SERVICE_UNIT_FILE_TEMPLATE = """
[Unit]
Description=FBOSS HW Agent Service

[Service]
LimitNOFILE=10000000
LimitCORE=32G
MemoryMax=3.75G
MemorySwapMax=0

Environment=TSAN_OPTIONS=die_after_fork=0
Environment=LD_LIBRARY_PATH=/opt/fboss/lib
ExecStart={hw_agent_service_cmd}
SyslogIdentifier={service_name}{switch_index}
Restart=no

[Install]
WantedBy=multi-user.target
"""


def agent_can_warm_boot_file_path(switch_index: t.Optional[int] = None) -> str:
    if switch_index is None:
        return FBOSS_AGENT_WB_FLAG_FILE
    return f"{FBOSS_AGENT_WB_FLAG_FILE}_{switch_index}"


def cleanup_hw_agent_service(switch_indexes: t.List[int]) -> None:
    for switch_index in switch_indexes:
        print(f"Cleaning up FBOSS HW Agent Service for index={switch_index}...")
        for svc in [
            HW_AGENT_SERVICE_PROD,
            _HW_AGENT_SERVICE_FOR_TESTING,
            _HW_AGENT_SERVICE_OSS,
        ]:
            subprocess.run(f"systemctl stop {svc}{switch_index}", shell=True)
            subprocess.run(f"pkill -f {svc}{switch_index}", shell=True)
            subprocess.run(f"rm -f /etc/rsyslog.d/{svc}{switch_index}.conf", shell=True)
        subprocess.run(
            f"systemctl disable {_HW_AGENT_SERVICE_OSS}{switch_index}", shell=True
        )
        subprocess.run("systemctl daemon-reload", shell=True)
    subprocess.run("systemctl restart rsyslog", shell=True)


def _manage_hw_agent_service(
    switch_indexes: t.List[int],
    isStart: bool,
    service_name: str = _HW_AGENT_SERVICE_OSS,
) -> t.List[int]:
    return_codes = []
    op = "start" if isStart else "stop"
    op_ing = "Starting" if isStart else "Stopping"
    for switch_index in switch_indexes:
        print(f"{op_ing} FBOSS HW Agent Service for index={switch_index}...")
        ret = subprocess.run(
            f"systemctl {op} {service_name}{switch_index}", shell=True
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
    service_name: str = _HW_AGENT_SERVICE_OSS,
) -> t.List[int]:
    if not skipStopping:
        return_codes = _manage_hw_agent_service(switch_indexes, False, service_name)
        if any(return_codes):
            print("Error while stopping HW Agent service")
            return return_codes

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

    return_codes = _manage_hw_agent_service(switch_indexes, True, service_name)
    if any(return_codes):
        print("Error while starting HW Agent service")
    return return_codes


def cold_boot_hw_agent(
    switch_indexes: t.List[int],
    skipStopping: bool = False,
    service_name: str = _HW_AGENT_SERVICE_OSS,
) -> t.List[int]:
    if not skipStopping:
        return_codes = _manage_hw_agent_service(switch_indexes, False, service_name)
        if any(return_codes):
            print("Error while stopping HW Agent service")
            return return_codes

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

    return_codes = _manage_hw_agent_service(switch_indexes, True, service_name)
    if any(return_codes):
        print("Error while starting HW Agent service")
    return return_codes


def _setup_hw_agent_service(
    switch_indexes: t.List[int],
    fboss_agent_config_path: str,
    hw_agent_service_bin_path: t.Optional[str] = None,
    platform_mapping_override_path: t.Optional[str] = None,
    sai_replayer_log_path: t.Optional[str] = None,
    is_fsdb_disabled: bool = False,
    hw_agent_service_name: str = _HW_AGENT_SERVICE_OSS,
    hw_agent_for_testing: bool = True,
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
        service_full_name = f"{hw_agent_service_name}{switch_index}"
        print(f"Setting up FBOSS HW Agent Service {service_full_name}...")

        extra_args = "--hw_agent_for_testing=true" if hw_agent_for_testing else ""
        if platform_mapping_override_path:
            if not os.path.exists(platform_mapping_override_path):
                raise Exception(
                    f"Platform mapping override path {platform_mapping_override_path} does not exist"
                )
            extra_args += f" {_PLATFORM_MAPPING_OVERRIDE_PATH_ARG} {platform_mapping_override_path}"
        if sai_replayer_log_path:
            extra_args += f" --sai_log {sai_replayer_log_path}"

        if not is_fsdb_disabled:
            extra_args += " --fsdb_client_ssl_preferred=false"

        hw_agent_service_cmd = f"{hw_agent_service_bin_path} --config {fboss_agent_config_path} --switchIndex {switch_index} {extra_args}"
        unit_file_path = f"/tmp/{service_full_name}.service"
        with open(unit_file_path, "w") as f:
            f.write(
                _HW_AGENT_SERVICE_UNIT_FILE_TEMPLATE.format(
                    hw_agent_service_cmd=hw_agent_service_cmd,
                    service_name=hw_agent_service_name,
                    switch_index=switch_index,
                )
            )
            f.flush()

        rsyslog_conf_path = f"/etc/rsyslog.d/{service_full_name}.conf"
        rsyslog_content = (
            f'if $programname == "{service_full_name}" '
            f"then {_DEFAULT_OSS_LOG_DIR}/{service_full_name}.log\n"
            f"& stop\n"
        )
        with open(rsyslog_conf_path, "w") as f:
            f.write(rsyslog_content)
            f.flush()

    subprocess.run("systemctl restart rsyslog; sleep 5", shell=True)
    for switch_index in switch_indexes:
        unit_file = f"/tmp/{hw_agent_service_name}{switch_index}.service"
        subprocess.run(f"systemctl enable {unit_file}", shell=True)
    subprocess.run("systemctl daemon-reload", shell=True)


def setup_and_start_hw_agent_service(
    switch_indexes: t.List[int],
    fboss_agent_config_path: str,
    hw_agent_service_bin_path: t.Optional[str] = None,
    platform_mapping_override_path: t.Optional[str] = None,
    sai_replayer_log_path: t.Optional[str] = None,
    is_fsdb_disabled: bool = False,
    is_warm_boot: bool = False,
    hw_agent_service_name: str = _HW_AGENT_SERVICE_OSS,
    hw_agent_for_testing: bool = True,
) -> None:
    _setup_hw_agent_service(
        switch_indexes,
        fboss_agent_config_path,
        hw_agent_service_bin_path,
        platform_mapping_override_path,
        sai_replayer_log_path,
        is_fsdb_disabled,
        hw_agent_service_name=hw_agent_service_name,
        hw_agent_for_testing=hw_agent_for_testing,
    )

    return_codes = (
        warm_boot_hw_agent(switch_indexes, service_name=hw_agent_service_name)
        if is_warm_boot
        else cold_boot_hw_agent(switch_indexes, service_name=hw_agent_service_name)
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


def cleanup_sw_agent_service(
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> None:
    print(f"Cleaning up FBOSS SW Agent Service ({sw_agent_service_name})...")
    subprocess.run(f"systemctl stop {sw_agent_service_name}", shell=True)
    subprocess.run(f"systemctl disable {sw_agent_service_name}", shell=True)
    subprocess.run("systemctl daemon-reload", shell=True)
    subprocess.run(f"pkill -f {sw_agent_service_name}", shell=True)
    subprocess.run(f"rm -f /etc/rsyslog.d/{sw_agent_service_name}.conf", shell=True)
    subprocess.run("systemctl restart rsyslog", shell=True)


def _manage_sw_agent_service(
    is_start: bool,
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> int:
    op = "start" if is_start else "stop"
    op_ing = "Starting" if is_start else "Stopping"
    print(f"{op_ing} FBOSS SW Agent Service ({sw_agent_service_name})...")
    ret = subprocess.run(
        f"systemctl {op} {sw_agent_service_name}", shell=True
    ).returncode
    if ret != 0:
        print(
            f"Failed to {op} FBOSS SW Agent Service ({sw_agent_service_name}) "
            f"with return code {ret}"
        )
    return ret


def cold_boot_sw_agent(
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> int:
    ret = _manage_sw_agent_service(False, sw_agent_service_name)
    if ret != 0:
        print("Error while stopping SW Agent for cold boot")
        return ret

    ret = subprocess.run(
        f"rm -f {agent_can_warm_boot_file_path()}", shell=True
    ).returncode
    if ret != 0:
        print("Error while removing SW Agent warmboot flag")
        return ret

    ret = _manage_sw_agent_service(True, sw_agent_service_name)
    if ret != 0:
        print("Error while starting SW Agent after cold boot")
    return ret


def warm_boot_sw_agent(
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> int:
    ret = _manage_sw_agent_service(False, sw_agent_service_name)
    if ret != 0:
        print("Error while stopping SW Agent for warm boot")
        return ret

    ret = subprocess.run(
        f"stat {FBOSS_WARMBOOT_DIR}/can_warm_boot", shell=True
    ).returncode
    if ret != 0:
        print("Error while checking SW Agent warmboot flag")
        return ret

    ret = _manage_sw_agent_service(True, sw_agent_service_name)
    if ret != 0:
        print("Error while starting SW Agent after warm boot")
    return ret


def _setup_sw_agent_service(
    fboss_agent_config_path: str,
    sw_agent_service_bin_path: t.Optional[str] = None,
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> None:
    if not sw_agent_service_bin_path:
        sw_agent_service_bin_path = _DEFAULT_OSS_SW_AGENT_SERVICE_PATH
    if not os.path.exists(sw_agent_service_bin_path):
        raise Exception(
            f"SW Agent Service binary path {sw_agent_service_bin_path} does not exist"
        )
    print(f"SW Agent Service binary path: {sw_agent_service_bin_path}")

    if not os.path.exists(fboss_agent_config_path):
        raise Exception(f"Agent config path {fboss_agent_config_path} does not exist")

    cleanup_sw_agent_service(sw_agent_service_name)

    print(f"Setting up FBOSS SW Agent Service ({sw_agent_service_name})...")
    sw_agent_service_cmd = (
        f"{sw_agent_service_bin_path} --config {fboss_agent_config_path}"
        " --multi_switch --fsdb_client_ssl_preferred=false"
    )
    unit_file_path = f"/tmp/{sw_agent_service_name}.service"
    with open(unit_file_path, "w") as f:
        f.write(
            _SW_AGENT_SERVICE_UNIT_FILE_TEMPLATE.format(
                sw_agent_service_cmd=sw_agent_service_cmd,
                service_name=sw_agent_service_name,
            )
        )
        f.flush()

    rsyslog_conf_path = f"/etc/rsyslog.d/{sw_agent_service_name}.conf"
    rsyslog_content = (
        f'if $programname == "{sw_agent_service_name}" '
        f"then {_DEFAULT_OSS_LOG_DIR}/{sw_agent_service_name}.log\n"
        f"& stop\n"
    )
    with open(rsyslog_conf_path, "w") as f:
        f.write(rsyslog_content)
        f.flush()

    subprocess.run("systemctl restart rsyslog; sleep 5", shell=True)
    subprocess.run(f"systemctl enable {unit_file_path}", shell=True)
    subprocess.run("systemctl daemon-reload", shell=True)


def setup_and_start_sw_agent_service(
    fboss_agent_config_path: str,
    sw_agent_service_bin_path: t.Optional[str] = None,
    is_warm_boot: bool = False,
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> None:
    _setup_sw_agent_service(
        fboss_agent_config_path,
        sw_agent_service_bin_path,
        sw_agent_service_name=sw_agent_service_name,
    )

    ret = (
        warm_boot_sw_agent(sw_agent_service_name)
        if is_warm_boot
        else cold_boot_sw_agent(sw_agent_service_name)
    )
    if ret != 0:
        raise Exception(
            "Error while starting SW Agent service in {} mode".format(
                "warm-boot" if is_warm_boot else "cold-boot"
            )
        )
    print(
        "Successfully started SW Agent service in {} mode".format(
            "warm-boot" if is_warm_boot else "cold-boot"
        )
    )


def is_agent_running(
    switch_indexes: t.List[int],
    hw_agent_service_name: str = HW_AGENT_SERVICE_PROD,
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> t.List[bool]:
    """Check if sw_agent and hw_agent services are currently active.

    Returns a list of bools: index 0 is sw_agent, rest are hw_agent per switch index.
    """
    results: t.List[bool] = []
    ret = subprocess.run(
        f"systemctl is-active --quiet {sw_agent_service_name}", shell=True
    ).returncode
    results.append(ret == 0)
    for switch_index in switch_indexes:
        ret = subprocess.run(
            f"systemctl is-active --quiet {hw_agent_service_name}{switch_index}",
            shell=True,
        ).returncode
        results.append(ret == 0)
    return results


def cold_boot_agents(
    switch_indexes: t.List[int],
    hw_agent_service_name: str = HW_AGENT_SERVICE_PROD,
    sw_agent_service_name: str = SW_AGENT_SERVICE_PROD,
) -> None:
    """Cold boot both hw_agent and sw_agent for test isolation.

    Stops both agents, cleans all warmboot state, sets cold boot flags,
    then starts both agents. Raises Exception if any step fails.
    """
    if not switch_indexes:
        raise ValueError("switch indexes cannot be empty")

    def _check(return_codes: t.List[int], step: str) -> None:
        if any(return_codes):
            raise Exception(f"Error during cold boot agents: {step}")

    # Step 1: Stop both agents
    return_codes = [_manage_sw_agent_service(False, sw_agent_service_name)]
    return_codes.extend(
        _manage_hw_agent_service(switch_indexes, False, hw_agent_service_name)
    )
    _check(return_codes, "stopping agents")

    # Step 2: Clean warmboot state
    print(f"Cleaning warm boot state: rm -rf {FBOSS_WARMBOOT_DIR}")
    ret = subprocess.run(f"rm -rf {FBOSS_WARMBOOT_DIR}", shell=True).returncode
    if ret != 0:
        raise Exception("Error during cold boot agents: cleaning warm boot state")

    # Step 3: Set cold boot flags
    print("Setting cold boot flags for hw and sw agents")
    ret = subprocess.run(f"mkdir -p {FBOSS_WARMBOOT_DIR}", shell=True).returncode
    if ret != 0:
        raise Exception("Error during cold boot agents: creating warm boot dir")
    for switch_index in switch_indexes:
        ret = subprocess.run(
            f"touch {FBOSS_WARMBOOT_DIR}/hw_cold_boot_once_{switch_index}",
            shell=True,
        ).returncode
        if ret != 0:
            raise Exception(
                f"Error during cold boot agents: setting hw cold boot flag for index={switch_index}"
            )
    ret = subprocess.run(
        f"touch {FBOSS_WARMBOOT_DIR}/sw_cold_boot_once", shell=True
    ).returncode
    if ret != 0:
        raise Exception("Error during cold boot agents: setting sw cold boot flag")

    # Step 4: Start both agents
    return_codes = list(
        _manage_hw_agent_service(switch_indexes, True, hw_agent_service_name)
    )
    _check(return_codes, "starting hw agents")
    return_codes.append(_manage_sw_agent_service(True, sw_agent_service_name))
    _check(return_codes, "starting sw agent")
