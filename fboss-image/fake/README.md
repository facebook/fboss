# FBOSS Distro Lite — fake image definition

Builds a fake-SAI FBOSS Distro image through the standard vendor-neutral
pipeline: fake is one vendor row (a manifest), and its identity files ride a
`fake-ident` RPM built from `payload/` via the manifest's `other_dependencies`
`execute` entry — the same channel vendors use for their files. No builder
changes, no hooks, no generated blobs.

This directory mirrors publicly, except `scripts/facebook/`, which stays
internal: ShipIt ignores `facebook/` directories at any depth by default.
Keep the mirrored remainder free of internal endpoints, hostnames,
credentials, and non-public paths.

## Contents

| Repo source | Destination in the image | Via |
|---|---|---|
| `fboss-lite.json` | (build input only) | manifest |
| `rpm/fake-ident.spec`, `rpm/build_fake_ident.sh` | (build input only) | `execute` |
| `payload/var/facebook/fboss/fruid.json` | `/var/facebook/fboss/fruid.json` | RPM |
| `payload/etc/coop/agent.conf` | `/etc/coop/agent.conf` | RPM |
| `payload/etc/fboss/fake_platform_manager.json` | `/etc/fboss/fake_platform_manager.json` | RPM |
| `payload/etc/systemd/system/*.service.d/fake.conf` | same paths | RPM |
| `%post` in the spec | `/usr/local/fake_bsp/<kver>/kmods.json` | RPM (`<kver>` known only at install time) |

`agent.conf` is generated once from the in-repo sim seed and committed as
plain JSON. Regenerate with:

```bash
jq '.defaultCommandLineArgs.multi_switch = "true"' \
  fboss/github/fboss-sim/docker/runtime/mono.conf > payload/etc/coop/agent.conf
```

The `m4062nhp` platform defaults do not boot under fake SAI, which is why the
seed exists. Stock `fboss_init.sh` runs unmodified: it skips files that
already exist, so baked files survive first boot and reboots.

## Build

```bash
python3 fboss/facebook/scripts/fetch_distro_artifacts.py \
  fboss/github/fboss-image/fake/fboss-lite.json --dest <artifacts>
ARTIFACT_BASE=file:<artifacts> sudo -E python3 \
  fboss/github/fboss-image/distro_cli/fboss-image build \
  fboss/github/fboss-image/fake/fboss-lite.json --output-dir <images>
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
