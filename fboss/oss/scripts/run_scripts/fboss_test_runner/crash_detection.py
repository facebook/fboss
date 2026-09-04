#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.

"""Detect a systemd unit crashing while a test binary runs.

Every runner leaves some production units running while its test binary
executes (sai_test keeps fsdb and the platform services up, sai_agent keeps
qsfp_service up, the fboss2 CLI suite runs against the production agents
themselves), and those units run with ``Restart=always``. A crash the test
caused is therefore invisible to gtest: systemd brings the unit back, the
binary's own readiness wait rides through it, and the test reports OK while
a core sits on disk and the hardware may never have received the config.

Two signals, both read after the binary exits, cover it:

* :func:`find_unclean_unit_exits` -- PID 1's own journal records of a unit's
  main process dying uncleanly inside the window.
* :func:`list_core_dumps` -- taken before and after the binary runs; a path
  in the second set but not the first is a core dumped inside the window.
"""

import json
import os
import re
import signal
import subprocess

_CORE_DUMP_DIRS = ["/var/core", "/var/lib/systemd/coredump"]

# The kernel truncates the process name embedded in a core file name
# (systemd-coredump: core.<comm>.<uid>.<boot>.<pid>.<ts>[.zst]) to
# TASK_COMM_LEN - 1 characters.
_TASK_COMM_LEN = 15

# systemd (PID 1) logs one line per main-process exit of every unit, e.g.
#   "fboss_hw_agent@0.service: Main process exited, code=dumped, status=6/ABRT"
# Parsing this line is preferable to `systemctl show -p NRestarts,Result`:
# NRestarts only moves for Restart=-triggered restarts (so a manual restart
# followed by a crash reads "unchanged"), and Result is reset to "success" the
# moment the unit is started again, so by the time a check runs after the test
# the crash has already been papered over. The journal keeps the record.
_MAIN_PROCESS_EXIT_RE = re.compile(
    r"^(?P<unit>\S+): Main process exited, code=(?P<code>\w+), "
    r"status=(?P<status>\d+)/(?P<name>\S+)$"
)
_OOM_KILL_RE = re.compile(
    r"^(?P<unit>\S+): A process of this unit has been killed by the OOM killer"
)

# Exits systemd's own `systemctl stop/restart` produces on a well-behaved unit:
# a clean exit, or termination by the stop signal (SIGTERM; SIGINT/SIGHUP for
# units that set KillSignal=) either as a signal death or as the 128+N status
# a shell/python wrapper returns when it relays it.
_GRACEFUL_SIGNALS = frozenset({"TERM", "INT", "HUP"})
_GRACEFUL_EXIT_STATUSES = frozenset(
    {0, 128 + signal.SIGTERM, 128 + signal.SIGINT, 128 + signal.SIGHUP}
)


def _classify_unit_exit(message: str) -> str | None:
    """Return a human-readable reason if `message` (a PID 1 journal line)
    records an unclean main-process exit of a unit, else None."""
    m = _OOM_KILL_RE.match(message)
    if m:
        return f"{m['unit']} killed by the OOM killer"
    m = _MAIN_PROCESS_EXIT_RE.match(message)
    if not m:
        return None
    code, status, name = m["code"], int(m["status"]), m["name"]
    if code == "exited" and status in _GRACEFUL_EXIT_STATUSES:
        return None
    if code == "killed" and name in _GRACEFUL_SIGNALS:
        return None
    what = {"dumped": "dumped core", "killed": "was killed", "exited": "exited"}.get(
        code, code
    )
    return f"{m['unit']} main process {what} (status={status}/{name})"


def find_unclean_unit_exits(start_time: float) -> list[str]:
    """Return one reason per systemd unit whose main process died uncleanly
    since `start_time` (crash, abort, OOM kill, non-zero exit), oldest first.

    Reads PID 1's journal records for the window. Clean stops and restarts --
    the ones the test infrastructure and the fboss2 CLI perform on purpose --
    exit 0 or die by SIGTERM and are not reported, so bouncing a unit inside
    the window is fine; only an exit systemd would count as a failure is.
    Units the runner stopped beforehand are inactive and produce nothing.
    Returns [] when journalctl is unavailable or fails (workstations,
    containers).

    The window is compared on the record's own microsecond timestamp, so a
    stop the runner performed right before launching the binary (a cold boot
    of the agents, say) is not charged to the test; `--since` only bounds how
    much journal is read.
    """
    since_usec = int(start_time * 1_000_000)
    try:
        result = subprocess.run(
            [
                "journalctl",
                "-q",
                "--no-pager",
                "-o",
                "json",
                f"--since=@{int(start_time)}",
                "_PID=1",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError:
        return []
    if result.returncode != 0:
        return []
    reasons: list[str] = []
    for line in result.stdout.splitlines():
        try:
            record = json.loads(line)
            message = record.get("MESSAGE")
            stamp = int(record.get("__REALTIME_TIMESTAMP", since_usec))
        except (ValueError, TypeError, AttributeError):
            continue
        if stamp < since_usec or not isinstance(message, str):
            continue
        reason = _classify_unit_exit(message)
        if reason:
            reasons.append(reason)
    return reasons


def list_core_dumps() -> set[str]:
    """Return the paths of every core file currently on disk.

    Callers take this before and after a test binary runs and report the
    difference. Comparing sets rather than mtimes means one core is charged
    to exactly one test, even when systemd-coredump finishes writing it a
    moment after the check that should have seen it, or when the next test
    starts within a second of the previous one. Unreadable directories and
    entries are skipped.
    """
    found: set[str] = set()
    for dir_path in _CORE_DUMP_DIRS:
        if not os.path.isdir(dir_path):
            continue
        try:
            with os.scandir(dir_path) as it:
                for entry in it:
                    try:
                        if entry.is_file():
                            found.add(entry.path)
                    except OSError:
                        continue
        except OSError:
            continue
    return found


def core_is_from(core_path: str, exe_name: str) -> bool:
    """True if the core file at `core_path` was dumped by `exe_name`."""
    name = os.path.basename(core_path)
    return name.startswith(f"core.{exe_name[:_TASK_COMM_LEN]}.") or exe_name in name
