# Multi-plane Testing

Multiple instances of the publisher can be run in parallel to simulate multiple planes. This also requires starting multiple instances of fsdb, 1 per plane. The following example shows how to start 2 planes with 1 test publisher each.

## Setup

Start the route injector to populate baseline routes:

```bash
./route_injector --routes_file=/home/agrewal/routes.txt &
```

### Routes file

```bash
# Comments start with #
2401:db00:29ff:cf00::1/64=2401:db00:292b:a220:bace::b,2401:db00:292b:a221:bace::b
2401:db00:29ff:cf01::1/64=2401:db00:292b:a228:bace::b,2401:db00:292b:a229:bace::b
```

## Plane 1 (default fsdb on port 5908)

```bash
./bgp_rib_test_publisher \
  --routes=2401:db00:29ff:cf01::/64,2401:db00:29ff:cf00::/64 \
  --next-hops=2401:db00:292b:a228:bace::b,2401:db00:292b:a220:bace::b &
```

Validate published routes:

```bash
buck2 build @mode/opt fbcode//fboss/fsdb/oper:test_client

/home/agrewal/test_client get bgp
```

## Plane 2 (separate fsdb instance)

Start a separate fsdb instance listening on unused ports 5911, 5912, 5913:

```bash
/etc/packages/neteng-fboss-fsdb/current/fsdb \
  --thrift_ssl_policy=permitted \
  --streamExpire_ms=0 \
  --fsdbPort=5911 \
  --migrated_fsdbPort=5912 \
  --fsdbPort_high_priority=5913 &
```

Start the publisher targeting Plane 2's fsdb instance (note the different next-hops):

```bash
./bgp_rib_test_publisher \
  --routes=2401:db00:29ff:cf01::/64,2401:db00:29ff:cf00::/64 \
  --next-hops=2401:db00:292b:a229:bace::b,2401:db00:292b:a221:bace::b \
  --fsdbPort=5911 &
```

Validate published routes on Plane 2 by specifying its fsdb port:

```bash
buck2 build @mode/opt fbcode//fboss/fsdb/oper:test_client
/home/agrewal/test_client --fsdbPort=5911 get bgp
```
