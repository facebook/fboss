# FBOSS Distro Lite — fake image definition

Builds a fake-SAI FBOSS Distro image through the standard vendor-neutral
pipeline: fake is one vendor row (a manifest), and its identity files ride a
`fake-ident` RPM built from `../components/fake/payload/` via the manifest's `other_dependencies`
`execute` entry — the same channel vendors use for their files. No builder
changes, no hooks, no generated blobs.

This directory mirrors publicly, except `scripts/facebook/`, which stays
internal: ShipIt ignores `facebook/` directories at any depth by default.
Keep the mirrored remainder free of internal endpoints, hostnames,
credentials, and non-public paths.

## Contents

| Repo source | Destination in the image | Via |
|---|---|---|
| `../manifests/6.11.1_fake.json` | (build input only) | manifest |
| `../components/fake/rpm/fake-ident.spec`, `../components/fake/rpm/build_fake_ident.sh` | (build input only) | `execute` |
| `../components/fake/payload/var/facebook/fboss/fruid.json` | `/var/facebook/fboss/fruid.json` | RPM |
| `../components/fake/payload/etc/coop/agent.conf` | `/etc/coop/agent.conf` | RPM |
| `../components/fake/payload/etc/fboss/fake_platform_manager.json` | `/etc/fboss/fake_platform_manager.json` | RPM |
| `../components/fake/payload/etc/systemd/system/*.service.d/fake.conf` | same paths | RPM |
| `%post` in the spec | `/usr/local/fake_bsp/<kver>/kmods.json` | RPM (`<kver>` known only at install time) |
| `../components/login-containers/rpm/login-containers.spec`, `../components/login-containers/rpm/build_login_containers.sh` | (build input only) | `execute` |
| `../components/login-containers/payload/var/lib/machines/{netops,netadmin}/` | same paths | RPM (machine rootfs skeleton; `/usr` + `/opt` bind-mounted ro from host at boot) |
| `../components/login-containers/payload/etc/systemd/nspawn/*.nspawn` | same paths | RPM (same-uid, shared net, ro binds) |
| `../components/login-containers/payload/usr/lib/systemd/system/*-machine.service` | same paths | RPM (enabled in `%post`) |
| `../components/login-containers/payload/usr/local/bin/*-shell` | same paths | RPM (login shells; assigned in the execute hook; self-escalate, resolve the leader, `nsenter` all namespaces as the role user) |
| `../components/login-containers/payload/usr/local/bin/role-entry` | same path | RPM (container-side cert-login entry; refuses host; sets `LOGIN_IDENTITY`) |
| `../components/login-containers/payload/etc/sudoers.d/login-containers` | same path | RPM (each account may run exactly its attach) |
| `../manifests/fake_after_pkgs_*.json` | (build input only, deleted after use) | manifest `image_build_hooks` (users + `systemd-container` + shell assignment) |

## Login machines (fixed at build, recorded here)

The `login-containers` RPM is a shared component (sibling of `fake/`);
only the fake manifest wires it until proven, then other manifests plug in.

- Machine IDs (unique per machine, distinct from the host's; duplicates
  would conflate D-Bus and journal entries):
  - netops: `5e41703e0d69408593dfdb07116e1420`
  - netadmin: `55b4fba27bb44704964fe7b48872be31`
- Machine users mirror host uids/gids (`netops` 1000:1000, `netadmin`
  1001:1002); machine passwords locked — entry only via the login wrapper
  (`nsenter` as the role user).
- Machines boot guest systemd from shared `/usr` into the minimal
  `login-shell.target` (journald + tmpfiles + dbus-broker).
- Attach is `nsenter`-based, not `machinectl shell`: the latter's PTY
  forwarding fails here (container unit cannot chown the slave). Units are
  wanted directly by `multi-user.target` (`machines.target` is pulled by
  nothing); machine `sbin` is a link to `usr/sbin`; `dbus.service` is an
  alias to `dbus-broker.service`, mirroring the host.
- Cert identity: issuance embeds
  `-O force-command='/usr/local/bin/role-entry ROLE USER'` (absolute:
  inner `PATH` lacks `/usr/local/bin`); `sshd` runs it through the
  role shell, which `nsenter`s, so `role-entry`
  always lands inside. It asserts the container (`systemd-detect-virt`)
  and the role (`whoami`), exports `LOGIN_IDENTITY`, logs one journal
  line (tag `role-entry`), prints one banner line when stdin is not a TTY,
  and execs `bash`. Banner/record format (frozen for collectors):
  `login-context role=<role> identity=<user> user=<role> machine-id=<mid>`.

`agent.conf` is generated once from the in-repo sim seed and committed as
plain JSON. Regenerate with:

```bash
jq '.defaultCommandLineArgs.multi_switch = "true" | .platform = {}' \
  fboss/github/fboss-sim/docker/runtime/mono.conf > fboss/github/fboss-image/components/fake/payload/etc/coop/agent.conf
```

The `m4062nhp` platform defaults do not boot under fake SAI, which is why the
seed exists. Stock `fboss_init.sh` runs unmodified: it skips files that
already exist, so baked files survive first boot and reboots.

## Build

```bash
python3 fboss/facebook/scripts/fetch_distro_artifacts.py \
  fboss/github/fboss-image/manifests/6.11.1_fake.json --dest <artifacts>
ARTIFACT_BASE=file:<artifacts> sudo -E python3 \
  fboss/github/fboss-image/distro_cli/fboss-image build \
  fboss/github/fboss-image/manifests/6.11.1_fake.json --output-dir <images>
```

Keep `--output-dir` off EdenFS. The RPM's `%post` verifies every file and
fails the build rather than ship an image that breaks at `fboss_init`.

## Disk use

The builder keeps a content-addressed download cache at
`fboss-image/distro_cli/.artifacts` (~15G observed). It is regenerable
and safe to `rm -rf`; the next build re-downloads.

## Identity

The VM claims `m4062nhp` (`-smbios type=1,product=M4062NHP`) so `fboss_init`
finds a full default config; `platformName` in `fake_platform_manager.json`
must equal the same value. The `SMBus I801` I/O address varies between hosts
and guests; if `platform_manager` fails resolving the bus, read the actual
name from `cat /sys/bus/i2c/devices/i2c-*/name` in the VM and update both
occurrences.

`qsfp_service` and `led_service` both reach `Started` on the fake VM: the
serial log shows `Started FBOSS QSFP Service` and `Started FBOSS LED
Service` with zero `fail` matches, and the health check asserts
`qsfp_service` green alongside the sw/hw agents, fsdb, platform, sensor,
fan, and data-corral services plus TCP 2223.
