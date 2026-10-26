"""Focused test for the first-boot escape-key provisioner.

Runs payload/usr/local/sbin/escape-keygen black-box against a temp ROOT
with stubbed ssh-keygen/chown on PATH, asserting the provisioned
artifacts (modes, owners, from= scoping), marker idempotency (re-runs
never regenerate), preservation of pre-existing authorized_keys lines,
loud failure when keygen fails, and that netops paths are never
touched. Fixed-path posture (machine .ssh dir, unit ExecStart) is
asserted separately against the committed files.
"""

import subprocess
import tempfile
import unittest
from pathlib import Path

SCRIPT_NAME = "escape-keygen"
MACHINE_SSH = "/var/lib/machines/netadmin/home/netadmin/.ssh"
ROOT_AUTH = "/root/.ssh/authorized_keys"
UNIT_EXEC = "ExecStart=/usr/local/sbin/escape-keygen"


def _payload_dir():
    return Path(__file__).resolve().parents[1] / "payload"


def _script_bytes(name=SCRIPT_NAME):
    try:
        from importlib.resources import files

        package = __package__ or "login_containers.tests"
        return (files(package) / name).read_bytes()
    except (FileNotFoundError, ModuleNotFoundError):
        return (_payload_dir() / "usr/local/sbin" / name).read_bytes()


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


class EscapeKeygenTest(unittest.TestCase):
    def _stage(self, keygen_body=STUB_KEYGEN):
        tmp = Path(tempfile.mkdtemp(prefix="escape-keygen-test-"))
        self.addCleanup(lambda: subprocess.run(["rm", "-rf", str(tmp)], check=False))
        root = tmp / "root"
        stubs = tmp / "stubs"
        stubs.mkdir()
        (stubs / "ssh-keygen").write_text(keygen_body)
        (stubs / "ssh-keygen").chmod(0o755)
        (stubs / "chown").write_text(STUB_CHOWN)
        (stubs / "chown").chmod(0o755)
        script = tmp / SCRIPT_NAME
        script.write_bytes(_script_bytes())
        script.chmod(0o755)
        env = {
            "PATH": "%s:/usr/bin:/bin" % stubs,
            "STUB_LOG": str(tmp / "stub.log"),
        }
        return tmp, root, script, env

    def _run(self, script, root, env):
        return subprocess.run(
            [str(script), str(root)],
            env=env,
            capture_output=True,
            text=True,
            timeout=30,
        )

    def _log(self, tmp):
        log = tmp / "stub.log"
        return log.read_text().splitlines() if log.exists() else []

    def test_fresh_run_provisions_all_artifacts(self):
        tmp, root, script, env = self._stage()
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        priv = root / MACHINE_SSH.lstrip("/") / "id_ed25519"
        conf = root / MACHINE_SSH.lstrip("/") / "config"
        auth = root / ROOT_AUTH.lstrip("/")
        self.assertEqual(priv.read_text(), "PRIVATE-STUB\n")
        self.assertEqual(oct(priv.stat().st_mode & 0o777), "0o600")
        self.assertEqual(
            conf.read_text(),
            "Host localhost 127.0.0.1 ::1\n"
            "    StrictHostKeyChecking accept-new\n"
            "    PasswordAuthentication no\n",
        )
        self.assertEqual(oct(conf.stat().st_mode & 0o777), "0o600")
        self.assertEqual(
            auth.read_text(),
            "# fboss-netadmin-escape\n"
            'from="127.0.0.1,::1" '
            "ssh-ed25519 STUBPUB fboss-netadmin-escape\n",
        )
        self.assertEqual(oct(auth.stat().st_mode & 0o777), "0o600")
        log = self._log(tmp)
        called = [line for line in log if line.startswith("called ")]
        self.assertEqual(len(called), 1)
        # -N '' renders as a vanished arg (double space) in "$*".
        self.assertTrue(
            called[0].startswith("called -t ed25519 -N  -C fboss-netadmin-escape -f "),
            called[0],
        )
        self.assertTrue(called[0].endswith(" -q"), called[0])
        chowns = sorted(line for line in log if line.startswith("chown "))
        ssh_dir = root / MACHINE_SSH.lstrip("/")
        self.assertEqual(oct(ssh_dir.stat().st_mode & 0o777), "0o700")
        self.assertEqual(
            chowns,
            sorted(
                [
                    # The dir itself: without this netadmin cannot
                    # traverse .ssh and the escape fails (live-proven).
                    "chown 1001:1002 %s" % ssh_dir,
                    # Ownership lands on the staged files; mv preserves
                    # it into place (stage-then-rename atomicity).
                    "chown 1001:1002 %s" % (ssh_dir / ".id_ed25519.tmp"),
                    "chown 1001:1002 %s" % (ssh_dir / ".config.tmp"),
                ]
            ),
        )

    def test_rerun_is_idempotent(self):
        tmp, root, script, env = self._stage()
        first = self._run(script, root, env)
        self.assertEqual(first.returncode, 0, first.stderr)
        auth = root / ROOT_AUTH.lstrip("/")
        before = auth.read_bytes()
        second = self._run(script, root, env)
        self.assertEqual(second.returncode, 0, second.stderr)
        self.assertEqual(auth.read_bytes(), before)
        log = self._log(tmp)
        called = [line for line in log if line.startswith("called ")]
        self.assertEqual(len(called), 1)

    def test_existing_marker_skips_generation(self):
        tmp, root, script, env = self._stage()
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text("ssh-ed25519 OTHERKEY other\n# fboss-netadmin-escape\n")
        ssh_dir = root / MACHINE_SSH.lstrip("/")
        ssh_dir.mkdir(parents=True)
        (ssh_dir / "id_ed25519").write_text("PRIVATE-STUB\n")
        (ssh_dir / "config").write_text("Host localhost 127.0.0.1 ::1\n")
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual(
            auth.read_text(),
            "ssh-ed25519 OTHERKEY other\n# fboss-netadmin-escape\n",
        )
        self.assertEqual(self._log(tmp), [])

    def test_missing_privkey_reprovisions_despite_marker(self):
        tmp, root, script, env = self._stage()
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text(
            "ssh-ed25519 OTHERKEY other\n"
            "# fboss-netadmin-escape\n"
            'from="127.0.0.1,::1" ssh-ed25519 OLDPUB fboss-netadmin-escape\n'
        )
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        priv = root / MACHINE_SSH.lstrip("/") / "id_ed25519"
        self.assertEqual(priv.read_text(), "PRIVATE-STUB\n")
        log = self._log(tmp)
        called = [line for line in log if line.startswith("called ")]
        self.assertEqual(len(called), 1)
        # Operator keys preserved; the stale escape block is replaced,
        # never duplicated: no orphaned root pubkey survives.
        text = auth.read_text()
        self.assertEqual(text.count("# fboss-netadmin-escape\n"), 1)
        self.assertNotIn("OLDPUB", text)
        self.assertIn("ssh-ed25519 OTHERKEY other", text)
        self.assertIn("ssh-ed25519 STUBPUB fboss-netadmin-escape", text)

    def test_existing_keys_preserved_on_first_provision(self):
        tmp, root, script, env = self._stage()
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text("ssh-ed25519 OTHERKEY other\n")
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual(
            auth.read_text(),
            "ssh-ed25519 OTHERKEY other\n"
            "# fboss-netadmin-escape\n"
            'from="127.0.0.1,::1" '
            "ssh-ed25519 STUBPUB fboss-netadmin-escape\n",
        )

    def test_keygen_failure_fails_loud(self):
        tmp, root, script, env = self._stage("#!/bin/bash\necho boom >&2\nexit 1\n")
        run = self._run(script, root, env)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("boom", run.stderr)
        self.assertFalse((root / ROOT_AUTH.lstrip("/")).exists())
        self.assertFalse((root / MACHINE_SSH.lstrip("/")).exists())

    def test_netops_never_touched(self):
        tmp, root, script, env = self._stage()
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertFalse((root / "var/lib/machines/netops").exists())
        self.assertNotIn("netops", _script_bytes().decode())

    def test_machine_path_and_unit_exec_are_fixed(self):
        self.assertIn(MACHINE_SSH, _script_bytes().decode())
        self.assertIn(ROOT_AUTH, _script_bytes().decode())
        unit = (
            _payload_dir() / "usr/lib/systemd/system/escape-keygen.service"
        ).read_text()
        self.assertIn(UNIT_EXEC, unit.splitlines())

    def test_authorized_keys_written_atomically(self):
        script = _script_bytes().decode()
        # Staged next to the target (same directory, so the rename is
        # atomic), then moved into place; never appended to directly,
        # so a crash can never leave a marker without its key.
        self.assertIn('mktemp "$ROOT/root/.ssh/.authorized_keys.', script)
        self.assertIn('mv "$auth_tmp" "$AUTH"', script)
        self.assertNotIn('>> "$AUTH"', script)
        # Privkey and config land the same way (stage-then-rename), so a
        # torn write can never satisfy the existence guard and block a
        # re-provisioning boot.
        self.assertIn('mv "$SSH_DIR/.id_ed25519.tmp" "$SSH_DIR/id_ed25519"', script)
        self.assertIn('mv "$SSH_DIR/.config.tmp" "$SSH_DIR/config"', script)
        self.assertNotIn('> "$SSH_DIR/config"', script)

    def test_exit_trap_follows_rc_pattern(self):
        script = _script_bytes().decode()
        # rc captured first, cleanup under set +e, real status out:
        # a failing cleanup can neither lose the exit status nor skip
        # the remaining cleanup. Trap precedes mktemp so an interrupt
        # in the gap cannot leak the temp dir.
        trap = "trap 'rc=$?; set +e;"
        self.assertIn(trap, script)
        self.assertIn("; exit $rc' EXIT", script)
        self.assertLess(script.index(trap), script.index('tmp="$(mktemp -d)"'))
        # Signal traps route HUP/INT/QUIT/PIPE/TERM through exit 128+N
        # so cleanup runs with the real signal cause preserved.
        self.assertIn(
            "for sig in 1 2 3 13 15; do eval \"trap 'exit $((sig + 128))' $sig\"; done",
            script,
        )

    def test_upgrade_strips_legacy_baked_key(self):
        tmp, root, script, env = self._stage()
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text(
            "ssh-ed25519 OTHERKEY other\n"
            'from="127.0.0.1,::1" ssh-ed25519 BAKEDPUB fake-netadmin-escape\n'
        )
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        # No marker (pre-keygen image) triggers provisioning; the legacy
        # shared key is replaced by the fresh per-device block.
        text = auth.read_text()
        self.assertEqual(text.count("# fboss-netadmin-escape\n"), 1)
        self.assertNotIn("BAKEDPUB", text)
        self.assertNotIn("fake-netadmin-escape", text)
        self.assertIn("ssh-ed25519 OTHERKEY other", text)
        self.assertIn("ssh-ed25519 STUBPUB fboss-netadmin-escape", text)

    def test_no_staging_residue_after_run(self):
        tmp, root, script, env = self._stage()
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        leftovers = list((root / "root/.ssh").glob(".authorized_keys.*"))
        self.assertEqual(leftovers, [])

    def test_disabled_flag_strips_trust_and_skips_keygen(self):
        tmp, root, script, env = self._stage()
        flag = root / "etc/login-containers/netadmin-root-disabled"
        flag.parent.mkdir(parents=True)
        flag.write_text("disabled by disable_netadmin_root\n")
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text(
            "ssh-ed25519 OTHERKEY other\n"
            "# fboss-netadmin-escape\n"
            'from="127.0.0.1,::1" ssh-ed25519 OLDPUB fboss-netadmin-escape\n'
        )
        ssh_dir = root / MACHINE_SSH.lstrip("/")
        ssh_dir.mkdir(parents=True)
        (ssh_dir / "id_ed25519").write_text("PRIVATE-STUB\n")
        (ssh_dir / "config").write_text("Host localhost 127.0.0.1 ::1\n")
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        # Complete artifacts would satisfy the guard; the flag wins over
        # it: trust stripped, operator keys kept, nothing provisioned.
        self.assertEqual(auth.read_text(), "ssh-ed25519 OTHERKEY other\n")
        self.assertEqual(self._log(tmp), [])

    def test_disabled_flag_without_auth_provisions_nothing(self):
        tmp, root, script, env = self._stage()
        flag = root / "etc/login-containers/netadmin-root-disabled"
        flag.parent.mkdir(parents=True)
        flag.write_text("disabled by disable_netadmin_root\n")
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertFalse((root / ROOT_AUTH.lstrip("/")).exists())
        self.assertFalse((root / MACHINE_SSH.lstrip("/")).exists())
        self.assertEqual(self._log(tmp), [])

    def test_disabled_flag_with_only_escape_lines_empties_auth(self):
        tmp, root, script, env = self._stage()
        flag = root / "etc/login-containers/netadmin-root-disabled"
        flag.parent.mkdir(parents=True)
        flag.write_text("disabled by disable_netadmin_root\n")
        auth = root / ROOT_AUTH.lstrip("/")
        auth.parent.mkdir(parents=True)
        auth.write_text(
            "# fboss-netadmin-escape\n"
            'from="127.0.0.1,::1" ssh-ed25519 OLDPUB fboss-netadmin-escape\n'
        )
        run = self._run(script, root, env)
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual(auth.read_text(), "")
        self.assertEqual(self._log(tmp), [])
