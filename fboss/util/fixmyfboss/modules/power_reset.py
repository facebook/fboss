# pyre-strict
import os
import re
from datetime import datetime, timedelta
from pathlib import Path

from fboss.util.fixmyfboss.check import check
from fboss.util.fixmyfboss.status import Problem, Warning

var_secure_logfile = Path("/var/log/secure")
var_secure_date_format = "%b %d %H:%M:%S"
default_lookback = timedelta(days=7)

WATCHDOG_LOG_WARNING_LIMIT = 2


def filter_logs_by_date(
    logs: list[str], time_format: str, lookback_time: timedelta = default_lookback
) -> list[str]:
    cutoff_date = datetime.now() - lookback_time
    time_str_len = len(datetime.now().strftime(time_format))

    filtered_logs = []
    for log in logs:
        try:
            log_date = None
            if "Y" not in time_format:
                log_date = datetime.strptime(
                    f"{datetime.now().year} {log[:time_str_len]}", f"%Y {time_format}"
                )
            else:
                log_date = datetime.strptime(f"{log[:time_str_len]}", f"{time_format}")
            if log_date >= cutoff_date:
                filtered_logs.append(log)
        except ValueError:
            continue
    return filtered_logs


def grep_file_for_pattern(file_path: Path, pattern: str) -> list[str]:
    """
    Grep a file for a pattern and return all lines containing that pattern.
    """
    matching_lines = []
    if not file_path.exists():
        return matching_lines

    with open(file_path, "r") as file:
        for line in file:
            if re.search(pattern, line):
                matching_lines.append(line.strip())
    return matching_lines


@check
def recent_manual_reboot() -> Warning | None:
    """
    Check if a reboot has been performed recently (in last 1 day)
    """
    pattern = "System is rebooting"
    reason = "System was rebooted with `reboot` command"
    reboot_logs = filter_logs_by_date(
        grep_file_for_pattern(var_secure_logfile, pattern),
        var_secure_date_format,
        timedelta(days=1),
    )

    if len(reboot_logs) == 0:
        return None

    desc_str = f"\n{reason}\n" + "\n".join(" " + log for log in reboot_logs)
    return Warning(description=(desc_str))


@check
def recent_kernel_panic() -> Problem | None:
    """
    Check if a kernel panic has occured recently (last 7 days)
    """
    crash_dir = "/var/crash/processed"
    processed_dir_date_format = "%Y-%m-%d%H:%M:%S"

    recent_panics = []
    cutoff_date = datetime.now() - timedelta(days=7)
    if not os.path.exists(crash_dir):
        return None

    for entry in os.listdir(crash_dir):
        date_str = re.sub(r"[a-zA-Z]", "", entry)
        try:
            entry_date = datetime.strptime(date_str, processed_dir_date_format)
            if entry_date >= cutoff_date:
                recent_panics.append(entry)
        except ValueError:
            continue

    if len(recent_panics) == 0:
        return None

    reason = "Recent kernel panic" + ("s" if len(recent_panics) > 1 else "")
    return Problem(
        description=(
            f"\n{reason}\n" + "\n".join(" " + panic for panic in recent_panics)
        ),
        manual_remediation=(
            "Investigate the kernel panics by looking at the logs in the /var/crash/processed directory."
        ),
    )


@check
def watchdog_did_not_stop() -> Problem | Warning | None:
    """
    Check dmesg for logs indicating that a watchdog did not stop (within the last 3 hours).
    """
    dmesg_output = os.popen("dmesg -T").read().splitlines()
    dmesg_date_format = "[%a %b %d %H:%M:%S %Y]"
    watchdog_logs = filter_logs_by_date(
        [
            line
            for line in dmesg_output
            if re.search(r"(?i)watchdog did not stop", line)
        ],
        dmesg_date_format,
        timedelta(hours=3),
    )

    log_count = len(watchdog_logs)
    if log_count == 0:
        return None
    if log_count <= WATCHDOG_LOG_WARNING_LIMIT:
        return Warning(
            description=(
                f"Watchdog did not stop {log_count} time(s) in the last 3 hours. "
                "This usually indicates someone manually kicked the watchdog. Continue "
                "monitoring the system to see if the issue persists."
            )
        )
    return Problem(
        description=(
            "Watchdog did not stop. This could indicate a process managing watchdog"
            "(like fan_service in fboss switch) exited without closing the watchdog in the "
            "proper way (using MAGICCLOSE feature). If no one is kicking the watchdog "
            "before the watchdog timer counts down, it can start its corrective "
            "action (like running fan at high speed for fan_watchdog)."
        )
    )
