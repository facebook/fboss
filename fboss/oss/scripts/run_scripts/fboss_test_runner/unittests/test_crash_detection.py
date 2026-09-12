#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.

"""Unit tests for fboss_test_runner.crash_detection."""

import json
from unittest.mock import MagicMock, patch

import fboss_test_runner.crash_detection as _mod
from fboss_test_runner.crash_detection import (
    core_is_from,
    find_unclean_unit_exits,
    list_core_dumps,
)


def _journal(*messages: str, stamp_usec: int = 5_000_000_000) -> str:
    """journalctl -o json output: one record per line, PID 1 style."""
    return "\n".join(
        json.dumps(
            {
                "_PID": "1",
                "UNIT": m.split(":", 1)[0],
                "MESSAGE": m,
                "__REALTIME_TIMESTAMP": str(stamp_usec),
            }
        )
        for m in messages
    )


def _patched_journalctl(stdout: str, returncode: int = 0):
    return patch.object(
        _mod.subprocess,
        "run",
        return_value=MagicMock(stdout=stdout, returncode=returncode),
    )


def test_unclean_exits_reports_crashes_not_clean_stops():
    """Cores, kills by anything but the stop signal, OOM kills and non-zero
    exits are crashes. A clean exit, a SIGTERM death and the 128+15 status a
    wrapper relays on SIGTERM are what `systemctl stop/restart` produces and
    must not fail a test (the fboss2 CLI restarts the agents on every
    commit; a console logout bounces serial-getty with exit 0)."""
    out = _journal(
        "fboss_hw_agent@0.service: Main process exited, code=dumped, status=6/ABRT",
        "fboss_sw_agent.service: Main process exited, code=exited, status=0/SUCCESS",
        "serial-getty@ttyS0.service: Main process exited, code=exited, status=0/SUCCESS",
        "qsfp_service.service: Main process exited, code=killed, status=15/TERM",
        "hostcfgd.service: Main process exited, code=exited, status=143/n/a",
        "sensor_service.service: Main process exited, code=killed, status=11/SEGV",
        "fan_service.service: Main process exited, code=exited, status=1/FAILURE",
        "platform_manager.service: A process of this unit has been killed by the OOM killer.",
        "fboss_hw_agent@0.service: Scheduled restart job, restart counter is at 1.",
        "Started FBOSS hw agent.",
    )
    with _patched_journalctl(out) as run:
        reasons = find_unclean_unit_exits(1_000.0)
    assert reasons == [
        "fboss_hw_agent@0.service main process dumped core (status=6/ABRT)",
        "sensor_service.service main process was killed (status=11/SEGV)",
        "fan_service.service main process exited (status=1/FAILURE)",
        "platform_manager.service killed by the OOM killer",
    ]
    # one journalctl call, PID 1 only, reading from the second the test started
    cmd = run.call_args[0][0]
    assert cmd[0] == "journalctl"
    assert "_PID=1" in cmd
    assert "--since=@1000" in cmd


def test_unclean_exits_uses_record_timestamp_not_since_granularity():
    """`--since=@N` is whole-second; the record's own microsecond timestamp
    decides. A cold boot the runner performed right before launching the
    binary (same second, earlier microseconds) is not charged to the test."""
    before = _journal(
        "fboss_sw_agent.service: Main process exited, code=dumped, status=6/ABRT",
        stamp_usec=1_000_400_000,
    )
    after = _journal(
        "fboss_hw_agent@0.service: Main process exited, code=dumped, status=6/ABRT",
        stamp_usec=1_000_900_000,
    )
    with _patched_journalctl(before + "\n" + after):
        assert find_unclean_unit_exits(1_000.5) == [
            "fboss_hw_agent@0.service main process dumped core (status=6/ABRT)"
        ]


def test_unclean_exits_ignores_unparseable_lines():
    out = (
        "not json\n"
        + json.dumps({"MESSAGE": ["array", "not", "str"]})
        + "\n"
        + _journal("x.service: Main process exited, code=dumped, status=11/SEGV")
    )
    with _patched_journalctl(out):
        assert find_unclean_unit_exits(0.0) == [
            "x.service main process dumped core (status=11/SEGV)"
        ]


def test_unclean_exits_empty_when_journalctl_unavailable():
    with _patched_journalctl("", returncode=1):
        assert find_unclean_unit_exits(0.0) == []
    with patch.object(_mod.subprocess, "run", side_effect=FileNotFoundError):
        assert find_unclean_unit_exits(0.0) == []


def test_list_core_dumps_lists_files_in_every_dir(tmp_path, monkeypatch):
    a = tmp_path / "coredump"
    b = tmp_path / "core"
    a.mkdir()
    b.mkdir()
    (a / "sub").mkdir()  # directories are not cores
    monkeypatch.setattr(
        _mod, "_CORE_DUMP_DIRS", [str(a), str(b), str(tmp_path / "nope")]
    )
    (a / "core.fboss_hw_agent-.0.abc.100.1000.zst").write_bytes(b"x")
    (b / "core.fboss_sw_agent.0.abc.300.3000").write_bytes(b"x")
    assert list_core_dumps() == {
        str(a / "core.fboss_hw_agent-.0.abc.100.1000.zst"),
        str(b / "core.fboss_sw_agent.0.abc.300.3000"),
    }


def test_list_core_dumps_missing_dir(tmp_path, monkeypatch):
    monkeypatch.setattr(_mod, "_CORE_DUMP_DIRS", [str(tmp_path / "does-not-exist")])
    assert list_core_dumps() == set()


def test_core_is_from_truncates_comm():
    """The kernel truncates the comm in the core name to 15 characters."""
    core = "/var/lib/systemd/coredump/core.sai_test-sai_im.0.x.5.6.zst"
    assert core_is_from(core, "sai_test-sai_impl")
    assert not core_is_from(core, "sai_agent_hw_test-sai_impl")
    assert core_is_from("/var/core/core.fboss_sw_agent.0.x.1.2", "fboss_sw_agent")
