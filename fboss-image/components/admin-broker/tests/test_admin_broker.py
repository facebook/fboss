# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Whitebox tests for the admin-broker daemon.

Loads payload/usr/local/sbin/admin-broker as a module (BUCK resource, with a
repo-relative fallback like the login-containers tests) and exercises every
function, including the serve loop and the __main__ guard. SelfCoverageTest
re-runs the whole suite under stdlib trace and fails unless every executable
daemon line ran: the 100% gate travels with the tests, under BUCK or raw.
"""

import dis
import errno
import io
import itertools
import os
import runpy
import shutil
import signal
import socket
import struct
import sys
import tempfile
import threading
import time
import trace
import unittest
from pathlib import Path
from unittest import mock

SCRIPT_NAME = "admin-broker"
MODNAME = "admin_broker_under_test"

_name_counter = itertools.count()


def _daemon_path():
    try:
        from importlib.resources import files

        package = __package__ or "admin_broker.tests"
        return str(files(package) / SCRIPT_NAME)
    except (FileNotFoundError, ModuleNotFoundError):
        return str(
            Path(__file__).resolve().parents[1] / "payload/usr/local/sbin" / SCRIPT_NAME
        )


def load_daemon():
    import importlib.util
    from importlib.machinery import SourceFileLoader

    loader = SourceFileLoader(MODNAME, _daemon_path())
    spec = importlib.util.spec_from_loader(MODNAME, loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[MODNAME] = module
    loader.exec_module(module)
    return module


broker = load_daemon()


def _write_proc(root, pid, status=None, comm=None, cmdline=None, cgroup=None):
    """Build one fake /proc/<pid> with exactly the given files."""
    proc_dir = Path(root) / str(pid)
    proc_dir.mkdir(parents=True, exist_ok=True)
    if status is not None:
        (proc_dir / "status").write_text(status)
    if comm is not None:
        (proc_dir / "comm").write_text(comm)
    if cmdline is not None:
        (proc_dir / "cmdline").write_bytes(cmdline)
    if cgroup is not None:
        (proc_dir / "cgroup").write_text(cgroup)
    return str(proc_dir)


def _status(uid):
    return "Name:\tprobe\nUid:\t%s\t%s\t%s\t%s\nGid:\t1\t1\t1\t1\n" % (
        uid,
        uid,
        uid,
        uid,
    )


def _mktemp(testcase, prefix):
    tmp = Path(tempfile.mkdtemp(prefix=prefix))
    testcase.addCleanup(lambda: shutil.rmtree(str(tmp), ignore_errors=True))
    return tmp


def _abstract_name():
    return "\0admin-broker-test-%d-%d" % (os.getpid(), next(_name_counter))


def _read_all(sock):
    chunks = []
    while True:
        data = sock.recv(65536)
        if not data:
            break
        chunks.append(data)
    return b"".join(chunks)


class PeerIdentityTest(unittest.TestCase):
    def test_socketpair_reports_self(self):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        ident = broker.peer_identity(ours)
        self.assertEqual(ident["uid"], os.getuid())
        self.assertEqual(ident["gid"], os.getgid())
        self.assertEqual(ident["pid"], os.getpid())


class OriginTest(unittest.TestCase):
    def test_user_scope_parses(self):
        text = "1:name=systemd:/\n0::/user.slice/user-1001.slice/session-85.scope\n"
        self.assertEqual(
            broker.origin_from_cgroup(text),
            (1001, "user-1001.slice/session-85.scope"),
        )

    def test_non_session_cgroup_yields_none(self):
        self.assertEqual(
            broker.origin_from_cgroup("0::/system.slice/sshd.service\n"), (None, None)
        )
        self.assertEqual(broker.origin_from_cgroup(""), (None, None))

    def test_read_origin_from_fixture(self):
        root = _mktemp(self, "origin-")
        _write_proc(root, 7, cgroup="0::/user.slice/user-1000.slice/session-3.scope\n")
        self.assertEqual(
            broker.read_origin(7, str(root)), (1000, "user-1000.slice/session-3.scope")
        )

    def test_read_origin_missing_pid(self):
        root = _mktemp(self, "origin-")
        self.assertEqual(broker.read_origin(7, str(root)), (None, None))

    def test_read_origin_unmatched_cgroup(self):
        root = _mktemp(self, "origin-")
        _write_proc(root, 7, cgroup="0::/system.slice/cron.service\n")
        self.assertEqual(broker.read_origin(7, str(root)), (None, None))


class CommTest(unittest.TestCase):
    def test_read_comm(self):
        root = _mktemp(self, "comm-")
        _write_proc(root, 9, comm="python3.9\n")
        self.assertEqual(broker.read_comm(9, str(root)), "python3.9")

    def test_read_comm_missing(self):
        root = _mktemp(self, "comm-")
        self.assertEqual(broker.read_comm(9, str(root)), "?")


class RoleTest(unittest.TestCase):
    def test_origin_wins_over_peer(self):
        self.assertEqual(broker.resolve_role(0, 1001), "netadmin")

    def test_peer_fallback(self):
        self.assertEqual(broker.resolve_role(1000, None), "netops")

    def test_unknown(self):
        self.assertEqual(broker.resolve_role(0, None), "unknown")
        self.assertEqual(broker.resolve_role(4242, 4242), "unknown")


class AuditTest(unittest.TestCase):
    def test_line_shape(self):
        line = broker.audit_line(
            "WHOAMI",
            {"uid": 1001, "gid": 1002, "pid": 55},
            "netadmin",
            "user-1001.slice/session-1.scope",
            "-",
            "ok",
        )
        self.assertRegex(
            line,
            r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[+-]\d{4} "
            r"verb=WHOAMI peer_uid=1001 peer_gid=1002 peer_pid=55 "
            r"role=netadmin session=user-1001\.slice/session-1\.scope "
            r"target=- result=ok detail=-\n$",
        )
        self.assertNotIn("\t", line)

    def test_line_with_target(self):
        line = broker.audit_line(
            "RESTART",
            {"uid": 1001, "gid": 1002, "pid": 55},
            "netadmin",
            "user-1001.slice/session-1.scope",
            "fboss_sw_agent.service",
            "ok",
        )
        self.assertIn("target=fboss_sw_agent.service result=ok", line)

    def test_line_with_detail(self):
        line = broker.audit_line(
            "EXEC",
            {"uid": 0, "gid": 0, "pid": 9},
            "netadmin",
            "user-1001.slice/session-1.scope",
            "/bin/ps",
            "started",
            "/bin/ps -e",
        )
        self.assertIn("result=started detail=/bin/ps -e\n", line)

    def test_append_roundtrip(self):
        tmp = _mktemp(self, "audit-")
        path = str(tmp / "audit.log")
        broker.append_audit(path, "line-one\n")
        broker.append_audit(path, "line-two\n")
        with open(path) as handle:
            self.assertEqual(handle.read(), "line-one\nline-two\n")


class SanitizeTest(unittest.TestCase):
    def test_flattens(self):
        self.assertEqual(
            broker.sanitize_cmdline(b"/bin/sleep\x00300\x00"), "/bin/sleep 300"
        )

    def test_collapses_whitespace(self):
        self.assertEqual(broker.sanitize_cmdline(b"a  b\t\tc\n\nd"), "a b c d")

    def test_truncates(self):
        self.assertEqual(len(broker.sanitize_cmdline(b"x" * 5000)), 1024)

    def test_replaces_bad_bytes(self):
        out = broker.sanitize_cmdline(b"\xff\xfe")
        self.assertEqual(out, "\ufffd\ufffd")

    def test_empty(self):
        self.assertEqual(broker.sanitize_cmdline(b""), "")


class ReadProcessTest(unittest.TestCase):
    def test_ok(self):
        root = _mktemp(self, "ps-")
        _write_proc(
            root, 42, status=_status(1001), comm="sleep\n", cmdline=b"sleep\x00300\x00"
        )
        self.assertEqual(
            broker.read_process(str(root), "42"), ("1001", "sleep", "sleep 300")
        )

    def test_missing_pid(self):
        root = _mktemp(self, "ps-")
        self.assertIsNone(broker.read_process(str(root), "42"))

    def test_malformed_status(self):
        root = _mktemp(self, "ps-")
        _write_proc(root, 42, status="Name:\todd\n", comm="odd\n", cmdline=b"odd\x00")
        self.assertIsNone(broker.read_process(str(root), "42"))


class ListProcessesTest(unittest.TestCase):
    def test_sorted_skips_garbage(self):
        root = _mktemp(self, "ps-")
        _write_proc(root, 10, status=_status(0), comm="ten\n", cmdline=b"ten\x00")
        _write_proc(root, 2, status=_status(1001), comm="two\n", cmdline=b"two\x00")
        _write_proc(root, 3, comm="broken\n")
        (Path(str(root)) / "self").mkdir()
        lines = broker.list_processes(str(root))
        self.assertEqual(lines, ["2 1001 two two", "10 0 ten ten"])

    def test_missing_root(self):
        root = _mktemp(self, "ps-")
        self.assertEqual(broker.list_processes(str(root / "nope")), [])


class HandleCommandTest(unittest.TestCase):
    def _ident(self, uid=1001, pid=7):
        return {"uid": uid, "gid": 1002, "pid": pid}

    def test_whoami_with_session(self):
        root = _mktemp(self, "cmd-")
        _write_proc(
            root,
            7,
            comm="bash\n",
            cgroup="0::/user.slice/user-1001.slice/session-9.scope\n",
        )
        reply, context, result = broker.handle_command(
            "WHOAMI", self._ident(), str(root)
        )
        self.assertEqual(result, "ok")
        self.assertIn("role=netadmin", reply)
        self.assertIn("uid=1001", reply)
        self.assertIn("session=user-1001.slice/session-9.scope", reply)
        self.assertIn("comm=bash", reply)
        self.assertEqual(context["role"], "netadmin")

    def test_whoami_escaped_root_keeps_role(self):
        root = _mktemp(self, "cmd-")
        _write_proc(
            root,
            7,
            comm="python3.9\n",
            cgroup="0::/user.slice/user-1001.slice/session-9.scope\n",
        )
        reply, context, result = broker.handle_command(
            "WHOAMI", self._ident(uid=0), str(root)
        )
        self.assertEqual(result, "ok")
        self.assertIn("role=netadmin", reply)
        self.assertIn("uid=0", reply)

    def test_whoami_without_session(self):
        root = _mktemp(self, "cmd-")
        reply, context, result = broker.handle_command(
            "WHOAMI", self._ident(), str(root)
        )
        self.assertEqual(result, "ok")
        self.assertIn("role=netadmin", reply)
        self.assertIn("session=-", reply)
        self.assertIn("comm=?", reply)

    def test_ps(self):
        root = _mktemp(self, "cmd-")
        _write_proc(root, 5, status=_status(0), comm="init\n", cmdline=b"init\x00")
        reply, _, result = broker.handle_command("PS", self._ident(), str(root))
        self.assertEqual(result, "ok")
        self.assertEqual(reply, "5 0 init init")

    def test_ps_netops_ok(self):
        root = _mktemp(self, "cmd-")
        _write_proc(root, 5, status=_status(0), comm="init\n", cmdline=b"init\x00")
        ident = {"uid": 1000, "gid": 1000, "pid": 7}
        reply, _, result = broker.handle_command("PS", ident, str(root))
        self.assertEqual(result, "ok")
        self.assertEqual(reply, "5 0 init init")

    def test_ps_unknown_role_forbidden(self):
        root = _mktemp(self, "cmd-")
        ident = {"uid": 4242, "gid": 4242, "pid": 7}
        reply, _, result = broker.handle_command("PS", ident, str(root))
        self.assertEqual((reply, result), ("ERR forbidden", "err:forbidden"))

    def test_unknown(self):
        root = _mktemp(self, "cmd-")
        reply, _, result = broker.handle_command("BOGUS", self._ident(), str(root))
        self.assertEqual(reply, "ERR unknown-command")
        self.assertEqual(result, "err:unknown-command")


class LineReaderTest(unittest.TestCase):
    def _pair(self):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        return ours, peer

    def test_seeded_lines(self):
        ours, peer = self._pair()
        reader = broker.LineReader(peer, b"/bin/ps\n-e\n")
        self.assertEqual(reader.readline(4096), (b"/bin/ps", None))
        self.assertEqual(reader.readline(4096), (b"-e", None))

    def test_leftover_preserved(self):
        ours, peer = self._pair()
        reader = broker.LineReader(peer, b"a\nbc")
        self.assertEqual(reader.readline(4096), (b"a", None))
        ours.sendall(b"d\n")
        ours.shutdown(socket.SHUT_WR)
        self.assertEqual(reader.readline(4096), (b"bcd", None))

    def test_split_recvs(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"ab", b"cd\n"]
        reader = broker.LineReader(conn)
        self.assertEqual(reader.readline(4096), (b"abcd", None))

    def test_overlong(self):
        ours, peer = self._pair()
        ours.sendall(b"0123456789")
        ours.shutdown(socket.SHUT_WR)
        reader = broker.LineReader(peer)
        self.assertEqual(reader.readline(4), (None, "err:exec-arg-long"))

    def test_eof(self):
        ours, peer = self._pair()
        ours.sendall(b"partial")
        ours.shutdown(socket.SHUT_WR)
        reader = broker.LineReader(peer)
        self.assertEqual(reader.readline(4096), (None, "err:exec-eof"))

    def test_recv_error(self):
        conn = mock.Mock()
        conn.recv.side_effect = OSError("gone")
        reader = broker.LineReader(conn)
        self.assertEqual(reader.readline(4096), (None, "err:exec-eof"))


class SanitizeExecTest(unittest.TestCase):
    def test_target_keeps_safe_token(self):
        self.assertEqual(broker.sanitize_target("/bin/ps"), "/bin/ps")

    def test_target_clamps_and_truncates(self):
        self.assertEqual(broker.sanitize_target("a b\nc;d", limit=4), "a_b_")
        self.assertEqual(broker.sanitize_target("abc", limit=2), "ab")

    def test_detail_flattens_and_truncates(self):
        self.assertEqual(
            broker.sanitize_detail(["/bin/echo", "a\nb", "  c  "]), "/bin/echo a b c"
        )
        self.assertEqual(broker.sanitize_detail(["abcdef"], limit=3), "abc")

    def test_detail_clamps_controls(self):
        self.assertEqual(
            broker.sanitize_detail(["/bin/echo", "\x1b[31mred\x07", "a\x7fb"]),
            "/bin/echo _[31mred_ a_b",
        )


class VerbMatrixTest(unittest.TestCase):
    """The role matrix: reads serve both roles, mutates need escaped netadmin."""

    def test_reads_serve_both_roles_regardless_of_uid(self):
        for role, uid in (
            ("netops", 1000),
            ("netops", 0),
            ("netadmin", 1001),
            ("netadmin", 0),
        ):
            for verb in ("PS", "STATUS"):
                with self.subTest(verb=verb, role=role, uid=uid):
                    self.assertTrue(broker.verb_allowed(verb, role, uid))

    def test_mutates_require_escaped_netadmin(self):
        for verb in ("START", "STOP", "RESTART"):
            with self.subTest(verb=verb, role="netadmin", uid=0):
                self.assertTrue(broker.verb_allowed(verb, "netadmin", 0))
            for role, uid in (
                ("netadmin", 1001),
                ("netops", 1000),
                ("netops", 0),
                ("unknown", 0),
            ):
                with self.subTest(verb=verb, role=role, uid=uid):
                    self.assertFalse(broker.verb_allowed(verb, role, uid))

    def test_unknown_role_gets_nothing(self):
        for verb in ("PS", "STATUS", "START", "STOP", "RESTART"):
            self.assertFalse(broker.verb_allowed(verb, "unknown", 0), verb)

    def test_unknown_verb_denied_even_for_escaped_netadmin(self):
        self.assertFalse(broker.verb_allowed("BOGUS", "netadmin", 0))


class ExecAllowedTest(unittest.TestCase):
    def test_escaped_netadmin_unrestricted(self):
        self.assertTrue(broker.exec_allowed("netadmin", 0, ["/bin/sh"]))
        self.assertTrue(broker.exec_allowed("netadmin", 0, ["anything", "goes"]))

    def test_unescaped_denied(self):
        self.assertFalse(broker.exec_allowed("netadmin", 1001, ["/bin/true"]))

    def test_unknown_denied(self):
        self.assertFalse(broker.exec_allowed("unknown", 0, ["/bin/true"]))

    def test_netops_denied_with_empty_allowlist(self):
        self.assertFalse(broker.exec_allowed("netops", 1000, ["/bin/true"]))

    def test_netops_allowlisted_argv_exact_match(self):
        allow = frozenset({("/bin/true",)})
        with mock.patch.object(broker, "NETOPS_EXEC_ALLOW", allow):
            self.assertTrue(broker.exec_allowed("netops", 1000, ["/bin/true"]))
            self.assertFalse(broker.exec_allowed("netops", 1000, ["/bin/true", "x"]))
            self.assertFalse(broker.exec_allowed("netops", 1000, ["/bin/false"]))

    def test_netops_wrong_uid_denied_even_when_allowlisted(self):
        allow = frozenset({("/bin/true",)})
        with mock.patch.object(broker, "NETOPS_EXEC_ALLOW", allow):
            self.assertFalse(broker.exec_allowed("netops", 0, ["/bin/true"]))


# Must match the fake manifest's escape-key path: this test fails if
# netops EXEC can ever reach the key file, whatever the allowlist holds.
NETADMIN_KEY_PATH = "/var/lib/machines/netadmin/home/netadmin/.ssh/id_ed25519"


class NetopsExecPolicyTest(unittest.TestCase):
    """Key-exfil invariant: no netops EXEC argv reaches shells, ssh,
    sudo, the escape key, or credential files."""

    SENSITIVE = (
        ["/bin/cat", NETADMIN_KEY_PATH],
        ["/bin/cat", "/root/.ssh/authorized_keys"],
        ["/bin/cat", "/etc/passwd"],
        ["/bin/cat", "/etc/shadow"],
        ["/bin/cat", "/etc/sudoers"],
        ["/bin/cat", "/etc/ssh/ssh_host_ed25519_key"],
        ["/usr/bin/ssh", "root@localhost"],
        ["/usr/bin/sudo", "su", "-"],
        ["/bin/sh", "-c", "id"],
        ["/bin/bash"],
        ["/bin/cat", "/etc/passwd;id"],
        ["cat", "/etc/passwd"],
        [""],
    )

    def test_sensitive_argv_denied(self):
        for argv in self.SENSITIVE:
            with self.subTest(argv=argv):
                self.assertFalse(broker.exec_allowed("netops", 1000, argv))

    def test_allowlist_starts_empty(self):
        # Structural fail-closed: any future allowlist entry breaks this
        # test and forces its author to extend this class with the
        # per-entry justification. Data cannot land without its test.
        self.assertEqual(broker.NETOPS_EXEC_ALLOW, frozenset())

    def test_shape_rejects_shell_spelling(self):
        self.assertTrue(broker.valid_exec_entry(("/bin/true",)))
        self.assertTrue(broker.valid_exec_entry(("/bin/cat", "/etc/hosts")))
        for entry in (
            (),
            "cat",
            ("cat",),
            ("",),
            ("/bin/cat", ""),
            ("/bin/cat", None),
            ("/bin/../bin/cat",),
            ("/bin/./cat",),
        ):
            self.assertFalse(broker.valid_exec_entry(entry), entry)


class SpawnSessionTest(unittest.TestCase):
    def test_parent_keeps_own_ends(self):
        with mock.patch.object(os, "fork", return_value=1234):
            pid, stdin_w, stdout_r = broker.spawn_session(["/bin/echo", "hi"])
        self.assertEqual(pid, 1234)
        for fd in (stdin_w, stdout_r):
            os.close(fd)
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_fork_failure_closes_all_pipes(self):
        made = []
        real_pipe = os.pipe

        def record_pipe():
            pair = real_pipe()
            made.extend(pair)
            return pair

        with (
            mock.patch.object(os, "pipe", side_effect=record_pipe),
            mock.patch.object(os, "fork", side_effect=OSError("EAGAIN")),
        ):
            with self.assertRaises(OSError):
                broker.spawn_session(["/bin/echo", "hi"])
        self.assertEqual(len(made), 4)
        for fd in made:
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_second_pipe_failure_closes_first_pipe(self):
        made = []
        real_pipe = os.pipe

        def fail_second():
            if not made:
                pair = real_pipe()
                made.extend(pair)
                return pair
            raise OSError("ENFILE")

        with mock.patch.object(os, "pipe", side_effect=fail_second):
            with self.assertRaises(OSError):
                broker.spawn_session(["/bin/echo", "hi"])
        self.assertEqual(len(made), 2)
        for fd in made:
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_child_dups_and_execs(self):
        argv = ["/bin/echo", "hi"]
        with (
            mock.patch.object(os, "fork", return_value=0),
            mock.patch.object(os, "dup2") as duped,
            mock.patch.object(os, "setsid") as sessioned,
            mock.patch.object(os, "execvp", side_effect=OSError("no")) as execed,
            mock.patch.object(os, "_exit") as gone,
        ):
            with self.assertRaises(OSError):
                broker.spawn_session(argv)
        gone.assert_called_once_with(127)
        execed.assert_called_once_with("/bin/echo", argv)
        sessioned.assert_called_once_with()
        self.assertEqual([call[0][1] for call in duped.call_args_list], [0, 1, 2])

    def test_child_setsid_failure_fails_closed(self):
        argv = ["/bin/echo", "hi"]
        with (
            mock.patch.object(os, "fork", return_value=0),
            mock.patch.object(os, "dup2"),
            mock.patch.object(os, "setsid", side_effect=OSError("nope")),
            mock.patch.object(os, "execvp") as execed,
            mock.patch.object(os, "_exit") as gone,
        ):
            with self.assertRaises(OSError):
                broker.spawn_session(argv)
        execed.assert_not_called()
        gone.assert_called_once_with(127)


class TerminateTest(unittest.TestCase):
    def test_fast_exit(self):
        # Leader already dead: the group is still signaled first (it
        # outlives the leader while grandchildren remain), then the
        # leader is reaped, and the SIGKILL stage still runs so
        # SIGTERM-ignoring stragglers cannot survive the leader.
        with (
            mock.patch.object(os, "killpg") as killed,
            mock.patch.object(os, "waitpid", return_value=(424242, 5)) as waited,
        ):
            self.assertEqual(broker._terminate(424242), 5)
        self.assertEqual(
            killed.call_args_list,
            [
                mock.call(424242, signal.SIGTERM),
                mock.call(424242, signal.SIGKILL),
            ],
        )
        waited.assert_called_once_with(424242, os.WNOHANG)

    def test_dead_leader_gone_group_swallowed(self):
        with (
            mock.patch.object(os, "killpg", side_effect=ProcessLookupError()),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            self.assertEqual(broker._terminate(424242), 0)

    def test_term_then_reap(self):
        with (
            mock.patch.object(os, "killpg") as killed,
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (424242, 7)]),
        ):
            self.assertEqual(broker._terminate(424242), 7)
        self.assertEqual(
            killed.call_args_list,
            [
                mock.call(424242, signal.SIGTERM),
                mock.call(424242, signal.SIGKILL),
            ],
        )

    def test_term_lookup_error_swallowed(self):
        with (
            mock.patch.object(os, "killpg", side_effect=ProcessLookupError()),
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (424242, 0)]),
            mock.patch.object(time, "monotonic", return_value=100.0),
        ):
            self.assertEqual(broker._terminate(424242), 0)

    def test_kill_after_grace(self):
        with (
            mock.patch.object(os, "killpg") as killed,
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (424242, 9)]),
            mock.patch.object(time, "monotonic", side_effect=[100.0, 200.0, 300.0]),
            mock.patch.object(time, "sleep") as slept,
        ):
            self.assertEqual(broker._terminate(424242, grace=2.0), 9)
        # Reaped after grace expiry: TERM, then the scheduled KILL. No
        # third signal: SIGKILL already ran, so the straggler sweep in
        # the grace loop has nothing left to cover.
        self.assertEqual(
            killed.call_args_list,
            [
                mock.call(424242, signal.SIGTERM),
                mock.call(424242, signal.SIGKILL),
            ],
        )
        slept.assert_not_called()

    def test_gone_before_kill(self):
        with (
            mock.patch.object(os, "killpg", side_effect=[None, ProcessLookupError()]),
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (424242, 0)]),
            mock.patch.object(time, "monotonic", side_effect=[100.0, 200.0, 300.0]),
        ):
            self.assertEqual(broker._terminate(424242, grace=2.0), 0)

    def test_grace_sleeps_then_kills(self):
        with (
            mock.patch.object(os, "killpg") as killed,
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (0, 0), (424242, 0)]),
            mock.patch.object(
                time, "monotonic", side_effect=[100.0, 100.5, 200.0, 300.0]
            ),
            mock.patch.object(time, "sleep") as slept,
        ):
            self.assertEqual(broker._terminate(424242, grace=2.0), 0)
        slept.assert_called_once_with(0.01)
        self.assertEqual(
            killed.call_args_list,
            [
                mock.call(424242, signal.SIGTERM),
                mock.call(424242, signal.SIGKILL),
            ],
        )

    def test_kill_eperm_loud_but_proceeds(self):
        stream = io.StringIO()
        with (
            mock.patch.object(os, "killpg", side_effect=PermissionError()),
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (424242, 0)]),
            mock.patch.object(sys, "stderr", stream),
        ):
            self.assertEqual(broker._terminate(424242, 0), 0)
        warned = stream.getvalue()
        self.assertIn("killpg SIGTERM EPERM on group 424242", warned)
        self.assertIn("killpg SIGKILL EPERM on group 424242", warned)
        self.assertIn("subtree may survive", warned)

    def test_reap_polls_then_reaps(self):
        with (
            mock.patch.object(os, "killpg"),
            mock.patch.object(os, "waitpid", side_effect=[(0, 0), (0, 0), (424242, 0)]),
            mock.patch.object(
                time, "monotonic", side_effect=[100.0, 200.0, 300.0, 300.5]
            ),
            mock.patch.object(time, "sleep") as slept,
        ):
            self.assertEqual(broker._terminate(424242, grace=2.0), 0)
        slept.assert_called_once_with(0.01)

    def test_sigkill_survivor_abandoned_loud(self):
        stream = io.StringIO()
        with (
            mock.patch.object(os, "killpg"),
            mock.patch.object(os, "waitpid", return_value=(0, 0)),
            mock.patch.object(sys, "stderr", stream),
            mock.patch.object(broker.time, "monotonic", return_value=100.0),
        ):
            self.assertEqual(broker._terminate(424242, 0), -1)
        self.assertIn("leader 424242 unreaped after SIGKILL", stream.getvalue())

    def test_live_straggler_killed(self):
        # SIG_DFL while the child lives: earlier tests may leave the
        # broker's SIGCHLD reaper installed, which would steal status.
        previous = signal.getsignal(signal.SIGCHLD)
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        try:
            self._straggler_live()
        finally:
            signal.signal(signal.SIGCHLD, previous)

    def _straggler_live(self):
        # Bite proof: a grandchild ignoring SIGTERM must not survive
        # its leader. The leader exits at once (reaped inside the grace
        # loop); without the straggler SIGKILL the grandchild lives on.
        pipe_r, pipe_w = os.pipe()
        self.addCleanup(os.close, pipe_r)
        leader = os.fork()
        if leader == 0:
            try:
                os.close(pipe_r)
                os.setsid()
                grand = os.fork()
                if grand == 0:
                    try:
                        signal.signal(signal.SIGTERM, signal.SIG_IGN)
                        os.write(pipe_w, str(os.getpid()).encode())
                        time.sleep(30)
                    finally:
                        os._exit(0)
            finally:
                os._exit(0)
        os.close(pipe_w)
        grandchild = int(os.read(pipe_r, 32))
        try:
            broker._terminate(leader, grace=5.0)
            deadline = time.monotonic() + 5.0
            while True:
                try:
                    os.kill(grandchild, 0)
                except ProcessLookupError:
                    break
                if time.monotonic() >= deadline:
                    self.fail("SIGTERM-ignoring grandchild survived _terminate")
                time.sleep(0.05)
        finally:
            try:
                os.kill(grandchild, signal.SIGKILL)
            except ProcessLookupError:
                pass


class ReapExitTest(unittest.TestCase):
    def test_fast_reap(self):
        with (
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
            mock.patch.object(broker, "_terminate") as term,
        ):
            self.assertEqual(broker._reap_session_exit(424242), 0)
        term.assert_not_called()

    def test_lingering_leader_terminated(self):
        with (
            mock.patch.object(os, "waitpid", return_value=(0, 0)),
            mock.patch.object(time, "monotonic", side_effect=[100.0, 200.0]),
            mock.patch.object(broker, "_terminate", return_value=0) as term,
        ):
            self.assertEqual(broker._reap_session_exit(424242, grace=2.0), 0)
        term.assert_called_once_with(424242)

    def test_abandoned_leader_returns_none_loud(self):
        stream = io.StringIO()
        with (
            mock.patch.object(os, "waitpid", return_value=(0, 0)),
            mock.patch.object(time, "monotonic", side_effect=[100.0, 200.0]),
            mock.patch.object(broker, "_terminate", return_value=-1),
            mock.patch.object(sys, "stderr", stream),
        ):
            self.assertIsNone(broker._reap_session_exit(424242, grace=2.0))
        self.assertIn("session 424242 alive after kill", stream.getvalue())

    def test_lingering_leader_reaped_once_live(self):
        # No mocks: a real lingering leader must come back through
        # _terminate's status, not a second waitpid (which raises
        # ChildProcessError on the already-reaped child). SIG_DFL while
        # the child lives: earlier tests may leave the broker's
        # SIGCHLD reaper installed, which would steal its status.
        previous = signal.getsignal(signal.SIGCHLD)
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        try:
            self._reap_live_leader()
        finally:
            signal.signal(signal.SIGCHLD, previous)

    def _reap_live_leader(self):
        pid = os.fork()
        if pid == 0:
            try:
                os.setsid()
                os.close(1)
                time.sleep(30)
            finally:
                os._exit(0)
        try:
            time.sleep(0.1)
            self.assertEqual(broker._reap_session_exit(pid, grace=0.1), -signal.SIGTERM)
        finally:
            try:
                gone, _status = os.waitpid(pid, os.WNOHANG)
            except ChildProcessError:
                gone = pid
            if gone == 0:
                os.kill(pid, signal.SIGKILL)
                os.waitpid(pid, 0)
                self.fail("_reap_session_exit left the leader unreaped")


class CloseQuietlyTest(unittest.TestCase):
    def test_closes(self):
        fd = os.open("/dev/null", os.O_RDONLY)
        broker._close_quietly(fd)
        with self.assertRaises(OSError):
            os.fstat(fd)

    def test_bogus_is_silent(self):
        broker._close_quietly(-1)


class ParseInFramesTest(unittest.TestCase):
    def test_empty_and_partial_wait(self):
        self.assertEqual(broker.parse_in_frames(b""), ([], b"", None))
        self.assertEqual(broker.parse_in_frames(b"IN 3"), ([], b"IN 3", None))

    def test_overlong_header_without_newline(self):
        events, rest, error = broker.parse_in_frames(b"x" * 129)
        self.assertEqual((events, rest, error), ([], b"", "err:bad-stdin"))

    def test_overlong_line_with_newline(self):
        events, rest, error = broker.parse_in_frames(b"x" * 129 + b"\n")
        self.assertEqual((events, rest, error), ([], b"", "err:bad-stdin"))

    def test_in_eof_keeps_trailing_partial(self):
        self.assertEqual(
            broker.parse_in_frames(b"IN_EOF\nIN"),
            ([(b"IN_EOF", b"")], b"IN", None),
        )

    def test_in_chunk_and_empty_chunk(self):
        self.assertEqual(
            broker.parse_in_frames(b"IN 3\nabc"),
            ([(b"IN", b"abc")], b"", None),
        )
        self.assertEqual(
            broker.parse_in_frames(b"IN 0\n"),
            ([(b"IN", b"")], b"", None),
        )

    def test_split_payload_waits(self):
        self.assertEqual(broker.parse_in_frames(b"IN 5\nab"), ([], b"IN 5\nab", None))

    def test_bad_headers_rejected(self):
        for raw in (b"BOGUS 1\n", b"IN\n", b"IN x\n", b"IN -1\n", b"IN 1 2\n"):
            events, rest, error = broker.parse_in_frames(raw)
            self.assertEqual((events, rest, error), ([], b"", "err:bad-stdin"), raw)

    def test_oversize_rejected(self):
        events, rest, error = broker.parse_in_frames(b"IN 2000000\n")
        self.assertEqual((events, rest, error), ([], b"", "err:bad-stdin"))

    def test_multiple_frames_in_order(self):
        self.assertEqual(
            broker.parse_in_frames(b"IN 1\naIN_EOF\nIN 2\nbc"),
            (
                [(b"IN", b"a"), (b"IN_EOF", b"")],
                b"IN 2\nbc",
                None,
            ),
        )

    def test_parse_stops_at_eof_leaving_tail(self):
        # Bytes after IN_EOF are never frames: the parser leaves them
        # unconsumed (no error) so the caller drains them.
        self.assertEqual(
            broker.parse_in_frames(b"IN_EOF\nGARBAGE\n"),
            ([(b"IN_EOF", b"")], b"GARBAGE\n", None),
        )


class ProxyTest(unittest.TestCase):
    def _pair(self):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        return ours, peer

    def test_forward_stdin_frame_stdout_exit(self):
        ours, peer = self._pair()
        ours.sendall(b"IN 2\nhiIN_EOF\n")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.write(stdout_w, b"hi\n")
        os.close(stdout_w)
        with mock.patch.object(os, "waitpid", return_value=(424242, 0)):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"DATA 3\nhi\nEXIT 0\n")
        self.assertEqual(os.read(stdin_r, 2), b"hi")
        self.assertEqual(os.read(stdin_r, 1), b"")

    def test_signaled_exit_code(self):
        ours, peer = self._pair()
        ours.sendall(b"IN_EOF\n")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with mock.patch.object(os, "waitpid", return_value=(424242, 9)):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, -9)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 137\n")

    def test_stdout_eof_with_stdin_open(self):
        ours, peer = self._pair()
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select, "select", return_value=([stdout_r], [], [])
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")
        with self.assertRaises(OSError):
            os.fstat(stdin_w)

    def test_recv_error_aborts(self):
        conn = mock.Mock()
        conn.recv.side_effect = OSError("rst")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(broker.select, "select", return_value=([conn], [], [])),
            mock.patch.object(broker, "_terminate") as term,
        ):
            self.assertIsNone(broker.proxy_session(conn, 424242, stdin_w, stdout_r))
        term.assert_called_once_with(424242)
        for fd in (stdin_w, stdout_r):
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_send_error_aborts(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"IN_EOF\n"]
        conn.sendall.side_effect = OSError("gone")
        stdin_r, stdin_w = os.pipe()
        os.close(stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.write(stdout_w, b"zz")
        self.addCleanup(os.close, stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[([conn], [], []), ([stdout_r], [], [])],
            ),
            mock.patch.object(broker, "_terminate") as term,
        ):
            self.assertIsNone(broker.proxy_session(conn, 424242, stdin_w, stdout_r))
        term.assert_called_once_with(424242)
        for fd in (stdin_w, stdout_r):
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_broken_stdin_pipe_continues(self):
        ours, peer = self._pair()
        # No client bytes: with stdin broken from the start the proxy
        # never reads the client (undeliverable input is dropped, and
        # unread bytes would RST the close).
        ours.shutdown(socket.SHUT_WR)
        stdin_r, stdin_w = os.pipe()
        os.close(stdin_r)
        os.close(stdin_w)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select, "select", return_value=([stdout_r], [], [])
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")

    def test_set_blocking_failure_closes_stdin(self):
        ours, peer = self._pair()
        ours.shutdown(socket.SHUT_WR)
        stdin_r, stdin_w = os.pipe()
        os.close(stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(os, "set_blocking", side_effect=OSError("bad")),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
            mock.patch.object(
                broker.select, "select", return_value=([stdout_r], [], [])
            ),
        ):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")
        with self.assertRaises(OSError):
            os.fstat(stdin_w)

    def test_stdout_read_error_aborts_loud(self):
        ours, peer = self._pair()
        ours.shutdown(socket.SHUT_WR)
        stdin_r, stdin_w = os.pipe()
        os.close(stdin_r)
        stdout_r, stdout_w = os.pipe()
        self.addCleanup(os.close, stdout_w)
        with (
            mock.patch.object(
                broker.select, "select", return_value=([stdout_r], [], [])
            ),
            mock.patch.object(os, "read", side_effect=OSError("bad")),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            result = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(result, "err:stdout-read")
        peer.close()
        self.assertEqual(_read_all(ours), b"ERR stdout-read")
        with self.assertRaises(OSError):
            os.fstat(stdin_w)
        with self.assertRaises(OSError):
            os.fstat(stdout_r)

    def test_initial_frames_flushed_first(self):
        ours, peer = self._pair()
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with mock.patch.object(os, "waitpid", return_value=(424242, 0)):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r, b"IN 4\nseed")
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")
        self.assertEqual(os.read(stdin_r, 4), b"seed")

    def test_initial_broken_pipe_continues(self):
        ours, peer = self._pair()
        stdin_r, stdin_w = os.pipe()
        os.close(stdin_r)
        os.close(stdin_w)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with mock.patch.object(os, "waitpid", return_value=(424242, 0)):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r, b"IN 4\nseed")
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")

    def test_close_mid_session_aborts(self):
        ours, peer = self._pair()
        ours.sendall(b"IN 2\nhi")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        self.addCleanup(os.close, stdout_w)
        with mock.patch.object(broker, "_terminate") as term:
            ours.close()
            self.assertIsNone(broker.proxy_session(peer, 424242, stdin_w, stdout_r))
        term.assert_called_once_with(424242)
        for fd in (stdin_w, stdout_r):
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_close_after_in_eof_aborts(self):
        ours, peer = self._pair()
        ours.sendall(b"IN_EOF\n")
        ours.close()
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        self.addCleanup(os.close, stdout_w)
        with mock.patch.object(broker, "_terminate") as term:
            self.assertIsNone(broker.proxy_session(peer, 424242, stdin_w, stdout_r))
        term.assert_called_once_with(424242)
        for fd in (stdin_w, stdout_r):
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_malformed_frame_aborts(self):
        ours, peer = self._pair()
        ours.sendall(b"BOGUS\n")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        self.addCleanup(os.close, stdout_w)
        with mock.patch.object(broker, "_terminate") as term:
            result = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(result, "err:bad-stdin")
        term.assert_called_once_with(424242)
        for fd in (stdin_w, stdout_r):
            with self.assertRaises(OSError):
                os.fstat(fd)

    def test_split_frame_across_recvs(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"IN 5\nab", b"cdeIN_EOF\n"]
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.write(stdout_w, b"done\n")
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[
                    ([conn], [], []),
                    ([conn], [], []),
                    ([stdout_r], [stdin_w], []),
                    ([stdout_r], [], []),
                ],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(conn, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        self.assertEqual(os.read(stdin_r, 5), b"abcde")
        conn.sendall.assert_any_call(b"DATA 5\ndone\n")
        conn.sendall.assert_called_with(b"EXIT 0\n")

    def test_late_frame_after_eof_dropped(self):
        ours, peer = self._pair()
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with mock.patch.object(os, "waitpid", return_value=(424242, 0)):
            code = broker.proxy_session(
                peer, 424242, stdin_w, stdout_r, b"IN_EOF\nIN 1\nx"
            )
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")
        self.assertEqual(os.read(stdin_r, 1), b"")

    def test_malformed_tail_after_eof_discarded(self):
        ours, peer = self._pair()
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[([], [], []), ([stdout_r], [], [])],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(
                peer, 424242, stdin_w, stdout_r, b"IN_EOF\nGARBAGE\n"
            )
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")

    def test_malformed_recv_after_eof_discarded(self):
        ours, peer = self._pair()
        ours.sendall(b"GARBAGE-NO-FRAME\n")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[
                    ([peer], [], []),
                    ([], [], []),
                    ([stdout_r], [], []),
                ],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r, b"IN_EOF\n")
        self.assertEqual(code, 0)
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")

    def test_stdin_write_wouldblock_retries(self):
        ours, peer = self._pair()
        ours.sendall(b"IN 2\nhiIN_EOF\n")
        ours.shutdown(socket.SHUT_WR)
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        real_write = os.write
        calls = []

        def fake_write(fd, data):
            calls.append(bytes(data))
            if len(calls) == 1:
                raise BlockingIOError()
            return real_write(fd, data)

        with (
            mock.patch.object(os, "write", side_effect=fake_write),
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[
                    ([peer], [], []),
                    ([], [stdin_w], []),
                    ([], [stdin_w], []),
                    ([stdout_r], [], []),
                ],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        self.assertEqual(calls, [b"hi", b"hi"])
        peer.close()
        self.assertEqual(_read_all(ours), b"EXIT 0\n")
        self.assertEqual(os.read(stdin_r, 2), b"hi")

    def test_stdin_write_error_closes_stdin(self):
        ours, peer = self._pair()
        ours.sendall(b"IN 2\nhiIN_EOF\n")
        ours.shutdown(socket.SHUT_WR)
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)

        def fake_write(fd, data):
            raise BrokenPipeError()

        with (
            mock.patch.object(os, "write", side_effect=fake_write),
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[
                    ([peer], [], []),
                    ([], [stdin_w], []),
                    ([stdout_r], [], []),
                ],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        with self.assertRaises(OSError):
            os.fstat(stdin_w)

    def test_in_eof_flushes_pending_before_close(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"IN 2\nhi", b"IN_EOF\n"]
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[
                    ([conn], [], []),
                    ([conn], [], []),
                    ([], [stdin_w], []),
                    ([stdout_r], [], []),
                ],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(conn, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        self.assertEqual(os.read(stdin_r, 2), b"hi")
        with self.assertRaises(OSError):
            os.fstat(stdin_w)

    def test_post_eof_garbage_drained_while_flushing(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"IN 2\nhi", b"IN_EOF\n", b"GARBAGE\n"]
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[
                    ([conn], [], []),
                    ([conn], [], []),
                    ([], [], []),
                    ([conn], [], []),
                    ([], [stdin_w], []),
                    ([stdout_r], [], []),
                ],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(conn, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        self.assertEqual(os.read(stdin_r, 2), b"hi")
        with self.assertRaises(OSError):
            os.fstat(stdin_w)

    def test_disconnect_during_drain_aborts(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"IN 2\nhi", b"IN_EOF\n", b""]
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        self.addCleanup(os.close, stdout_w)
        seen = []
        scripted = [([conn], [], []), ([conn], [], []), ([conn], [], [])]

        def fake_select(readers, writers, _x):
            seen.append((list(readers), list(writers)))
            return scripted[len(seen) - 1]

        with (
            mock.patch.object(broker.select, "select", side_effect=fake_select),
            mock.patch.object(os, "write", side_effect=BlockingIOError()),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
            mock.patch.object(broker, "_terminate") as term,
        ):
            result = broker.proxy_session(conn, 424242, stdin_w, stdout_r)
        self.assertIsNone(result)
        term.assert_called_once_with(424242)
        # The flush is stuck (writes block) yet the connection stays
        # watched: disconnect aborts instead of hanging on the shell.
        self.assertEqual(seen[2][0], [stdout_r, conn])
        with self.assertRaises(OSError):
            os.fstat(stdin_w)
        with self.assertRaises(OSError):
            os.fstat(stdout_r)

    def test_pending_cap_pauses_client_reads(self):
        conn = mock.Mock()
        conn.recv.return_value = b"IN 2\nxx"
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        seen = []

        def fake_select(readers, writers, _x):
            seen.append((list(readers), list(writers)))
            if len(seen) == 1:
                return ([conn], [], [])
            if len(seen) == 2:
                return ([], [stdin_w], [])
            return ([stdout_r], [], [])

        with (
            mock.patch.object(broker, "STDIN_PENDING_CAP", 2),
            mock.patch.object(broker.select, "select", side_effect=fake_select),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(conn, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)
        self.assertEqual(seen[0][0], [stdout_r, conn])
        self.assertEqual(seen[1][0], [stdout_r])
        self.assertEqual(seen[1][1], [stdin_w])

    def test_eof_unkillable_reports_reap_failed(self):
        ours, peer = self._pair()
        ours.sendall(b"IN_EOF\n")
        ours.shutdown(socket.SHUT_WR)
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with mock.patch.object(broker, "_reap_session_exit", return_value=None):
            result = broker.proxy_session(peer, 424242, stdin_w, stdout_r)
        self.assertEqual(result, "err:reap-failed")
        peer.close()
        self.assertEqual(_read_all(ours), b"ERR reap-failed")

    def test_exit_send_error_still_returns_code(self):
        conn = mock.Mock()
        conn.sendall.side_effect = [None, OSError("gone")]
        stdin_r, stdin_w = os.pipe()
        os.close(stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.write(stdout_w, b"zz")
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker.select,
                "select",
                side_effect=[([stdout_r], [], []), ([stdout_r], [], [])],
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            code = broker.proxy_session(conn, 424242, stdin_w, stdout_r)
        self.assertEqual(code, 0)


class ExitReportTest(unittest.TestCase):
    def test_signal_maps_to_shell_convention(self):
        self.assertEqual(broker.exit_report(-9), (137, "sig-9"))
        self.assertEqual(broker.exit_report(-15), (143, "sig-15"))
        self.assertEqual(broker.exit_report(-1), (129, "sig-1"))

    def test_plain_codes_unchanged(self):
        self.assertEqual(broker.exit_report(0), (0, "exit-0"))
        self.assertEqual(broker.exit_report(1), (1, "exit-1"))
        # A real 137 exit keeps the exit- tag: the audit tag is what
        # disambiguates it from a SIGKILL (sig-9).
        self.assertEqual(broker.exit_report(137), (137, "exit-137"))


class ServeExecTest(unittest.TestCase):
    def _pair(self):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        return ours, peer

    def _ident(self):
        return {"uid": 0, "gid": 0, "pid": 424242}

    def test_bad_counts_rejected(self):
        for count in (None, "abc", "0", "33", "-2"):
            tmp = _mktemp(self, "exec-")
            audit = str(tmp / "audit.log")
            ours, peer = self._pair()
            result = broker._serve_exec(
                peer, audit, self._ident(), "netadmin", "s", count, b""
            )
            peer.close()
            self.assertEqual(result, "err:bad-exec")
            self.assertEqual(_read_all(ours), b"ERR bad-exec")
            with open(audit) as handle:
                self.assertIn("result=err:bad-exec", handle.read())

    def test_forbidden_after_reading_argv(self):
        for role, uid in (("netops", 1000), ("netadmin", 1001), ("unknown", 0)):
            tmp = _mktemp(self, "exec-")
            audit = str(tmp / "audit.log")
            ours, peer = self._pair()
            ours.sendall(b"/bin/echo\nhi\n")
            ours.shutdown(socket.SHUT_WR)
            ident = {"uid": uid, "gid": 0, "pid": 1}
            result = broker._serve_exec(peer, audit, ident, role, "s", "2", b"")
            peer.close()
            self.assertEqual(result, "err:forbidden")
            self.assertEqual(_read_all(ours), b"ERR forbidden")
            with open(audit) as handle:
                record = handle.read()
            self.assertIn("target=- result=err:forbidden", record)
            self.assertNotIn("/bin/echo", record)

    def test_netops_key_path_forbidden_over_wire(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"/bin/cat\n" + NETADMIN_KEY_PATH.encode() + b"\n")
        ours.shutdown(socket.SHUT_WR)
        ident = {"uid": 1000, "gid": 0, "pid": 1}
        result = broker._serve_exec(peer, audit, ident, "netops", "s", "2", b"")
        peer.close()
        self.assertEqual(result, "err:forbidden")
        self.assertEqual(_read_all(ours), b"ERR forbidden")
        with open(audit) as handle:
            record = handle.read()
        self.assertIn("role=netops", record)
        self.assertIn("result=err:forbidden", record)
        self.assertNotIn("id_ed25519", record)

    def test_arg_long(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"x" * 5000)
        ours.shutdown(socket.SHUT_WR)
        result = broker._serve_exec(
            peer, audit, self._ident(), "netadmin", "s", "1", b""
        )
        # Drain the unread overlong bytes as serve_once's discard would,
        # so close() is a clean FIN instead of RST.
        peer.recv(65536)
        peer.close()
        self.assertEqual(result, "err:exec-arg-long")
        self.assertEqual(_read_all(ours), b"ERR exec-arg-long")

    def test_arg_long_send_error_swallowed(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        conn = mock.Mock()
        conn.recv.return_value = b"x" * 5000
        conn.sendall.side_effect = OSError("gone")
        result = broker._serve_exec(
            conn, audit, self._ident(), "netadmin", "s", "1", b""
        )
        self.assertEqual(result, "err:exec-arg-long")

    def test_forbidden_send_error_swallowed(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        conn = mock.Mock()
        conn.recv.side_effect = [b"/bin/echo\nhi\n", b""]
        conn.sendall.side_effect = OSError("gone")
        ident = {"uid": 1000, "gid": 0, "pid": 1}
        result = broker._serve_exec(conn, audit, ident, "netops", "s", "2", b"")
        self.assertEqual(result, "err:forbidden")

    def test_spawn_failure_audited_twice(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        with mock.patch.object(broker, "spawn_session", side_effect=OSError("EAGAIN")):
            result = broker._serve_exec(
                peer, audit, self._ident(), "netadmin", "s", "1", b"/bin/true\n"
            )
        peer.close()
        self.assertEqual(result, "err:spawn-failed")
        self.assertEqual(_read_all(ours), b"ERR spawn-failed")
        with open(audit) as handle:
            records = handle.read().splitlines()
        self.assertEqual(len(records), 2)
        self.assertIn("result=started", records[0])
        self.assertIn("result=err:spawn-failed", records[1])

    def test_eof_mid_argv(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"/bin/echo\n")
        ours.shutdown(socket.SHUT_WR)
        result = broker._serve_exec(
            peer, audit, self._ident(), "netadmin", "s", "2", b""
        )
        peer.close()
        self.assertEqual(result, "err:exec-eof")
        self.assertEqual(_read_all(ours), b"")
        with open(audit) as handle:
            self.assertIn("result=err:exec-eof", handle.read())

    def test_bad_encoding(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"\xff\xfe\n")
        ours.shutdown(socket.SHUT_WR)
        result = broker._serve_exec(
            peer, audit, self._ident(), "netadmin", "s", "1", b""
        )
        peer.close()
        self.assertEqual(result, "err:bad-encoding")
        self.assertEqual(_read_all(ours), b"ERR bad-encoding")

    def test_empty_argv0(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"\n")
        ours.shutdown(socket.SHUT_WR)
        result = broker._serve_exec(
            peer, audit, self._ident(), "netadmin", "s", "1", b""
        )
        peer.close()
        self.assertEqual(result, "err:bad-exec")
        self.assertEqual(_read_all(ours), b"ERR bad-exec")

    def test_session_ok(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"IN_EOF\n")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.write(stdout_w, b"hi\n")
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker,
                "spawn_session",
                return_value=(424242, stdin_w, stdout_r),
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            result = broker._serve_exec(
                peer,
                audit,
                self._ident(),
                "netadmin",
                "sess",
                "2",
                b"/bin/echo\nhi\n",
            )
        peer.close()
        self.assertEqual(result, "exit-0")
        self.assertEqual(_read_all(ours), b"DATA 3\nhi\nEXIT 0\n")
        with open(audit) as handle:
            records = handle.read().splitlines()
        self.assertEqual(len(records), 2)
        self.assertIn("result=started", records[0])
        self.assertIn("target=/bin/echo", records[0])
        self.assertIn("detail=/bin/echo hi", records[0])
        self.assertIn("result=exit-0", records[1])

    def test_session_signaled_audits_sig(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        ours.sendall(b"IN_EOF\n")
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker,
                "spawn_session",
                return_value=(424242, stdin_w, stdout_r),
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 9)),
        ):
            result = broker._serve_exec(
                peer,
                audit,
                self._ident(),
                "netadmin",
                "sess",
                "1",
                b"/bin/sleep\n",
            )
        peer.close()
        self.assertEqual(result, "sig-9")
        self.assertEqual(_read_all(ours), b"EXIT 137\n")
        with open(audit) as handle:
            records = handle.read().splitlines()
        self.assertEqual(len(records), 2)
        self.assertIn("result=sig-9", records[1])

    def test_pipelined_stdin_forwarded(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        stdin_r, stdin_w = os.pipe()
        self.addCleanup(os.close, stdin_r)
        stdout_r, stdout_w = os.pipe()
        os.write(stdout_w, b"piped\n")
        os.close(stdout_w)
        with (
            mock.patch.object(
                broker,
                "spawn_session",
                return_value=(424242, stdin_w, stdout_r),
            ),
            mock.patch.object(os, "waitpid", return_value=(424242, 0)),
        ):
            result = broker._serve_exec(
                peer,
                audit,
                self._ident(),
                "netadmin",
                "sess",
                "1",
                b"/bin/cat\nIN 6\npiped\nIN_EOF\n",
            )
        peer.close()
        self.assertEqual(result, "exit-0")
        self.assertEqual(_read_all(ours), b"DATA 6\npiped\nEXIT 0\n")
        self.assertEqual(os.read(stdin_r, 6), b"piped\n")

    def test_bad_stdin_audited(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        with (
            mock.patch.object(broker, "spawn_session", return_value=(424242, 9, 10)),
            mock.patch.object(broker, "proxy_session", return_value="err:bad-stdin"),
        ):
            result = broker._serve_exec(
                peer, audit, self._ident(), "netadmin", "sess", "1", b"/bin/cat\n"
            )
        self.assertEqual(result, "err:bad-stdin")
        with open(audit) as handle:
            records = handle.read().splitlines()
        self.assertEqual(len(records), 2)
        self.assertIn("result=err:bad-stdin", records[1])

    def test_client_gone(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        with (
            mock.patch.object(broker, "spawn_session", return_value=(424242, 9, 10)),
            mock.patch.object(broker, "proxy_session", return_value=None),
        ):
            result = broker._serve_exec(
                peer, audit, self._ident(), "netadmin", "sess", "1", b"/bin/sleep\n"
            )
        self.assertEqual(result, "err:client-gone")
        with open(audit) as handle:
            records = handle.read().splitlines()
        self.assertEqual(len(records), 2)
        self.assertIn("result=err:client-gone", records[1])

    def test_reap_failed(self):
        tmp = _mktemp(self, "exec-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair()
        with (
            mock.patch.object(broker, "spawn_session", return_value=(424242, 9, 10)),
            mock.patch.object(broker, "proxy_session", return_value="err:reap-failed"),
        ):
            result = broker._serve_exec(
                peer, audit, self._ident(), "netadmin", "sess", "1", b"/bin/sleep\n"
            )
        self.assertEqual(result, "err:reap-failed")
        with open(audit) as handle:
            records = handle.read().splitlines()
        self.assertEqual(len(records), 2)
        self.assertIn("result=err:reap-failed", records[1])


class ServeOnceTest(unittest.TestCase):
    def _pair(self, request):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        ours.sendall(request)
        ours.shutdown(socket.SHUT_WR)
        return ours, peer

    def _fixture_for_self(self, root):
        _write_proc(
            root,
            os.getpid(),
            comm="test\n",
            cgroup="0::/user.slice/user-1001.slice/session-11.scope\n",
        )

    def _reply(self, ours, peer):
        peer.close()
        return _read_all(ours)

    def test_whoami_roundtrip(self):
        tmp = _mktemp(self, "serve-")
        self._fixture_for_self(tmp)
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"WHOAMI\n")
        result = broker.serve_once(peer, audit, str(tmp))
        reply = self._reply(ours, peer).decode()
        self.assertEqual(result, "ok")
        self.assertIn("role=netadmin", reply)
        self.assertIn("uid=%d" % os.getuid(), reply)
        with open(audit) as handle:
            record = handle.read()
        self.assertIn("verb=WHOAMI", record)
        self.assertIn("result=ok", record)

    def test_ps_roundtrip(self):
        tmp = _mktemp(self, "serve-")
        self._fixture_for_self(tmp)
        _write_proc(tmp, 6, status=_status(0), comm="k\n", cmdline=b"k\x00")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"PS\n")
        result = broker.serve_once(peer, audit, str(tmp))
        reply = self._reply(ours, peer).decode()
        self.assertEqual(result, "ok")
        self.assertIn("6 0 k k", reply.splitlines())

    def test_too_long(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"x" * (broker.MAX_REQUEST + 10))
        result = broker.serve_once(peer, audit)
        reply = self._reply(ours, peer)
        self.assertEqual(result, "err:too-long")
        self.assertEqual(reply, b"ERR too-long")
        with open(audit) as handle:
            self.assertIn("result=err:too-long", handle.read())

    def test_bad_encoding(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"\xff\xfe")
        result = broker.serve_once(peer, audit)
        reply = self._reply(ours, peer)
        self.assertEqual(result, "err:bad-encoding")
        self.assertEqual(reply, b"ERR bad-encoding")

    def test_empty(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"  \n ")
        result = broker.serve_once(peer, audit)
        reply = self._reply(ours, peer)
        self.assertEqual(result, "err:empty")
        self.assertEqual(reply, b"ERR empty")

    def test_eof_without_newline_is_a_command(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"WHO")
        result = broker.serve_once(peer, audit)
        reply = self._reply(ours, peer)
        self.assertEqual(result, "err:unknown-command")
        self.assertEqual(reply, b"ERR unknown-command")

    def test_trailing_line_ignored(self):
        tmp = _mktemp(self, "serve-")
        self._fixture_for_self(tmp)
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"WHOAMI\nEXTRA")
        result = broker.serve_once(peer, audit, str(tmp))
        reply = self._reply(ours, peer).decode()
        self.assertEqual(result, "ok")
        self.assertIn("role=netadmin", reply)

    def test_recv_error_audited_without_reply(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        conn = mock.Mock()
        conn.getsockopt.return_value = struct.pack("3i", 1, 1001, 1002)
        conn.recv.side_effect = OSError("gone")
        result = broker.serve_once(conn, audit)
        self.assertEqual(result, "err:recv")
        conn.sendall.assert_not_called()
        with open(audit) as handle:
            record = handle.read()
        self.assertIn("result=err:recv", record)
        self.assertIn("role=netadmin", record)

    def test_recv_error_uses_cgroup_role_for_escaped_peer(self):
        tmp = _mktemp(self, "serve-")
        _write_proc(
            tmp,
            7,
            cgroup="0::/user.slice/user-1001.slice/session-11.scope\n",
        )
        audit = str(tmp / "audit.log")
        conn = mock.Mock()
        conn.getsockopt.return_value = struct.pack("3i", 7, 0, 0)
        conn.recv.side_effect = OSError("gone")
        result = broker.serve_once(conn, audit, str(tmp))
        self.assertEqual(result, "err:recv")
        with open(audit) as handle:
            record = handle.read()
        self.assertIn("result=err:recv", record)
        self.assertIn("role=netadmin", record)

    def test_exec_dispatched(self):
        tmp = _mktemp(self, "serve-")
        _write_proc(
            tmp,
            424242,
            comm="sudo\n",
            cgroup="0::/user.slice/user-1001.slice/session-11.scope\n",
        )
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"EXEC 1\n/bin/true\n")
        ident = {"uid": 0, "gid": 0, "pid": 424242}
        with (
            mock.patch.object(broker, "peer_identity", return_value=ident),
            mock.patch.object(broker, "_serve_exec", return_value="exit-0") as execed,
        ):
            result = broker.serve_once(peer, audit, str(tmp))
        self.assertEqual(result, "exit-0")
        execed.assert_called_once_with(
            peer,
            audit,
            ident,
            "netadmin",
            "user-1001.slice/session-11.scope",
            "1",
            b"/bin/true\n",
        )

    def test_exec_bare_dispatched(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        ours, peer = self._pair(b"EXEC\n")
        with mock.patch.object(broker, "_serve_exec", return_value="err:bad-exec"):
            result = broker.serve_once(peer, audit, str(tmp))
        self.assertEqual(result, "err:bad-exec")


class ReapTest(unittest.TestCase):
    def test_reaps_until_none(self):
        with mock.patch.object(
            os, "waitpid", side_effect=[(1234, 0), (0, 0)]
        ) as waited:
            self.assertIsNone(broker._reap_children(None, None))
        self.assertEqual(waited.call_count, 2)
        waited.assert_called_with(-1, os.WNOHANG)

    def test_no_children_returns(self):
        with mock.patch.object(
            os, "waitpid", side_effect=ChildProcessError()
        ) as waited:
            self.assertIsNone(broker._reap_children(None, None))
        self.assertEqual(waited.call_count, 1)


class MaybeForkTest(unittest.TestCase):
    def test_inline_never_forks(self):
        with mock.patch.object(os, "fork") as forked:
            self.assertFalse(broker._maybe_fork(False))
        forked.assert_not_called()

    def test_parent_and_child(self):
        with mock.patch.object(os, "fork", return_value=1234):
            self.assertFalse(broker._maybe_fork(True))
        with mock.patch.object(os, "fork", return_value=0):
            self.assertTrue(broker._maybe_fork(True))


class ServeSpawnTest(unittest.TestCase):
    def test_parent_closes_and_counts(self):
        tmp = _mktemp(self, "spawn-")
        audit = str(tmp / "audit.log")
        conn = mock.Mock()
        srv = mock.Mock()
        srv.accept.return_value = (conn, None)
        with (
            mock.patch.object(os, "fork", return_value=1234),
            mock.patch.object(signal, "signal"),
        ):
            broker.serve(srv, audit, max_requests=1, spawn=True)
        conn.close.assert_called_once_with()
        # The parent serves nothing itself: no audit record appears.
        self.assertFalse(os.path.exists(audit))
        # The parent keeps accepting: its listener stays open.
        srv.close.assert_not_called()

    def test_fork_failure_sheds_connection_and_continues(self):
        tmp = _mktemp(self, "spawn-")
        audit = str(tmp / "audit.log")
        dropped = mock.Mock()
        kept = mock.Mock()
        srv = mock.Mock()
        srv.accept.side_effect = [(dropped, None), (kept, None)]
        with (
            mock.patch.object(os, "fork", side_effect=[OSError("EAGAIN"), 1234]),
            mock.patch.object(signal, "signal"),
        ):
            broker.serve(srv, audit, max_requests=2, spawn=True)
        dropped.close.assert_called_once_with()
        kept.close.assert_called_once_with()
        self.assertEqual(srv.accept.call_count, 2)
        self.assertFalse(os.path.exists(audit))

    def test_fork_failure_counts_against_bound(self):
        tmp = _mktemp(self, "spawn-")
        audit = str(tmp / "audit.log")
        dropped = mock.Mock()
        srv = mock.Mock()
        srv.accept.side_effect = [(dropped, None)]
        with (
            mock.patch.object(os, "fork", side_effect=OSError("EAGAIN")),
            mock.patch.object(signal, "signal"),
        ):
            broker.serve(srv, audit, max_requests=1, spawn=True)
        dropped.close.assert_called_once_with()
        self.assertEqual(srv.accept.call_count, 1)
        self.assertFalse(os.path.exists(audit))

    def test_child_serves_and_exits(self):
        tmp = _mktemp(self, "spawn-")
        _write_proc(
            tmp,
            os.getpid(),
            comm="test\n",
            cgroup="0::/user.slice/user-1001.slice/session-13.scope\n",
        )
        audit = str(tmp / "audit.log")
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        ours.sendall(b"WHOAMI\n")
        ours.shutdown(socket.SHUT_WR)
        srv = mock.Mock()
        srv.accept.return_value = (peer, None)
        with (
            mock.patch.object(os, "fork", return_value=0),
            mock.patch.object(os, "_exit") as gone,
            mock.patch.object(signal, "signal"),
        ):
            broker.serve(srv, audit, str(tmp), max_requests=1, spawn=True)
        gone.assert_called_once_with(0)
        self.assertIn("role=netadmin", _read_all(ours).decode())
        with open(audit) as handle:
            record = handle.read()
        self.assertIn("verb=WHOAMI", record)
        self.assertIn("result=ok", record)

    def test_child_exits_on_unexpected_error(self):
        tmp = _mktemp(self, "spawn-")
        audit = str(tmp / "audit.log")
        conn = mock.Mock()
        srv = mock.Mock()
        srv.accept.return_value = (conn, None)
        with (
            mock.patch.object(os, "fork", return_value=0),
            mock.patch.object(os, "_exit") as gone,
            mock.patch.object(signal, "signal"),
            mock.patch.object(broker, "serve_once", side_effect=ValueError("boom")),
        ):
            with self.assertRaises(ValueError):
                broker.serve(srv, audit, max_requests=1, spawn=True)
        gone.assert_called_once_with(0)
        conn.close.assert_called_once_with()
        self.assertEqual(srv.accept.call_count, 1)
        # The child serves one connection, so it drops its inherited
        # copy of the listening socket before serving.
        srv.close.assert_called_once_with()


class ServeTest(unittest.TestCase):
    def _mock_conn(self):
        conn = mock.Mock()
        conn.getsockopt.return_value = struct.pack("3i", 8, 1000, 1000)
        conn.recv.side_effect = [b"WHOAMI\n", BlockingIOError()]
        return conn

    def test_discard_setblocking_down_error(self):
        conn = mock.Mock()
        conn.setblocking.side_effect = OSError("down")
        broker._discard_unread(conn)
        conn.recv.assert_not_called()

    def test_discard_setblocking_up_error(self):
        conn = mock.Mock()
        conn.recv.side_effect = BlockingIOError()
        conn.setblocking.side_effect = [None, OSError("up")]
        broker._discard_unread(conn)
        self.assertEqual(conn.setblocking.call_count, 2)

    def test_bounded_run(self):
        tmp = _mktemp(self, "serve-")
        _write_proc(
            tmp,
            8,
            comm="c\n",
            cgroup="0::/user.slice/user-1000.slice/session-2.scope\n",
        )
        audit = str(tmp / "audit.log")
        srv = mock.Mock()
        srv.accept.side_effect = [
            (self._mock_conn(), None),
            (self._mock_conn(), None),
        ]
        broker.serve(srv, audit, str(tmp), max_requests=2, spawn=False)
        self.assertEqual(srv.accept.call_count, 2)
        with open(audit) as handle:
            self.assertEqual(len(handle.read().splitlines()), 2)

    def test_audit_failure_does_not_kill_loop(self):
        srv = mock.Mock()
        srv.accept.return_value = (self._mock_conn(), None)
        broker.serve(srv, "/nonexistent-dir-xyz/audit.log", max_requests=1, spawn=False)
        self.assertEqual(srv.accept.call_count, 1)

    def test_accept_error_sheds_one_round_and_continues(self):
        tmp = _mktemp(self, "serve-")
        audit = str(tmp / "audit.log")
        srv = mock.Mock()
        srv.accept.side_effect = [
            OSError(errno.EINTR, "EINTR"),
            (self._mock_conn(), None),
        ]
        with mock.patch.object(time, "sleep") as slept:
            # spawn=False serves inline: the shed-round assertions must not
            # race a forked child for the audit record.
            broker.serve(srv, audit, max_requests=2, spawn=False)
        slept.assert_called_once_with(0.05)
        self.assertEqual(srv.accept.call_count, 2)
        with open(audit) as handle:
            self.assertEqual(len(handle.read().splitlines()), 1)

    def test_accept_fatal_error_reraises(self):
        srv = mock.Mock()
        srv.accept.side_effect = OSError(errno.EBADF, "EBADF")
        with self.assertRaises(OSError):
            broker.serve(srv, "/nonexistent/audit.log", max_requests=1)

    def test_unbounded_loop_evaluates(self):
        srv = mock.Mock()
        srv.accept.side_effect = KeyboardInterrupt()
        with self.assertRaises(KeyboardInterrupt):
            broker.serve(srv, "/nonexistent/audit.log", spawn=False)

    def test_live_accept_loop(self):
        tmp = _mktemp(self, "serve-")
        self._fixture_self(tmp)
        audit = str(tmp / "audit.log")
        name = _abstract_name()
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.bind(name)
        sock.listen(16)
        self.addCleanup(sock.close)
        worker = threading.Thread(
            target=broker.serve,
            args=(sock, audit, str(tmp)),
            kwargs={"max_requests": 1, "spawn": False},
            daemon=True,
        )
        worker.start()
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(name)
        client.sendall(b"WHOAMI\n")
        reply = _read_all(client).decode()
        client.close()
        worker.join(timeout=10)
        self.assertFalse(worker.is_alive())
        self.assertIn("role=netadmin", reply)

    def _fixture_self(self, root):
        _write_proc(
            root,
            os.getpid(),
            comm="test\n",
            cgroup="0::/user.slice/user-1001.slice/session-12.scope\n",
        )


class MainTest(unittest.TestCase):
    def test_full_run_one_request(self):
        tmp = _mktemp(self, "main-")
        _write_proc(
            tmp,
            os.getpid(),
            comm="test\n",
            cgroup="0::/user.slice/user-1000.slice/session-4.scope\n",
        )
        audit = str(tmp / "audit.log")
        name = _abstract_name()
        result = {}

        def run():
            result["rc"] = broker.main(
                [
                    "--socket-name",
                    name,
                    "--audit-path",
                    audit,
                    "--proc-root",
                    str(tmp),
                    "--max-requests",
                    "1",
                ]
            )

        # Mocked child: serve() forks per connection in production, but the
        # unit suite pins the child path in-process (fork -> 0, _exit mock).
        with (
            mock.patch.object(os, "fork", return_value=0),
            mock.patch.object(os, "_exit") as gone,
            mock.patch.object(signal, "signal") as sig,
        ):
            worker = threading.Thread(target=run, daemon=True)
            worker.start()
            client = None
            for _ in range(100):
                attempt = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                try:
                    attempt.connect(name)
                    client = attempt
                    break
                except OSError:
                    attempt.close()
                    time.sleep(0.05)
            self.assertIsNotNone(client)
            client.sendall(b"WHOAMI\n")
            reply = _read_all(client).decode()
            client.close()
            worker.join(timeout=10)
            self.assertFalse(worker.is_alive())
        self.assertEqual(result["rc"], 0)
        self.assertIn("role=netops", reply)
        gone.assert_called_once_with(0)
        sig.assert_any_call(signal.SIGCHLD, broker._reap_children)

    def test_zero_requests_exits(self):
        tmp = _mktemp(self, "main-")
        rc = broker.main(
            [
                "--socket-name",
                _abstract_name(),
                "--audit-path",
                str(tmp / "audit.log"),
                "--max-requests",
                "0",
            ]
        )
        self.assertEqual(rc, 0)

    def test_bad_argv_exits_nonzero(self):
        with self.assertRaises(SystemExit) as ctx:
            broker.main(["--max-requests", "abc"])
        self.assertNotEqual(ctx.exception.code, 0)


class MainGuardTest(unittest.TestCase):
    def test_main_guard_runs(self):
        tmp = _mktemp(self, "guard-")
        argv = [
            "admin-broker",
            "--socket-name",
            _abstract_name(),
            "--audit-path",
            str(tmp / "audit.log"),
            "--max-requests",
            "0",
        ]
        with mock.patch.object(sys, "argv", argv):
            with self.assertRaises(SystemExit) as ctx:
                runpy.run_path(_daemon_path(), run_name="__main__")
        self.assertEqual(ctx.exception.code, 0)


class SanitizeLineTest(unittest.TestCase):
    def test_suffix(self):
        self.assertEqual(broker.sanitize_line("boom\nfailed"), ": boom failed")

    def test_empty(self):
        self.assertEqual(broker.sanitize_line("  \n "), "")

    def test_truncates(self):
        self.assertEqual(len(broker.sanitize_line("x" * 5000)), 514)


class AllowlistTest(unittest.TestCase):
    def test_missing_file_denies_all(self):
        tmp = _mktemp(self, "allow-")
        self.assertEqual(broker.load_allowlist(str(tmp / "nope.conf")), {})

    def test_parses_verbs_skips_comments(self):
        tmp = _mktemp(self, "allow-")
        path = tmp / "allowlist.conf"
        path.write_text(
            "# heading comment\n"
            "\n"
            "fboss_sw_agent.service: STATUS, restart  # trailing comment\n"
            "   \n"
            "fboss_hw_agent@.service: STATUS,\n"
        )
        self.assertEqual(
            broker.load_allowlist(str(path)),
            {
                "fboss_sw_agent.service": frozenset({"STATUS", "RESTART"}),
                "fboss_hw_agent@.service": frozenset({"STATUS"}),
            },
        )

    def test_bare_unit_defaults_status_only(self):
        tmp = _mktemp(self, "allow-")
        path = tmp / "allowlist.conf"
        path.write_text("fsdb.service\n")
        self.assertEqual(
            broker.load_allowlist(str(path)), {"fsdb.service": frozenset({"STATUS"})}
        )

    def test_unknown_verb_warns_and_drops(self):
        tmp = _mktemp(self, "allow-")
        path = tmp / "allowlist.conf"
        path.write_text("a.service: STATUS, RESTRAT\n")
        stream = io.StringIO()
        with mock.patch.object(sys, "stderr", stream):
            mapping = broker.load_allowlist(str(path))
        self.assertEqual(mapping, {"a.service": frozenset({"STATUS"})})
        self.assertIn("unknown verb 'RESTRAT' ignored", stream.getvalue())

    def test_empty_verbs_deny_loudly(self):
        tmp = _mktemp(self, "allow-")
        path = tmp / "allowlist.conf"
        path.write_text("a.service:\nb.service: FOO\n")
        stream = io.StringIO()
        with mock.patch.object(sys, "stderr", stream):
            mapping = broker.load_allowlist(str(path))
        self.assertEqual(mapping, {"a.service": frozenset(), "b.service": frozenset()})
        err = stream.getvalue()
        self.assertIn("has no valid verbs; denying every verb", err)
        self.assertIn("'a.service'", err)
        self.assertIn("'b.service'", err)


class UnitSyntaxTest(unittest.TestCase):
    def test_valid(self):
        for unit in (
            "fboss_sw_agent.service",
            "fboss_hw_agent@0.service",
            "a-b_c.d@1.service",
        ):
            self.assertTrue(broker.valid_unit_syntax(unit), unit)

    def test_invalid(self):
        for unit in (
            "",
            "nope",
            "../x.service",
            "a b.service",
            "a.service/x",
            "a.socket",
            "a@Service",
        ):
            self.assertFalse(broker.valid_unit_syntax(unit), unit)


class PatternTest(unittest.TestCase):
    ALLOW = {
        "fboss_sw_agent.service": frozenset({"STATUS"}),
        "fboss_hw_agent@.service": frozenset({"STATUS"}),
    }

    def test_exact(self):
        self.assertEqual(
            broker.pattern_for("fboss_sw_agent.service", self.ALLOW),
            "fboss_sw_agent.service",
        )

    def test_template_numeric(self):
        self.assertEqual(
            broker.pattern_for("fboss_hw_agent@0.service", self.ALLOW),
            "fboss_hw_agent@.service",
        )
        self.assertEqual(
            broker.pattern_for("fboss_hw_agent@12.service", self.ALLOW),
            "fboss_hw_agent@.service",
        )

    def test_template_evil_instance(self):
        self.assertIsNone(broker.pattern_for("fboss_hw_agent@x.service", self.ALLOW))
        self.assertIsNone(
            broker.pattern_for("fboss_hw_agent@0.service@1.service", self.ALLOW)
        )

    def test_template_bare_rejected(self):
        self.assertIsNone(broker.pattern_for("fboss_hw_agent@.service", self.ALLOW))

    def test_mismatch(self):
        self.assertIsNone(broker.pattern_for("qsfp_service.service", self.ALLOW))
        self.assertIsNone(broker.pattern_for("qsfp@0.service", self.ALLOW))
        self.assertIsNone(broker.pattern_for("fboss_hw_agent@0.timer", self.ALLOW))
        self.assertIsNone(broker.pattern_for("fboss_sw_agent.service", {}))

    def test_non_template_entries_skipped(self):
        allow = {"plain.service": frozenset({"STATUS"})}
        self.assertIsNone(broker.pattern_for("other@0.service", allow))


class RunSystemctlTest(unittest.TestCase):
    def test_argv_and_timeout(self):
        with mock.patch.object(broker.subprocess, "run") as run:
            broker.run_systemctl(["show", "a.service"])
        run.assert_called_once_with(
            [
                "/usr/bin/systemctl",
                "--no-pager",
                "--no-ask-password",
                "show",
                "a.service",
            ],
            capture_output=True,
            text=True,
            timeout=120,
        )

    def test_timeout_override(self):
        with mock.patch.object(broker.subprocess, "run") as run:
            broker.run_systemctl(["show", "a.service"], timeout=5)
        self.assertEqual(run.call_args[1]["timeout"], 5)


def _proc_result(rc=0, stdout="", stderr=""):
    proc = mock.Mock()
    proc.returncode = rc
    proc.stdout = stdout
    proc.stderr = stderr
    return proc


class ControlUnitTest(unittest.TestCase):
    ALLOW = {
        "fboss_sw_agent.service": frozenset(broker.UNIT_VERBS),
        "fboss_hw_agent@.service": frozenset(broker.UNIT_VERBS),
        "fsdb.service": frozenset({"STATUS"}),
    }

    def test_bad_unit(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "START", "../x.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR bad-unit", "err:bad-unit"))
        run.assert_not_called()

    def test_unknown_role_forbidden(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "STATUS", "fboss_sw_agent.service", "unknown", 0, self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR forbidden", "err:forbidden"))
        run.assert_not_called()

    def test_not_allowlisted(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "STATUS", "cron.service", "netadmin", 1001, self.ALLOW, run
        )
        self.assertEqual(
            (reply, result), ("ERR not-allowlisted", "err:not-allowlisted")
        )
        run.assert_not_called()

    def test_netops_status_ok_but_mutating_forbidden(self):
        run = mock.Mock(return_value=_proc_result(0, "ActiveState=active\n"))
        reply, result = broker.control_unit(
            "STATUS", "fboss_sw_agent.service", "netops", 1000, self.ALLOW, run
        )
        self.assertEqual(result, "ok")
        self.assertIn("ActiveState=active", reply)
        run.assert_called_once_with(
            ["show", "fboss_sw_agent.service", "-p", "ActiveState,SubState,MainPID"]
        )
        reply, result = broker.control_unit(
            "RESTART", "fboss_sw_agent.service", "netops", 1000, self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR forbidden", "err:forbidden"))

    def test_netadmin_mutating_ok(self):
        run = mock.Mock(return_value=_proc_result(0))
        for verb in ("START", "STOP", "RESTART"):
            reply, result = broker.control_unit(
                verb, "fboss_sw_agent.service", "netadmin", 0, self.ALLOW, run
            )
            self.assertEqual((reply, result), ("ok", "ok"), verb)
        run.assert_called_with(["restart", "fboss_sw_agent.service"])

    def test_netadmin_unescaped_mutating_forbidden(self):
        run = mock.Mock()
        for verb in ("START", "STOP", "RESTART"):
            reply, result = broker.control_unit(
                verb, "fboss_sw_agent.service", "netadmin", 1001, self.ALLOW, run
            )
            self.assertEqual((reply, result), ("ERR forbidden", "err:forbidden"), verb)
        run.assert_not_called()

    def test_template_instance_ok(self):
        run = mock.Mock(return_value=_proc_result(0))
        reply, result = broker.control_unit(
            "STATUS", "fboss_hw_agent@0.service", "netadmin", 1001, self.ALLOW, run
        )
        self.assertEqual(result, "ok")

    def test_status_failed(self):
        run = mock.Mock(return_value=_proc_result(4, "", "not found"))
        reply, result = broker.control_unit(
            "STATUS", "fboss_sw_agent.service", "netadmin", 1001, self.ALLOW, run
        )
        self.assertEqual(
            (reply, result),
            ("ERR status-failed rc=4: not found", "err:status-failed"),
        )

    def test_mutating_failed_with_detail(self):
        run = mock.Mock(return_value=_proc_result(1, "", "job\nfailed"))
        reply, result = broker.control_unit(
            "STOP", "fboss_sw_agent.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual(reply, "ERR stop-failed rc=1: job failed")
        self.assertEqual(result, "err:stop-failed")

    def test_mutating_failed_without_detail(self):
        run = mock.Mock(return_value=_proc_result(1, "", " \n "))
        reply, result = broker.control_unit(
            "STOP", "fboss_sw_agent.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual(reply, "ERR stop-failed rc=1")

    def test_verb_not_allowed(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "RESTART", "fsdb.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual(
            (reply, result), ("ERR verb-not-allowed", "err:verb-not-allowed")
        )
        run.assert_not_called()

    def test_verb_allowed_template(self):
        run = mock.Mock(return_value=_proc_result(0))
        reply, result = broker.control_unit(
            "RESTART", "fboss_hw_agent@0.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ok", "ok"))

    def test_timeout(self):
        import subprocess as _subprocess

        run = mock.Mock(side_effect=_subprocess.TimeoutExpired("systemctl", 120))
        reply, result = broker.control_unit(
            "RESTART", "fboss_sw_agent.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR timeout", "err:timeout"))

    def test_exec_error(self):
        run = mock.Mock(side_effect=OSError("gone"))
        reply, result = broker.control_unit(
            "RESTART", "fboss_sw_agent.service", "netadmin", 0, self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR exec-error", "err:exec-error"))


class UnitDispatchTest(unittest.TestCase):
    def _ident(self, uid=1001, pid=7):
        return {"uid": uid, "gid": 1002, "pid": pid}

    def test_empty_command(self):
        root = _mktemp(self, "cmd-")
        reply, _, result = broker.handle_command("", self._ident(), str(root))
        self.assertEqual(
            (reply, result), ("ERR unknown-command", "err:unknown-command")
        )

    def test_bare_verb_with_arg_rejected(self):
        root = _mktemp(self, "cmd-")
        reply, _, result = broker.handle_command("WHOAMI X", self._ident(), str(root))
        self.assertEqual(result, "err:unknown-command")

    def test_missing_unit(self):
        root = _mktemp(self, "cmd-")
        reply, context, result = broker.handle_command(
            "STATUS", self._ident(), str(root)
        )
        self.assertEqual((reply, result), ("ERR missing-unit", "err:missing-unit"))
        self.assertEqual(context["target"], "-")

    def test_unit_dispatch_audits_target(self):
        root = _mktemp(self, "cmd-")
        run = mock.Mock(return_value=_proc_result(0))
        allow = {"fboss_sw_agent.service": frozenset(broker.UNIT_VERBS)}
        reply, context, result = broker.handle_command(
            "STATUS   fboss_sw_agent.service  ", self._ident(), str(root), allow, run
        )
        self.assertEqual(result, "ok")
        self.assertEqual(context["target"], "fboss_sw_agent.service")
        run.assert_called_once_with(
            ["show", "fboss_sw_agent.service", "-p", "ActiveState,SubState,MainPID"]
        )


class ServeUnitTest(unittest.TestCase):
    def test_status_roundtrip_audits_target(self):
        tmp = _mktemp(self, "serve-")
        _write_proc(
            tmp,
            os.getpid(),
            comm="test\n",
            cgroup="0::/user.slice/user-1001.slice/session-21.scope\n",
        )
        audit = str(tmp / "audit.log")
        allow = {"fboss_sw_agent.service": frozenset(broker.UNIT_VERBS)}
        run = mock.Mock(return_value=_proc_result(0, "ActiveState=active\n"))
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        ours.sendall(b"STATUS fboss_sw_agent.service\n")
        ours.shutdown(socket.SHUT_WR)
        result = broker.serve_once(peer, audit, str(tmp), allow, run)
        peer.close()
        reply = _read_all(ours).decode()
        self.assertEqual(result, "ok")
        self.assertIn("ActiveState=active", reply)
        with open(audit) as handle:
            record = handle.read()
        self.assertIn("verb=STATUS peer_uid=", record)
        self.assertIn("target=fboss_sw_agent.service result=ok", record)


class MainAllowlistTest(unittest.TestCase):
    def test_empty_allowlist_warns(self):
        tmp = _mktemp(self, "main-")
        stream = io.StringIO()
        with mock.patch.object(sys, "stderr", stream):
            rc = broker.main(
                [
                    "--socket-name",
                    _abstract_name(),
                    "--audit-path",
                    str(tmp / "audit.log"),
                    "--allowlist-path",
                    str(tmp / "missing.conf"),
                    "--max-requests",
                    "0",
                ]
            )
        self.assertEqual(rc, 0)
        self.assertIn("warning: empty allowlist", stream.getvalue())

    def test_loaded_allowlist_counts(self):
        tmp = _mktemp(self, "main-")
        path = tmp / "allowlist.conf"
        path.write_text("a.service\nb.service\n")
        stream = io.StringIO()
        with mock.patch.object(sys, "stderr", stream):
            rc = broker.main(
                [
                    "--socket-name",
                    _abstract_name(),
                    "--audit-path",
                    str(tmp / "audit.log"),
                    "--allowlist-path",
                    str(path),
                    "--max-requests",
                    "0",
                ]
            )
        self.assertEqual(rc, 0)
        self.assertIn("allowlist: 2 units", stream.getvalue())


class SelfCoverageTest(unittest.TestCase):
    def test_all_daemon_lines_execute(self):
        loader = unittest.TestLoader()
        suite = unittest.TestSuite()
        this = sys.modules[__name__]
        for attr in dir(this):
            obj = getattr(this, attr)
            if (
                isinstance(obj, type)
                and issubclass(obj, unittest.TestCase)
                and obj is not SelfCoverageTest
            ):
                suite.addTests(loader.loadTestsFromTestCase(obj))
        tracer = trace.Trace(count=1, trace=0, ignoredirs=[sys.prefix, sys.exec_prefix])

        def fresh_run():
            sys.modules.pop(MODNAME, None)
            globals()["broker"] = load_daemon()
            runner = unittest.TextTestRunner(stream=io.StringIO())
            return runner.run(suite)

        result = tracer.runfunc(fresh_run)
        self.assertTrue(
            result.wasSuccessful(),
            "inner suite failed: %s" % result,
        )
        daemon_path = os.path.realpath(_daemon_path())
        with open(daemon_path) as handle:
            code = compile(handle.read(), daemon_path, "exec")

        executable = set()

        def walk(node):
            for _, lineno in dis.findlinestarts(node):
                # Lineno 0 is implicit setup (module RESUME), not a source line.
                if lineno:
                    executable.add(lineno)
            for const in node.co_consts:
                if hasattr(const, "co_code"):
                    walk(const)

        walk(code)
        counts = tracer.results().counts
        executed = {
            lineno for (path, lineno) in counts if os.path.realpath(path) == daemon_path
        }
        missed = sorted(executable - executed)
        self.assertEqual(missed, [], "daemon lines never executed: %s" % (missed,))
