"""Focused test for the login-containers PAM session hook.

Runs payload/usr/local/bin/session-recorder with stubbed PAM env and a
stub logger, asserting the open/close record shape, the always-zero exit
(the hook must never block login), and the fixed /usr/bin/logger path
(the script under test is a copy with only that path redirected, so the
message-construction logic is exercised while the fixed-path posture is
asserted separately against the committed file).
"""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

SCRIPT_NAME = "session-recorder"
ANCESTORS_NAME = "session-ancestors"
FIXED_LOGGER = "/usr/bin/logger"


def _script_bytes(name=SCRIPT_NAME):
    try:
        from importlib.resources import files

        package = __package__ or "login_containers.tests"
        return (files(package) / name).read_bytes()
    except (FileNotFoundError, ModuleNotFoundError):
        return (
            Path(__file__).resolve().parents[1] / "payload/usr/local/bin" / name
        ).read_bytes()


class SessionRecorderTest(unittest.TestCase):
    def _run_hook(self, env):
        tmp = Path(tempfile.mkdtemp(prefix="session-recorder-test-"))
        self.addCleanup(lambda: subprocess.run(["rm", "-rf", str(tmp)], check=False))
        capture = tmp / "logger.log"
        stub = tmp / "logger"
        stub.write_text(f'#!/bin/bash\necho "$*" >> {capture}\n')
        stub.chmod(0o755)
        script = tmp / SCRIPT_NAME
        script.write_bytes(
            _script_bytes().replace(FIXED_LOGGER.encode(), str(stub).encode())
        )
        script.chmod(0o755)
        run = subprocess.run(
            [str(script)],
            env=env,
            capture_output=True,
            text=True,
            timeout=30,
        )
        lines = capture.read_text().splitlines() if capture.exists() else []
        return run, lines

    def _expected_boot(self):
        try:
            return Path("/proc/sys/kernel/random/boot_id").read_text().strip()
        except OSError:
            return "unknown"

    def test_open_session_logs_open_record(self):
        run, lines = self._run_hook(
            {
                "PATH": "/usr/bin:/bin",
                "PAM_TYPE": "open_session",
                "PAM_USER": "netops",
                "PAM_SERVICE": "sshd",
            }
        )
        self.assertEqual(run.returncode, 0)
        self.assertEqual(len(lines), 1)
        self.assertIn("-t session-recorder", lines[0])
        for want in (
            "open user=netops",
            "service=sshd",
            f"sshdpid={os.getpid()}",
            f"boot={self._expected_boot()}",
        ):
            self.assertIn(want, lines[0])

    def test_close_session_logs_close_record(self):
        run, lines = self._run_hook(
            {
                "PATH": "/usr/bin:/bin",
                "PAM_TYPE": "close_session",
                "PAM_USER": "netadmin",
                "PAM_SERVICE": "sshd",
            }
        )
        self.assertEqual(run.returncode, 0)
        self.assertEqual(len(lines), 1)
        self.assertIn("close user=netadmin", lines[0])
        self.assertIn(f"sshdpid={os.getpid()}", lines[0])

    def test_unknown_type_logs_nothing_but_exits_zero(self):
        run, lines = self._run_hook(
            {
                "PATH": "/usr/bin:/bin",
                "PAM_TYPE": "auth",
                "PAM_USER": "netops",
                "PAM_SERVICE": "sshd",
            }
        )
        self.assertEqual(run.returncode, 0)
        self.assertEqual(lines, [])

    def test_missing_env_exits_zero_with_defaults(self):
        run, lines = self._run_hook({"PATH": "/usr/bin:/bin"})
        self.assertEqual(run.returncode, 0)
        self.assertEqual(lines, [])

    def test_logger_path_is_fixed(self):
        self.assertIn(FIXED_LOGGER, _script_bytes().decode())


class SessionAncestorsTest(unittest.TestCase):
    def _run_chain(self, proc, pid):
        tmp = Path(tempfile.mkdtemp(prefix="session-ancestors-test-"))
        self.addCleanup(lambda: subprocess.run(["rm", "-rf", str(tmp)], check=False))
        for child, ppid, comm in proc:
            d = tmp / str(child)
            d.mkdir()
            (d / "stat").write_text(f"{child} ({comm}) S {ppid} 0 0 0\n")
            (d / "comm").write_text(f"{comm}\n")
        script = tmp / ANCESTORS_NAME
        script.write_bytes(_script_bytes(ANCESTORS_NAME))
        script.chmod(0o755)
        return subprocess.run(
            [str(script), str(pid)],
            env={"PATH": "/usr/bin:/bin", "PROC_ROOT": str(tmp)},
            capture_output=True,
            text=True,
            timeout=30,
        )

    def test_chain_lists_leaf_to_init(self):
        run = self._run_chain(
            [
                (1, 0, "systemd"),
                (1468, 1, "sshd"),
                (3391, 1468, "sshd-session"),
                (3416, 3391, "sshd-session"),
            ],
            3416,
        )
        self.assertEqual(run.returncode, 0)
        self.assertEqual(
            run.stdout.strip(),
            "3416:sshd-session,3391:sshd-session,1468:sshd,1:systemd",
        )

    def test_missing_pid_exits_nonzero_without_output(self):
        run = self._run_chain([(1, 0, "systemd")], 9999)
        self.assertNotEqual(run.returncode, 0)
        self.assertEqual(run.stdout, "")

    def test_non_numeric_pid_exits_nonzero(self):
        run = self._run_chain([(1, 0, "systemd")], "nope")
        self.assertNotEqual(run.returncode, 0)
        self.assertEqual(run.stdout, "")

    def test_comm_is_sanitized_to_single_line(self):
        run = self._run_chain([(1, 0, "sys temd:x")], 1)
        self.assertEqual(run.returncode, 0)
        self.assertEqual(run.stdout.strip(), "1:sys_temd_x")

    def test_comm_containing_paren_space_parses_ppid(self):
        # Greedy strip through the LAST ") " always lands on the
        # comm-closing paren (post-comm fields are numeric), so an
        # adversarial comm must not shift the ppid field.
        run = self._run_chain(
            [
                (1, 0, "systemd"),
                (1468, 1, "sshd"),
                (3391, 1468, "a) b"),
            ],
            3391,
        )
        self.assertEqual(run.returncode, 0)
        self.assertEqual(run.stdout.strip(), "3391:a__b,1468:sshd,1:systemd")
