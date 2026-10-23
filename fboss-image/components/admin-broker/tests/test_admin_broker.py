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
            r"target=- result=ok\n$",
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

    def test_unknown(self):
        root = _mktemp(self, "cmd-")
        reply, _, result = broker.handle_command("BOGUS", self._ident(), str(root))
        self.assertEqual(reply, "ERR unknown-command")
        self.assertEqual(result, "err:unknown-command")


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
            "START", "../x.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR bad-unit", "err:bad-unit"))
        run.assert_not_called()

    def test_unknown_role_forbidden(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "STATUS", "fboss_sw_agent.service", "unknown", self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR forbidden", "err:forbidden"))
        run.assert_not_called()

    def test_not_allowlisted(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "STATUS", "cron.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual(
            (reply, result), ("ERR not-allowlisted", "err:not-allowlisted")
        )
        run.assert_not_called()

    def test_netops_status_ok_but_mutating_forbidden(self):
        run = mock.Mock(return_value=_proc_result(0, "ActiveState=active\n"))
        reply, result = broker.control_unit(
            "STATUS", "fboss_sw_agent.service", "netops", self.ALLOW, run
        )
        self.assertEqual(result, "ok")
        self.assertIn("ActiveState=active", reply)
        run.assert_called_once_with(
            ["show", "fboss_sw_agent.service", "-p", "ActiveState,SubState,MainPID"]
        )
        reply, result = broker.control_unit(
            "RESTART", "fboss_sw_agent.service", "netops", self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR forbidden", "err:forbidden"))

    def test_netadmin_mutating_ok(self):
        run = mock.Mock(return_value=_proc_result(0))
        for verb in ("START", "STOP", "RESTART"):
            reply, result = broker.control_unit(
                verb, "fboss_sw_agent.service", "netadmin", self.ALLOW, run
            )
            self.assertEqual((reply, result), ("ok", "ok"), verb)
        run.assert_called_with(["restart", "fboss_sw_agent.service"])

    def test_template_instance_ok(self):
        run = mock.Mock(return_value=_proc_result(0))
        reply, result = broker.control_unit(
            "STATUS", "fboss_hw_agent@0.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual(result, "ok")

    def test_status_failed(self):
        run = mock.Mock(return_value=_proc_result(4, "", "not found"))
        reply, result = broker.control_unit(
            "STATUS", "fboss_sw_agent.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual(
            (reply, result),
            ("ERR status-failed rc=4: not found", "err:status-failed"),
        )

    def test_mutating_failed_with_detail(self):
        run = mock.Mock(return_value=_proc_result(1, "", "job\nfailed"))
        reply, result = broker.control_unit(
            "STOP", "fboss_sw_agent.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual(reply, "ERR stop-failed rc=1: job failed")
        self.assertEqual(result, "err:stop-failed")

    def test_mutating_failed_without_detail(self):
        run = mock.Mock(return_value=_proc_result(1, "", " \n "))
        reply, result = broker.control_unit(
            "STOP", "fboss_sw_agent.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual(reply, "ERR stop-failed rc=1")

    def test_verb_not_allowed(self):
        run = mock.Mock()
        reply, result = broker.control_unit(
            "RESTART", "fsdb.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual(
            (reply, result), ("ERR verb-not-allowed", "err:verb-not-allowed")
        )
        run.assert_not_called()

    def test_verb_allowed_template(self):
        run = mock.Mock(return_value=_proc_result(0))
        reply, result = broker.control_unit(
            "RESTART", "fboss_hw_agent@0.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ok", "ok"))

    def test_timeout(self):
        import subprocess as _subprocess

        run = mock.Mock(side_effect=_subprocess.TimeoutExpired("systemctl", 120))
        reply, result = broker.control_unit(
            "RESTART", "fboss_sw_agent.service", "netadmin", self.ALLOW, run
        )
        self.assertEqual((reply, result), ("ERR timeout", "err:timeout"))

    def test_exec_error(self):
        run = mock.Mock(side_effect=OSError("gone"))
        reply, result = broker.control_unit(
            "RESTART", "fboss_sw_agent.service", "netadmin", self.ALLOW, run
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
