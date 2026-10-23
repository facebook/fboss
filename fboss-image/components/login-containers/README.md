# login-containers

Role login machines plus the pieces that scope host access. Each role
(`netops`, `netadmin`) gets its own systemd-nspawn machine; users land
in their role's machine through a login-shell wrapper and never see a
host shell. Netadmin additionally gets two scoped paths to the host
context (container root shell, localhost ssh to host root); netops
gets neither. Shipped as the `login-containers` RPM.

## Layout

Payload paths below `payload/` install to the same absolute paths:

| Payload | Purpose |
|---|---|
| `var/lib/machines/<role>/` | Machine rootfs skeleton (passwd/group, locked shadow, machine-id, homes) |
| `usr/lib/systemd/system/<role>-machine.service` | Machine units, wanted by `multi-user.target` |
| `usr/lib/systemd/system/escape-keygen.service` | First-boot escape-key provisioner (oneshot) |
| `etc/systemd/nspawn/<role>.nspawn` | Shared net, read-only `/usr`+`/opt`; netadmin adds writable FBOSS binds |
| `usr/local/bin/<role>-shell` | Login-shell wrappers: escalate, mint attribution, enter the machine |
| `usr/local/bin/role-entry` | Container-side cert-login entry (checks, banner, identity export) |
| `usr/local/bin/session-recorder`, `session-ancestors` | PAM open/close hook and ancestor-chain helper |
| `usr/local/sbin/escape-keygen` | Generates the per-device escape keypair at first boot |
| `etc/sudoers.d/login-containers` | Each role may run exactly its own wrapper as root |
| `var/lib/machines/netadmin/etc/{sudoers,pam.d/sudo,sysusers.d/}` | Container-local netadmin sudo (root shell grant) |
| `etc/ssh/sshd_config.d/12-escape-localhost.conf` | Root auth denied off loopback |
| `etc/ssh/sshd_config.d/13-client-alive.conf` | Dead-client session reaping |

The RPM's `%post` fails the install unless all units are enabled and
every login-path file is present — it refuses to ship a dead-login
image.

## Role workflows

**Netadmin.** Log in (password or certificate) and land in the
netadmin machine as `netadmin`. From there:

- `sudo /bin/bash` — container-root shell (host root with a
  container-shaped filesystem view).
- `ssh root@localhost` — interactive host-context root shell. Works
  with no password or flags: the machine home holds a private key
  whose public half is scoped to loopback in host root's
  `authorized_keys`.
- `host-ctl exec …` (as container root) — scripted host-context
  commands via admin-broker; see the admin-broker README.

**Netops.** Log in and land in the netops machine as `netops`, with
read access to the shared mounts. There is no sudo grant, no escape
key, and no path to host root: `sudo` fails, `ssh root@localhost`
is denied, and the broker refuses privileged verbs.

## First-boot key provisioning

No key is baked into the image. At first boot `escape-keygen.service`
runs before the netadmin machine and sshd, generates a per-device
ed25519 pair, installs the private key into the netadmin machine home
only, and appends the loopback-scoped public key to host root's
`authorized_keys`. Every artifact lands atomically, and re-runs exit
0 only when all of them are present; a wiped home re-provisions a
fresh pair (prior escape blocks are stripped first), while operator
`authorized_keys` lines are always preserved.
