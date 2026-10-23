# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

"""Whitebox tests for the host-ctl EXEC client.

Loads payload/usr/local/bin/host-ctl as a module (BUCK resource, with a
repo-relative fallback like the admin-broker tests) and exercises every
function, including frame parsing, session relay, and the __main__
guard. SelfCoverageTest re-runs the whole suite under stdlib trace and
fails unless every executable line ran: the 100% gate travels with the
tests, under BUCK or raw. All process and network doubles are mocks or
pre-buffered socketpairs; no real forks, threads, or listeners.
"""

import contextlib
import dis
import io
import os
import runpy
import socket
import sys
import trace
import unittest
from pathlib import Path
from unittest import mock

SCRIPT_NAME = "host-ctl"
MODNAME = "host_ctl_under_test"


def _client_path():
    try:
        from importlib.resources import files

        package = __package__ or "admin_broker.tests"
        return str(files(package) / SCRIPT_NAME)
    except (FileNotFoundError, ModuleNotFoundError):
        return str(
            Path(__file__).resolve().parents[1] / "payload/usr/local/bin" / SCRIPT_NAME
        )


def load_client():
    import importlib.util
    from importlib.machinery import SourceFileLoader

    loader = SourceFileLoader(MODNAME, _client_path())
    spec = importlib.util.spec_from_loader(MODNAME, loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[MODNAME] = module
    loader.exec_module(module)
    return module


helper = load_client()


class ShebangTest(unittest.TestCase):
    def test_unversioned_python3(self):
        with open(_client_path()) as handle:
            first = handle.readline().rstrip("\n")
        self.assertEqual(first, "#!/usr/bin/python3")


class FrameReaderTest(unittest.TestCase):
    def _pair(self):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        return ours, peer

    def test_line(self):
        ours, peer = self._pair()
        ours.sendall(b"DATA 3\n")
        ours.shutdown(socket.SHUT_WR)
        reader = helper.FrameReader(peer)
        self.assertEqual(reader.read_line(), b"DATA 3")

    def test_split_line(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"DA", b"TA 3\n"]
        reader = helper.FrameReader(conn)
        self.assertEqual(reader.read_line(), b"DATA 3")

    def test_line_eof(self):
        ours, peer = self._pair()
        ours.shutdown(socket.SHUT_WR)
        reader = helper.FrameReader(peer)
        self.assertIsNone(reader.read_line())

    def test_line_error(self):
        conn = mock.Mock()
        conn.recv.side_effect = OSError("gone")
        reader = helper.FrameReader(conn)
        self.assertIsNone(reader.read_line())

    def test_exact(self):
        ours, peer = self._pair()
        ours.sendall(b"hello")
        ours.shutdown(socket.SHUT_WR)
        reader = helper.FrameReader(peer)
        self.assertEqual(reader.read_exact(5), b"hello")

    def test_exact_split(self):
        conn = mock.Mock()
        conn.recv.side_effect = [b"ab", b"cde"]
        reader = helper.FrameReader(conn)
        self.assertEqual(reader.read_exact(5), b"abcde")

    def test_exact_short(self):
        ours, peer = self._pair()
        ours.sendall(b"ab")
        ours.shutdown(socket.SHUT_WR)
        reader = helper.FrameReader(peer)
        self.assertIsNone(reader.read_exact(5))

    def test_exact_error(self):
        conn = mock.Mock()
        conn.recv.side_effect = OSError("gone")
        reader = helper.FrameReader(conn)
        self.assertIsNone(reader.read_exact(5))


class ParseHeaderTest(unittest.TestCase):
    def test_data(self):
        self.assertEqual(helper.parse_header(b"DATA 12"), ("DATA", 12))

    def test_exit(self):
        self.assertEqual(helper.parse_header(b"EXIT 0"), ("EXIT", 0))
        self.assertEqual(helper.parse_header(b"EXIT -9"), ("EXIT", -9))

    def test_malformed(self):
        for line in (b"DATA", b"BOGUS 1", b"DATA x", b"DATA -1", b""):
            self.assertEqual(helper.parse_header(line), (None, None))


class WriteAllTest(unittest.TestCase):
    def test_roundtrip(self):
        r, w = os.pipe()
        try:
            helper._write_all(w, b"hello")
        finally:
            os.close(w)
        self.assertEqual(os.read(r, 5), b"hello")
        os.close(r)

    def test_partial_writes_loop(self):
        with mock.patch.object(os, "write", side_effect=[2, 3]) as wrote:
            helper._write_all(9, b"abcde")
        self.assertEqual(
            wrote.call_args_list, [mock.call(9, b"abcde"), mock.call(9, b"cde")]
        )


class FramingContractTest(unittest.TestCase):
    def test_client_chunk_within_broker_cap(self):
        import importlib.util
        from importlib.machinery import SourceFileLoader

        try:
            from importlib.resources import files

            package = __package__ or "admin_broker.tests"
            path = str(files(package) / "admin-broker")
        except (FileNotFoundError, ModuleNotFoundError):
            path = str(
                Path(__file__).resolve().parents[1]
                / "payload/usr/local/sbin/admin-broker"
            )
        loader = SourceFileLoader("broker_framing_contract", path)
        spec = importlib.util.spec_from_loader("broker_framing_contract", loader)
        broker = importlib.util.module_from_spec(spec)
        sys.modules["broker_framing_contract"] = broker
        loader.exec_module(broker)
        # host-ctl reads stdin in CHUNK slices, so no IN frame it emits
        # can exceed CHUNK; the broker must accept at least that much.
        self.assertLessEqual(helper.CHUNK, broker.MAX_IN_CHUNK)


class RunSessionTest(unittest.TestCase):
    def _pair(self):
        ours, peer = socket.socketpair()
        self.addCleanup(ours.close)
        self.addCleanup(peer.close)
        return ours, peer

    def _stdin(self, data):
        r, w = os.pipe()
        if data:
            os.write(w, data)
        os.close(w)
        self.addCleanup(os.close, r)
        return r

    def _stdout(self):
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        return r, w

    def test_ok(self):
        server, client = self._pair()
        server.sendall(b"DATA 3\nhi\nEXIT 0\n")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        code = helper.run_session(client, self._stdin(b""), out_w)
        os.close(out_w)
        self.assertEqual(code, 0)
        self.assertEqual(os.read(out_r, 3), b"hi\n")

    def test_stdin_forwarded(self):
        server, client = self._pair()
        server.sendall(b"EXIT 0\n")
        out_r, out_w = self._stdout()
        code = helper.run_session(client, self._stdin(b"xy"), out_w)
        os.close(out_w)
        self.assertEqual(code, 0)
        # EXIT arrived before stdin EOF was consumed, so half-close here
        # to let the server side observe end of stream.
        client.shutdown(socket.SHUT_WR)
        got = b""
        while True:
            chunk = server.recv(65536)
            if not chunk:
                break
            got += chunk
        self.assertEqual(got, b"IN 2\nxy")

    def test_stdin_binary_framed(self):
        server, client = self._pair()
        server.sendall(b"EXIT 0\n")
        out_r, out_w = self._stdout()
        code = helper.run_session(client, self._stdin(b"a\nb\x00c"), out_w)
        os.close(out_w)
        self.assertEqual(code, 0)
        client.shutdown(socket.SHUT_WR)
        got = b""
        while True:
            chunk = server.recv(65536)
            if not chunk:
                break
            got += chunk
        self.assertEqual(got, b"IN 5\na\nb\x00c")

    def test_output_flows_after_in_eof(self):
        server, client = self._pair()
        server.sendall(b"DATA 3\nhi\nEXIT 0\n")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        code = helper.run_session(client, self._stdin(b""), out_w)
        os.close(out_w)
        self.assertEqual(code, 0)
        self.assertEqual(os.read(out_r, 3), b"hi\n")
        self.assertEqual(server.recv(7), b"IN_EOF\n")

    def test_refused(self):
        server, client = self._pair()
        server.sendall(b"ERR forbidden")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        with self.assertRaises(helper._Refused) as ctx:
            helper.run_session(client, self._stdin(b""), out_w)
        self.assertEqual(str(ctx.exception), "ERR forbidden")
        os.close(out_w)

    def test_refused_with_newline(self):
        server, client = self._pair()
        server.sendall(b"ERR forbidden\n")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        with self.assertRaises(helper._Refused) as ctx:
            helper.run_session(client, self._stdin(b""), out_w)
        self.assertEqual(str(ctx.exception), "ERR forbidden")
        os.close(out_w)

    def test_lost_on_eof(self):
        server, client = self._pair()
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        self.assertIsNone(helper.run_session(client, self._stdin(b""), out_w))
        os.close(out_w)

    def test_lost_on_truncated_frame(self):
        server, client = self._pair()
        server.sendall(b"DATA 10")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        self.assertIsNone(helper.run_session(client, self._stdin(b""), out_w))
        os.close(out_w)

    def test_lost_on_malformed_header(self):
        server, client = self._pair()
        server.sendall(b"BOGUS\n")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        self.assertIsNone(helper.run_session(client, self._stdin(b""), out_w))
        os.close(out_w)

    def test_lost_on_short_payload(self):
        server, client = self._pair()
        server.sendall(b"DATA 10\nabc")
        server.shutdown(socket.SHUT_WR)
        out_r, out_w = self._stdout()
        self.assertIsNone(helper.run_session(client, self._stdin(b""), out_w))
        os.close(out_w)

    def test_lost_on_send_error(self):
        sock = mock.Mock()
        sock.sendall.side_effect = OSError("gone")
        out_r, out_w = self._stdout()
        stdin = self._stdin(b"xy")
        with mock.patch.object(helper.select, "select", return_value=([stdin], [], [])):
            self.assertIsNone(helper.run_session(sock, stdin, out_w))
        os.close(out_w)

    def test_lost_on_in_eof_error(self):
        sock = mock.Mock()
        sock.sendall.side_effect = OSError("gone")
        out_r, out_w = self._stdout()
        stdin = self._stdin(b"")
        with mock.patch.object(helper.select, "select", return_value=([stdin], [], [])):
            self.assertIsNone(helper.run_session(sock, stdin, out_w))
        os.close(out_w)

    def test_stdin_read_error_means_eof(self):
        sock = mock.Mock()
        sock.recv.return_value = b""
        with (
            mock.patch.object(
                helper.select,
                "select",
                side_effect=[([99], [], []), ([sock], [], [])],
            ),
            mock.patch.object(os, "read", side_effect=OSError("bad")),
        ):
            self.assertIsNone(helper.run_session(sock, 99, 100))
        sock.sendall.assert_called_once_with(b"IN_EOF\n")

    def test_lost_on_stdout_error(self):
        server, client = self._pair()
        server.sendall(b"DATA 2\nzzEXIT 0\n")
        server.shutdown(socket.SHUT_WR)
        dead_r, dead_w = os.pipe()
        os.close(dead_r)
        os.close(dead_w)
        self.assertIsNone(helper.run_session(client, self._stdin(b""), dead_w))


class MainTest(unittest.TestCase):
    def test_help(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(helper.main(["--help"]), 0)
        self.assertIn("usage:", out.getvalue())

    def test_help_short(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(helper.main(["-h"]), 0)
        self.assertIn("usage:", out.getvalue())

    def test_no_args(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.assertEqual(helper.main([]), 2)
        self.assertIn("usage:", err.getvalue())

    def test_unknown_verb(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.assertEqual(helper.main(["frobnicate"]), 2)
        self.assertIn("usage:", err.getvalue())

    def test_exec_without_command(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.assertEqual(helper.main(["exec"]), 2)
        self.assertIn("needs a command", err.getvalue())

    def test_broker_unreachable(self):
        err = io.StringIO()
        with mock.patch.object(
            helper, "SOCKET_NAME", "\0host-ctl-test-dead-%d" % os.getpid()
        ):
            with contextlib.redirect_stderr(err):
                self.assertEqual(helper.main(["exec", "/bin/true"]), 1)
        self.assertIn("cannot reach broker", err.getvalue())

    def test_send_failed(self):
        err = io.StringIO()
        with (
            mock.patch.object(helper, "socket"),
            mock.patch.object(
                helper.socket.socket.return_value,
                "sendall",
                side_effect=OSError("gone"),
            ),
            contextlib.redirect_stderr(err),
        ):
            self.assertEqual(helper.main(["exec", "/bin/true"]), 1)
        self.assertIn("send failed", err.getvalue())

    def test_refused(self):
        err = io.StringIO()
        with (
            mock.patch.object(helper, "socket"),
            mock.patch.object(
                helper, "run_session", side_effect=helper._Refused("ERR forbidden")
            ),
            contextlib.redirect_stderr(err),
        ):
            self.assertEqual(helper.main(["exec", "/bin/true"]), 1)
        self.assertIn("broker refused: ERR forbidden", err.getvalue())

    def test_newline_arg_rejected_before_connect(self):
        err = io.StringIO()
        with (
            mock.patch.object(helper, "socket") as sockmod,
            contextlib.redirect_stderr(err),
        ):
            self.assertEqual(helper.main(["exec", "/bin/echo", "a\nb"]), 2)
        self.assertIn("argument contains newline", err.getvalue())
        sockmod.socket.assert_not_called()

    def test_unencodable_arg_returns_two(self):
        err = io.StringIO()
        with (
            mock.patch.object(helper, "socket"),
            contextlib.redirect_stderr(err),
        ):
            self.assertEqual(helper.main(["exec", "/bin/echo", "\udcff"]), 2)
        self.assertIn("argument not encodable", err.getvalue())

    def test_lost(self):
        err = io.StringIO()
        with (
            mock.patch.object(helper, "socket"),
            mock.patch.object(helper, "run_session", return_value=None),
            contextlib.redirect_stderr(err),
        ):
            self.assertEqual(helper.main(["exec", "/bin/true"]), 1)
        self.assertIn("connection lost", err.getvalue())

    def test_ok_returns_remote_code(self):
        with (
            mock.patch.object(helper, "socket") as sockmod,
            mock.patch.object(helper, "run_session", return_value=3) as run,
        ):
            self.assertEqual(helper.main(["exec", "/bin/echo", "hi"]), 3)
        sock = sockmod.socket.return_value
        run.assert_called_once_with(sock, 0, 1)
        sent = [call[0][0] for call in sock.sendall.call_args_list]
        self.assertEqual(sent, [b"EXEC 2\n", b"/bin/echo\n", b"hi\n"])


class MainGuardTest(unittest.TestCase):
    def test_main_guard_runs(self):
        out = io.StringIO()
        with mock.patch.object(sys, "argv", ["host-ctl", "--help"]):
            with contextlib.redirect_stdout(out):
                with self.assertRaises(SystemExit) as ctx:
                    runpy.run_path(_client_path(), run_name="__main__")
        self.assertEqual(ctx.exception.code, 0)
        self.assertIn("usage:", out.getvalue())


class SelfCoverageTest(unittest.TestCase):
    def test_all_client_lines_execute(self):
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
            globals()["helper"] = load_client()
            runner = unittest.TextTestRunner(stream=io.StringIO())
            return runner.run(suite)

        result = tracer.runfunc(fresh_run)
        self.assertTrue(
            result.wasSuccessful(),
            "inner suite failed: %s" % result,
        )
        client_path = os.path.realpath(_client_path())
        with open(client_path) as handle:
            code = compile(handle.read(), client_path, "exec")

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
            lineno for (path, lineno) in counts if os.path.realpath(path) == client_path
        }
        missed = sorted(executable - executed)
        self.assertEqual(missed, [], "client lines never executed: %s" % (missed,))
