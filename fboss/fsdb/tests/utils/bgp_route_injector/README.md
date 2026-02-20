# FBOSS Route Injector

Injects routes into FBOSS Agent via Thrift API with ECMP support (multiple next-hops per prefix).

## Build

```bash
buck2 build @mode/opt fbcode//fboss/fsdb/tests/utils/bgp_route_injector:route_injector
```

## Usage

```bash
echo "10.0.0.0/24=10.1.0.1,10.1.0.2" > /tmp/routes.txt
route_injector --routes_file=/tmp/routes.txt &
PID=$!
```

### Flags

| Flag | Description | Default |
|------|-------------|---------|
| `--routes_file` | Path to routes file | Required |
| `--agent_host` | FBOSS agent host | 127.0.0.1 |
| `--agent_port` | FBOSS agent port | 5909 |

## Routes File Format

```
# Comments start with #
10.0.0.0/24=10.1.0.1,10.1.0.2
2001:db8::/32=2001:db8::1,2001:db8::2
```

Format: `prefix=nexthop1,nexthop2`

## Signal Handling

| Signal | Action |
|--------|--------|
| `SIGHUP` | Reload routes from file |
| `SIGUSR1` | Delete routes and **exit** |
| `SIGUSR2` | Delete routes but **keep running** |
| `SIGINT/SIGTERM` | Graceful shutdown |

### Example: inject, reload, and exit

```bash
echo "10.0.0.0/24=10.1.0.1,10.1.0.2" > /tmp/routes.txt
route_injector --routes_file=/tmp/routes.txt &
PID=$!

# Verify routes injected
fboss2 show route details 10.0.0.0/24

# Update routes file and reload
echo "10.0.0.0/24=10.1.0.3,10.1.0.4" > /tmp/routes.txt
kill -HUP $PID

# Delete routes and exit
kill -USR1 $PID
```

### Example: delete and re-inject

```bash
echo "10.0.0.0/24=10.1.0.1,10.1.0.2" > /tmp/routes.txt
route_injector --routes_file=/tmp/routes.txt &
PID=$!

# Delete routes but keep running
kill -USR2 $PID

# Re-inject routes from file
kill -HUP $PID

# Clean exit
kill -USR1 $PID
```
