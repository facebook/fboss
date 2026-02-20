# FSDB BGP RIB Test Publisher

A test publisher utility that writes BGP RIB data to FSDB. Used for testing FSDB subscribers (like HostReachTracker) that consume BGP RIB updates and handle both route announcements and **explicit withdrawals** for the Multiplanar NSF (FPF) project.

## Build

```bash
buck2 build fbcode//fboss/fsdb/tests/utils/bgp_rib_test_publisher:bgp_rib_test_publisher
```

## Usage

```bash
bgp_rib_test_publisher \
  --routes="2001:db8:1::/48,2001:db8:2::/48" \
  --next-hops="2001:db8:ffff::1,2001:db8:ffff::2" &
PID=$!
```
### Multi-plane tests
Multiple instances of the publisher can be run in parallel to simulate multiple planes. This also requires starting multiple instances of fsdb, 1 per plane. The following example shows how to start 2 planes with 1 test publisher each.
```bash
# Plane 1 (uses default fsdb service running on port 5908)
bgp_rib_test_publisher \
  --routes="2001:db8:1::/48,2001:db8:2::/48" \
  --next-hops="2001:db8:ffff::1,2001:db8:ffff::2" &
# with default fsdb service, fboss2 CLI can be used for validation. E.g.
# fboss2 show fsdb state bgp/ribMap

# Plane 2 (uses separate fsdb instance to simulate plane 2)
# start fsdb listening on unused ports 5911, 5912, 5913
fsdb --readConfigFile=false --thrift_ssl_policy=permitted --streamExpire_ms=0 --fsdbPort=5911 --migrated_fsdbPort=5912 --fsdbPort_high_priority=5913 &
# start publisher using plane 2's fsdb instance
bgp_rib_test_publisher \
  --routes="2001:db8:1::/48,2001:db8:2::/48" \
  --next-hops="2001:db8:ffff::1,2001:db8:ffff::2" \
  --fsdbPort=5911 &

# for non-default fsdb instance, use test_client to query ribMap published by publisher
# and specify fsdbPort for the fsdb instance
buck2 build fbcode//fboss/fsdb/oper:test_client
# either run test_client on the test device, or specify host-IP. E.g.
buck2 run //fboss/fsdb/oper:test_client --host=2401:db00:2066:3083::f --fsdbPort=5911 get bgp/ribMap
```

### Flags

| Flag | Description | Required |
|------|-------------|----------|
| `--routes` | Comma-separated list of prefixes to publish | Yes |
| `--next-hops` | Comma-separated list of next hops (1:1 with `--routes`) | Yes |

Both flags are required and must have the same number of entries.

## Signal Handling

Uses `folly::AsyncSignalHandler` with `EventBase::loopForever()` — no polling or sleep loops.

| Signal | Action |
|--------|--------|
| `SIGUSR1` | Withdraw all routes and **exit** |
| `SIGUSR2` | Withdraw all routes but **keep running** |
| `SIGHUP` | **Republish** routes |
| `SIGINT/SIGTERM` | Graceful shutdown (no withdrawal) |

### Example: publish, withdraw, and exit

```bash
bgp_rib_test_publisher \
  --routes="2001:db8:1::/48" \
  --next-hops="2001:db8:ffff::1" &
PID=$!

# Verify routes published
fboss2 show fsdb state bgp/ribMap

# Withdraw and exit
kill -USR1 $PID
```

### Example: withdraw and republish

```bash
bgp_rib_test_publisher \
  --routes="2001:db8:1::/48,2001:db8:2::/48" \
  --next-hops="2001:db8:ffff::1,2001:db8:ffff::2" &
PID=$!

# Withdraw routes but keep running
kill -USR2 $PID

# Republish routes
kill -HUP $PID

# Clean exit
kill -USR1 $PID
```

## Architecture

- **`BgpRibTestPublisher`** — Core library using `FsdbSyncManager` to publish BGP RIB data
- **`main.cpp`** — CLI binary with event-driven signal handling via `folly::AsyncSignalHandler`

Follows the `FsdbSyncer` publisher pattern from the BGP codebase, writing to `BgpData.ribMap` with `TRibEntry` structs.

## Related Documentation

- [Design Doc](https://docs.google.com/document/d/1jzv6AVfonoCdHd36NkOiKMUVOcvMs6yUfCav58pwdbc/)
- FsdbSyncer Reference: `fbcode/neteng/fboss/bgp/cpp/fsdb/FsdbSyncer.cpp`
- FSDB Data Model: `fbcode/fboss/fsdb/if/facebook/fsdb_model.thrift`
