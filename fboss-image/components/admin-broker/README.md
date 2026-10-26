# admin-broker

A small root daemon that answers host-context requests from role
sessions over the local abstract socket `@admin-broker`. Callers prove
their role by being the role: identity comes from the socket peer
credentials joined with the peer's cgroup session scope — no tokens,
no account lookup. Everything is audited. Shipped as the
`admin-broker` RPM with the `host-ctl` client.

## Role matrix

| Verb | netops | netadmin (in container) | netadmin (container root) |
|---|---|---|---|
| `WHOAMI` | yes | yes | yes |
| `PS`, unit `STATUS` | yes | yes | yes |
| `START`, `STOP`, `RESTART` | no | no | yes |
| `EXEC` | allowlisted argv only (list ships empty) | no | yes, any argv |

Reads serve both roles; state changes require container root
(`sudo /bin/bash` in the netadmin machine). Unit verbs additionally
pass a build-time unit→verbs allowlist — one unit per line, either
bare (status only) or with an explicit verb list:

```text
fsdb.service
fboss_sw_agent.service: STATUS, RESTART
fboss_hw_agent@.service: STATUS, RESTART
```

Unlisted units and unreadable files deny everything.

## `host-ctl`

Exec-only client. Sends argv (never a shell string), forwards stdin,
renders stdout, and exits with the remote code:

```bash
host-ctl exec /bin/ls -l /var/log
echo $?   # remote exit code; 137 if the command was killed, 130 on Ctrl-C
```

A refused request prints `host-ctl: broker refused: ERR forbidden`
and exits 1; nothing runs.

Sessions are broker-spawned, each in its own process group. Stdin is
framed, so the broker tells a dead client (closed socket: kill the
whole process group, `SIGTERM` then `SIGKILL` after a grace) from
finished stdin (end trailer: let it run). Signal deaths report the
shell convention on the wire (`128+N`, so `SIGKILL` reads 137).

## Audit

One line per request (`EXEC` logs start and outcome):

```text
2026-10-21T15:00:00+0000 verb=EXEC peer_uid=0 peer_gid=0 peer_pid=4242 role=netadmin session=user-1001.slice/session-9.scope target=/bin/ls result=exit-0 detail=/bin/ls -l
```

Denials log with a blank target — refused command text is never
recorded.
