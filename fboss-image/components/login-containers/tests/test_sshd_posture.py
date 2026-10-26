"""Pin the login-containers sshd posture against drift.

Asserts the effective directives of the shipped sshd_config.d drop-ins:
the localhost root-auth scoping and the dead-client liveness probes.
Comments and blank lines are ignored, so documenting the files never
breaks the pin; any directive change still fails it. Read straight
from the payload; the RPM %post presence check proves they ship, this
proves what they say.
"""

import unittest
from pathlib import Path

DROPINS = Path(__file__).resolve().parents[1] / "payload/etc/ssh/sshd_config.d"


def _directives(name):
    """Non-comment, non-blank lines of a drop-in, order-preserved."""
    lines = []
    for raw in (DROPINS / name).read_text().splitlines():
        line = raw.strip()
        if line and not line.startswith("#"):
            lines.append(line)
    return lines


class SshdPostureTest(unittest.TestCase):
    def test_escape_stays_localhost_scoped(self):
        self.assertEqual(
            _directives("12-escape-localhost.conf"),
            [
                "Match User root Address *,!127.0.0.1,!::1",
                "PubkeyAuthentication no",
                "PasswordAuthentication no",
                "KbdInteractiveAuthentication no",
                "Match all",
            ],
        )

    def test_dead_clients_are_reaped(self):
        self.assertEqual(
            _directives("13-client-alive.conf"),
            ["ClientAliveInterval 60", "ClientAliveCountMax 3"],
        )
