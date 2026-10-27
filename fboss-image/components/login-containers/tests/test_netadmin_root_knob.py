"""Focused test for the netadmin root-escalation knob.

Runs payload/usr/local/sbin/{enable,disable}_netadmin_root black-box
against a temp ROOT with stubbed ssh-keygen/chown/id on PATH, asserting
both gates (machine sudo grant, escape-key trust in host root's
authorized_keys), the disabled flag, idempotency, loud failure on broken
states, root-only refusal, and that netops paths are never touched.
"""

import subprocess
import tempfile
import unittest
from pathlib import Path

MACHINE_SUDOERS = "/var/lib/machines/netadmin/etc/sudoers"
MACHINE_SSH = "/var/lib/machines/netadmin/home/netadmin/.ssh"
ROOT_AUTH = "/root/.ssh/authorized_keys"
DISABLED_FLAG = "/etc/login-containers/netadmin-root-disabled"
KEYGEN_PATH = "/usr/local/sbin/escape-keygen"
OPERATOR_KEY = "ssh-ed25519 OTHERKEY other\n"
ESCAPE_BLOCK = (
    "# fboss-netadmin-escape\n"
    'from="127.0.0.1,::1" ssh-ed25519 OLDPUB fboss-netadmin-escape\n'
)


def _payload_dir():
    return Path(__file__).resolve().parents[1] / "payload"


def _script_bytes(name):
    try:
        from importlib.resources import files

        package = __package__ or "login_containers.tests"
        return (files(package) / name).read_bytes()
    except (FileNotFoundError, ModuleNotFoundError):
        return (_payload_dir() / "usr/local/sbin" / name).read_bytes()


def _shipped_sudoers_bytes():
    return (_payload_dir() / ("var/lib/machines/netadmin/etc/sudoers")).read_bytes()


STUB_KEYGEN = """#!/bin/bash
# Stub ssh-keygen: records invocations, writes deterministic key files.
echo "called $*" >> "$STUB_LOG"
out=""
prev=""
for arg in "$@"; do
    if [ "$prev" = "-f" ]; then
        out="$arg"
    fi
    prev="$arg"
done
[ -n "$out" ] || exit 1
echo "PRIVATE-STUB" > "$out"
echo "ssh-ed25519 STUBPUB fboss-netadmin-escape" > "$out.pub"
"""

STUB_CHOWN = """#!/bin/bash
echo "chown $*" >> "$STUB_LOG"
"""

STUB_ID_ROOT = """#!/bin/bash
echo 0
"""

STUB_ID_NONROOT = """#!/bin/bash
echo 1001
"""

STUB_VISUDO_OK = """#!/bin/bash
echo "visudo $*" >> "$STUB_LOG"
exit 0
"""

STUB_VISUDO_FAIL = """#!/bin/bash
echo "visudo $*" >> "$STUB_LOG"
echo "bad syntax" >&2
exit 1
"""


class NetadminRootKnobTest(unittest.TestCase):
    def _stage(self, id_body=STUB_ID_ROOT, visudo_body=STUB_VISUDO_OK):
        tmp = Path(tempfile.mkdtemp(prefix="netadmin-root-knob-test-"))
        self.addCleanup(lambda: subprocess.run(["rm", "-rf", str(tmp)], check=False))
        root = tmp / "root"
        stubs = tmp / "stubs"
        stubs.mkdir()
        (stubs / "ssh-keygen").write_text(STUB_KEYGEN)
        (stubs / "ssh-keygen").chmod(0o755)
        (stubs / "chown").write_text(STUB_CHOWN)
        (stubs / "chown").chmod(0o755)
        (stubs / "id").write_text(id_body)
        (stubs / "id").chmod(0o755)
        (stubs / "visudo").write_text(visudo_body)
        (stubs / "visudo").chmod(0o755)
        scripts = tmp / "scripts"
        scripts.mkdir()
        for name in ("enable_netadmin_root", "disable_netadmin_root"):
            script = scripts / name
            script.write_bytes(_script_bytes(name))
            script.chmod(0o755)
        keygen_dir = root / "usr/local/sbin"
        keygen_dir.mkdir(parents=True)
        keygen = keygen_dir / "escape-keygen"
        keygen.write_bytes(_script_bytes("escape-keygen"))
        keygen.chmod(0o755)
        env = {
            "PATH": "%s:/usr/bin:/bin" % stubs,
            "STUB_LOG": str(tmp / "stub.log"),
        }
        return tmp, root, scripts, env

    def _run(self, script, args, env):
        return subprocess.run(
            [str(script)] + args,
            env=env,
            capture_output=True,
            text=True,
            timeout=30,
        )

    def _log(self, tmp):
        log = tmp / "stub.log"
        return log.read_text().splitlines() if log.exists() else []

    def _keygen_calls(self, tmp):
        return [line for line in self._log(tmp) if line.startswith("called ")]

    def _write_enabled_state(self, root):
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        sudoers.parent.mkdir(parents=True)
        sudoers.write_bytes(_shipped_sudoers_bytes())
        sudoers.chmod(0o440)
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text(OPERATOR_KEY + ESCAPE_BLOCK)
        ssh_dir = root / MACHINE_SSH.lstrip("/")
        ssh_dir.mkdir(parents=True)
        (ssh_dir / "id_ed25519").write_text("PRIVATE-STUB\n")
        (ssh_dir / "config").write_text("Host localhost 127.0.0.1 ::1\n")

    def test_disable_closes_both_gates_and_flags(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        run = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        self.assertFalse(sudoers.exists())
        moved = root / (MACHINE_SUDOERS.lstrip("/") + ".disabled")
        self.assertEqual(moved.read_bytes(), _shipped_sudoers_bytes())
        auth = root / ROOT_AUTH.lstrip("/")
        self.assertEqual(auth.read_text(), OPERATOR_KEY)
        self.assertEqual(oct(auth.stat().st_mode & 0o777), "0o600")
        flag = root / DISABLED_FLAG.lstrip("/")
        self.assertTrue(flag.is_file())
        self.assertEqual(self._keygen_calls(tmp), [])

    def test_disable_is_idempotent(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        first = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(first.returncode, 0, first.stderr)
        auth = root / ROOT_AUTH.lstrip("/")
        before = auth.read_bytes()
        second = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(second.returncode, 0, second.stderr)
        self.assertEqual(auth.read_bytes(), before)

    def test_disable_without_auth_still_flags(self):
        # Pre-first-boot: no authorized_keys yet. The flag alone blocks
        # provisioning, so disable succeeds on the sudo gate only.
        tmp, root, scripts, env = self._stage()
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        sudoers.parent.mkdir(parents=True)
        sudoers.write_bytes(_shipped_sudoers_bytes())
        run = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertTrue((root / DISABLED_FLAG.lstrip("/")).is_file())
        self.assertFalse((root / ROOT_AUTH.lstrip("/")).exists())

    def test_disable_fails_loud_without_sudoers(self):
        tmp, root, scripts, env = self._stage()
        run = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("no sudoers file to disable", run.stderr)
        self.assertFalse((root / DISABLED_FLAG.lstrip("/")).exists())

    def test_disable_refuses_non_root(self):
        # The stubbed id reports non-root, so the script must exit before
        # touching anything (no sudoers move, no flag).
        tmp, root, scripts, env = self._stage(id_body=STUB_ID_NONROOT)
        self._write_enabled_state(root)
        run = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("must run as root", run.stderr)
        self.assertTrue((root / MACHINE_SUDOERS.lstrip("/")).is_file())
        self.assertFalse((root / DISABLED_FLAG.lstrip("/")).exists())
        auth = root / ROOT_AUTH.lstrip("/")
        self.assertEqual(auth.read_text(), OPERATOR_KEY + ESCAPE_BLOCK)

    def test_enable_restores_and_provisions(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        disable = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(disable.returncode, 0, disable.stderr)
        # Fresh keypair, not the stripped one: drop the client artifacts
        # so the test proves provisioning rather than the guard.
        ssh_dir = root / MACHINE_SSH.lstrip("/")
        (ssh_dir / "id_ed25519").unlink()
        (ssh_dir / "config").unlink()
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        self.assertEqual(sudoers.read_bytes(), _shipped_sudoers_bytes())
        self.assertEqual(oct(sudoers.stat().st_mode & 0o777), "0o440")
        self.assertFalse((root / (MACHINE_SUDOERS.lstrip("/") + ".disabled")).exists())
        auth = root / ROOT_AUTH.lstrip("/")
        text = auth.read_text()
        self.assertEqual(text.count("# fboss-netadmin-escape\n"), 1)
        self.assertNotIn("OLDPUB", text)
        self.assertIn("ssh-ed25519 OTHERKEY other", text)
        self.assertIn("ssh-ed25519 STUBPUB fboss-netadmin-escape", text)
        self.assertFalse((root / DISABLED_FLAG.lstrip("/")).exists())
        self.assertEqual(len(self._keygen_calls(tmp)), 1)

    def test_enable_is_idempotent(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        auth = root / ROOT_AUTH.lstrip("/")
        self.assertEqual(auth.read_text(), OPERATOR_KEY + ESCAPE_BLOCK)
        # Trust already complete: the keygen guard hits, no fresh pair.
        self.assertEqual(self._keygen_calls(tmp), [])

    def test_enable_fails_loud_without_sudoers(self):
        tmp, root, scripts, env = self._stage()
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("no sudoers file to restore", run.stderr)

    def test_enable_fails_loud_without_keygen(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        (root / "usr/local/sbin/escape-keygen").unlink()
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("missing", run.stderr)

    def test_enable_refuses_non_root(self):
        tmp, root, scripts, env = self._stage(id_body=STUB_ID_NONROOT)
        self._write_enabled_state(root)
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("must run as root", run.stderr)
        self.assertEqual(self._keygen_calls(tmp), [])

    def test_round_trip_keeps_single_block(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        for _ in range(2):
            disable = self._run(scripts / "disable_netadmin_root", [str(root)], env)
            self.assertEqual(disable.returncode, 0, disable.stderr)
            enable = self._run(scripts / "enable_netadmin_root", [str(root)], env)
            self.assertEqual(enable.returncode, 0, enable.stderr)
        auth = root / ROOT_AUTH.lstrip("/")
        text = auth.read_text()
        self.assertEqual(text.count("# fboss-netadmin-escape\n"), 1)
        self.assertIn("ssh-ed25519 OTHERKEY other", text)
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        self.assertEqual(sudoers.read_bytes(), _shipped_sudoers_bytes())

    def test_netops_never_touched(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        disable = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(disable.returncode, 0, disable.stderr)
        enable = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertEqual(enable.returncode, 0, enable.stderr)
        self.assertFalse((root / "var/lib/machines/netops").exists())
        self.assertNotIn("netops", _script_bytes("enable_netadmin_root").decode())
        self.assertNotIn("netops", _script_bytes("disable_netadmin_root").decode())

    def test_fixed_paths(self):
        for name in ("enable_netadmin_root", "disable_netadmin_root"):
            script = _script_bytes(name).decode()
            self.assertIn(DISABLED_FLAG, script)
            self.assertIn(MACHINE_SUDOERS, script)
        # Enable delegates authorized_keys to escape-keygen; only disable
        # strips it directly.
        self.assertIn(ROOT_AUTH, _script_bytes("disable_netadmin_root").decode())
        self.assertNotIn(ROOT_AUTH, _script_bytes("enable_netadmin_root").decode())
        self.assertIn(KEYGEN_PATH, _script_bytes("enable_netadmin_root").decode())

    def test_disable_writes_auth_atomically(self):
        script = _script_bytes("disable_netadmin_root").decode()
        self.assertIn('mktemp "$ROOT/root/.ssh/.authorized_keys.', script)
        self.assertIn('mv "$auth_tmp" "$AUTH"', script)
        self.assertNotIn('>> "$AUTH"', script)

    def test_disable_with_only_escape_lines_empties_auth(self):
        # Every line stripped: grep exits 1, which the script accepts via
        # `test $? -eq 1` instead of failing under set -e.
        tmp, root, scripts, env = self._stage()
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        sudoers.parent.mkdir(parents=True)
        sudoers.write_bytes(_shipped_sudoers_bytes())
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text(ESCAPE_BLOCK)
        run = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual(auth.read_text(), "")

    def test_enable_checks_restored_sudoers_syntax(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        disable = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(disable.returncode, 0, disable.stderr)
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        sudoers = root / MACHINE_SUDOERS.lstrip("/")
        visudo_calls = [line for line in self._log(tmp) if line.startswith("visudo ")]
        self.assertEqual(visudo_calls, ["visudo -c -f %s" % sudoers])

    def test_enable_fails_loud_on_bad_sudoers_syntax(self):
        tmp, root, scripts, env = self._stage(visudo_body=STUB_VISUDO_FAIL)
        self._write_enabled_state(root)
        disable = self._run(scripts / "disable_netadmin_root", [str(root)], env)
        self.assertEqual(disable.returncode, 0, disable.stderr)
        run = self._run(scripts / "enable_netadmin_root", [str(root)], env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("bad sudoers syntax", run.stderr)
        # Flag still set and no fresh key: the failed enable leaves the
        # disabled state (and its reboot persistence) intact.
        self.assertTrue((root / DISABLED_FLAG.lstrip("/")).is_file())
        self.assertEqual(self._keygen_calls(tmp), [])

    def test_root_arg_trailing_slash(self):
        tmp, root, scripts, env = self._stage()
        self._write_enabled_state(root)
        run = self._run(scripts / "disable_netadmin_root", [str(root) + "/"], env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertTrue((root / DISABLED_FLAG.lstrip("/")).is_file())
