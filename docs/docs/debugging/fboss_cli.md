---
sidebar_position: 1
---

# FBOSS2 CLI

The FBOSS2 CLI (`fboss2`) is a command-line tool for interacting with and managing FBOSS switches. It provides comprehensive visibility into the switch's operational state, configuration, and various subsystems.

## Overview

FBOSS2 CLI is the primary tool for:
- **Monitoring**: View real-time switch state, port status, and traffic statistics
- **Troubleshooting**: Debug connectivity issues, inspect routing tables, and analyze network behavior
- **Configuration**: Modify switch settings, manage interfaces, and apply policies
- **Operational tasks**: Verify configurations, check hardware health, and validate deployments

### Basic Usage

```bash
# General syntax
fboss2 <command> [subcommand] [options]

# Get help
fboss2 --help
fboss2 <command> --help
```

---

## Command Overview

The FBOSS2 CLI organizes functionality into the following top-level commands:

| Command | Description |
|---------|-------------|
| `show` | Display current state, statistics, and configuration information |
| `clear` | Reset counters, clear tables, and remove temporary state |
| `set` | Modify object properties and settings |
| `get` | Retrieve specific object information |
| `config` | Configuration management commands |
| `create` | Create new objects (routes, ACLs, etc.) |
| `delete` | Remove objects from the switch |
| `bounce` | Disable and re-enable objects (ports, interfaces) |
| `reload` | Reload configuration or state for objects |
| `debug` | Access debugging and diagnostic functions |
| `start` | Start events or processes |
| `stop` | Stop events or processes |
| `stream` | Continuously stream real-time data |
| `help` | Display help information for commands and objects |

### Command Syntax Pattern

Most commands follow a consistent pattern:

```bash
fboss2 <verb> <object> [options]
```

For example:
```bash
fboss2 show port              # Display port information
fboss2 clear arp              # Clear ARP table
fboss2 set port eth1/1/1 ...  # Modify port settings
fboss2 bounce port eth1/1/1   # Bounce a port
```

---

## show

Display current state and statistics. This is the most commonly used command for monitoring and troubleshooting.

Most `show` commands support JSON output for scripting:

```bash
fboss2 show <object> --fmt json
```

### Show Subcommands

| Subcommand | Description |
|------------|-------------|
| `acl` | Show ACL information |
| `agent` | Show Agent state |
| `arp` | Show ARP information |
| `cpuport` | Show CPU port information |
| `host` | Show Host Information |
| `hw-agent` | Show HwAgent Status |
| `hw-object` | Show HW Objects |
| `interface` | Show Interface information |
| `lldp` | Show LLDP information |
| `mac` | Show MAC information |
| `mirror` | Show mirror |
| `ndp` | Show NDP information |
| `port` | Show Port information |
| `rif` | Show RIF information |
| `route` | Show Route information |
| `transceiver` | Show Transceiver information |
| `config` | Show config info for various binaries |
| `fsdb` | Show operational information from fsdb |
| `version` | Show versions info for various binaries |

---

## Show Commands Reference

The following sections provide detailed documentation for each `show` subcommand.

### acl

Show Access Control List (ACL) information configured on the switch.

**Usage:**
```bash
fboss2 show acl
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
ACL Table Name: AclTable1
Acl: cpuPolicing-CPU-Port-Mcast-v6
   priority: 2
   action: permit
   enabled: 1

Acl: cpuPolicing-CPU-Port-linkLocal-v6
   priority: 1
   action: permit
   enabled: 1

Acl: cpuPolicing-high-NetworkControl-ff02::/16
   priority: 4
   dscp: 48
   action: permit
   enabled: 1

Acl: cpuPolicing-high-NetworkControl-linkLocal-v6
   priority: 3
   dscp: 48
   action: permit
   enabled: 1

Acl: cpuPolicing-mid-ff02::/16
   priority: 6
   action: permit
   enabled: 1

Acl: cpuPolicing-mid-linkLocal-v6
   priority: 5
   action: permit
   enabled: 1

Acl: flowlet-selective-enable
   priority: 100006
   proto: 17
   L4 dst port: 4791
   action: permit
   enabled: 1

Acl: random-spray-cancel
   priority: 100007
   action: permit
   enabled: 1

Acl: ttld-interconnect
   priority: 100004
   proto: 6
   ttl: 128
   action: permit
   enabled: 1

Acl: ttld-ip-per-task
   priority: 100002
   proto: 6
   ttl: 128
   action: permit
   enabled: 1

Acl: ttld-onavo-infra
   priority: 100003
   proto: 6
   ttl: 128
   action: permit
   enabled: 1

Acl: ttld-prod-private
   priority: 100000
   proto: 6
   ttl: 128
   action: permit
   enabled: 1

Acl: ttld-prod-public
   priority: 100001
   proto: 6
   ttl: 128
   action: permit
   enabled: 1
```

</div>

---

### agent

Show Agent state information.

**Usage:**
```bash
fboss2 show agent <subcommand>
```

#### Subcommands

| Subcommand | Description |
|------------|-------------|
| `firmware` | Show Agent Firmware information |



### arp

Show ARP (Address Resolution Protocol) table entries.

**Usage:**
```bash
fboss2 show arp
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
IP Address            MAC Address        Interface   VLAN               State         TTL      CLASSID     Voq Switch
```

</div>

*Note: Empty table indicates no ARP entries are currently learned. IPv6-only networks will show entries in the NDP table instead.*

---

### cpuport

Show CPU port information including queue statistics.

**Usage:**
```bash
fboss2 show cpuport
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 Switch ID  CPU Queue ID  Queue Name        Ingress Packets  Discard Packets
-----------------------------------------------------------------------------------
 0          0             cpuQueue-low      24               0
 0          1             cpuQueue-default  0                0
 0          2             cpuQueue-mid      318259           0
 0          9             cpuQueue-high     361922           0
```

</div>

---




### host

Show Host Information including connected hosts and their port associations.

**Usage:**
```bash
fboss2 show host
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 Port       ID   Queue ID  Hostname                          Admin State  Link State  Speed  FEC       InErr  InDiscard  OutErr  OutDiscard
---------------------------------------------------------------------------------------------------------------------------------------------------------
 eth1/34/1  176  Olympic   eth1-7-1.ftsw001.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22953      0       0
 eth1/34/5  180  Olympic   eth1-7-5.ftsw001.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22954      0       0
 eth1/33/1  184  Olympic   eth1-3-1.ftsw001.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22952      0       0
 eth1/33/5  185  Olympic   eth1-3-5.ftsw001.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22952      0       0
 eth1/35/1  187  Olympic   eth1-11-1.ftsw001.l102.c083.ash6  Enabled      Up          400G   RS544_2N  0      22953      0       0
 eth1/35/5  191  Olympic   eth1-11-5.ftsw001.l102.c083.ash6  Enabled      Up          400G   RS544_2N  0      22953      0       0
 eth1/50/1  264  Olympic   eth1-7-1.ftsw002.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22953      0       4
 eth1/50/5  268  Olympic   eth1-7-5.ftsw002.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22952      0       4
 eth1/49/1  272  Olympic   eth1-3-1.ftsw002.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22953      0       4
 eth1/49/5  273  Olympic   eth1-3-5.ftsw002.l102.c083.ash6   Enabled      Up          400G   RS544_2N  0      22953      0       4
 eth1/51/1  275  Olympic   eth1-11-1.ftsw002.l102.c083.ash6  Enabled      Up          400G   RS544_2N  0      22953      0       4
 eth1/51/5  279  Olympic   eth1-11-5.ftsw002.l102.c083.ash6  Enabled      Up          400G   RS544_2N  0      22953      0       4
```

</div>

---

### hw-agent

Show HwAgent Status including synchronization state between software and hardware.

**Usage:**
```bash
fboss2 show hw-agent
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
Output Format: <InSync/OutofSync - [Y]es/[N]o  | No. of Events sent | No. of Events received>
 SwitchIndex  SwitchId  State       Link       Stats            Fdb    RxPkt            TxPkt            SwitchReachability
--------------------------------------------------------------------------------------------------------------------------------------
 0            0         CONFIGURED  Y|129|129  Y|113736|113736  Y|0|0  Y|679171|679169  Y|875241|875243  Y|0|0
```

</div>

---

### hw-object

Show HW Objects programmed in the switch ASIC.

**Usage:**
```bash
fboss2 show hw-object [hw_object_type...]
```

**Arguments:**
- `hw_object_type` - Optional hardware object type(s) to display (e.g., `l2`, `route`)

---

### interface

Show Interface information including status, speed, and addressing.

**Usage:**
```bash
fboss2 show interface
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
| Interface | Status | Speed | VLAN | MTU  | Addresses                      | Description                      |
| eth1/1/1  | up     | 400G  | 2001 | 9000 | 2401:db00:209b::a/64           |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/1/5  | up     | 400G  | 2002 | 9000 | 2401:db00:209b:1::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/2/1  | up     | 400G  | 2003 | 9000 | 2401:db00:209b:2::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/2/5  | up     | 400G  | 2004 | 9000 | 2401:db00:209b:3::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/3/1  | up     | 400G  | 2005 | 9000 | 2401:db00:209b:4::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/3/5  | up     | 400G  | 2006 | 9000 | 2401:db00:209b:5::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/4/1  | down   | 400G  | 2007 | 9000 | 2401:db00:209b:6::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/4/5  | down   | 400G  | 2008 | 9000 | 2401:db00:209b:7::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/5/1  | down   | 400G  | 2009 | 9000 | 2401:db00:209b:8::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/5/5  | down   | 400G  | 2010 | 9000 | 2401:db00:209b:9::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/6/1  | down   | 400G  | 2011 | 9000 | 2401:db00:209b:a::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/6/5  | down   | 400G  | 2012 | 9000 | 2401:db00:209b:b::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/7/1  | down   | 400G  | 2013 | 9000 | 2401:db00:209b:c::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/7/5  | down   | 400G  | 2014 | 9000 | 2401:db00:209b:d::a/64         |                                  |
|           |        |       |      |      | fe80::be:face:b00c/64          |                                  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/33/1 | up     | 400G  | 4001 | 9000 | 2401:db00:e206:b20::1/127      | ftsw001.l102.c083.ash6:eth1/3/1  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/33/5 | up     | 400G  | 4002 | 9000 | 2401:db00:e206:b20::3/127      | ftsw001.l102.c083.ash6:eth1/3/5  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/34/1 | up     | 400G  | 4003 | 9000 | 2401:db00:e206:b20::5/127      | ftsw001.l102.c083.ash6:eth1/7/1  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/34/5 | up     | 400G  | 4004 | 9000 | 2401:db00:e206:b20::7/127      | ftsw001.l102.c083.ash6:eth1/7/5  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/35/1 | up     | 400G  | 4005 | 9000 | 2401:db00:e206:b20::9/127      | ftsw001.l102.c083.ash6:eth1/11/1 |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/35/5 | up     | 400G  | 4006 | 9000 | 2401:db00:e206:b20::b/127      | ftsw001.l102.c083.ash6:eth1/11/5 |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/49/1 | up     | 400G  | 4007 | 9000 | 2401:db00:e206:b20:0:1:0:1/127 | ftsw002.l102.c083.ash6:eth1/3/1  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/49/5 | up     | 400G  | 4008 | 9000 | 2401:db00:e206:b20:0:1:0:3/127 | ftsw002.l102.c083.ash6:eth1/3/5  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/50/1 | up     | 400G  | 4009 | 9000 | 2401:db00:e206:b20:0:1:0:5/127 | ftsw002.l102.c083.ash6:eth1/7/1  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/50/5 | up     | 400G  | 4010 | 9000 | 2401:db00:e206:b20:0:1:0:7/127 | ftsw002.l102.c083.ash6:eth1/7/5  |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/51/1 | up     | 400G  | 4011 | 9000 | 2401:db00:e206:b20:0:1:0:9/127 | ftsw002.l102.c083.ash6:eth1/11/1 |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
| eth1/51/5 | up     | 400G  | 4012 | 9000 | 2401:db00:e206:b20:0:1:0:b/127 | ftsw002.l102.c083.ash6:eth1/11/5 |
|           |        |       |      |      | fe80::b4db:91ff:fe95:fdda/64   |                                  |
```

</div>

---

### mac

Show MAC address information.

**Usage:**
```bash
fboss2 show mac <subcommand>
```

#### Subcommands

| Subcommand | Description |
|------------|-------------|
| `blocked` | Show details of blocked MAC addresses |
| `details` | Show details of the MAC (L2) table |

#### mac details

Show details of the MAC (L2) table.

**Usage:**
```bash
fboss2 show mac details
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
MAC Address             Port/Trunk         VLAN          TYPE               CLASSID
ae:81:b5:03:41:60       eth1/51/5          4012          Validated          -
ae:81:b5:03:41:60       eth1/51/1          4011          Validated          -
ae:81:b5:03:41:60       eth1/50/5          4010          Validated          -
ae:81:b5:03:41:60       eth1/50/1          4009          Validated          -
ae:81:b5:03:41:60       eth1/49/5          4008          Validated          -
ae:81:b5:03:41:60       eth1/49/1          4007          Validated          -
b6:db:91:95:fd:f6       eth1/35/5          4006          Validated          -
b6:db:91:95:fd:f6       eth1/35/1          4005          Validated          -
b6:db:91:95:fd:f6       eth1/34/5          4004          Validated          -
b6:db:91:95:fd:f6       eth1/34/1          4003          Validated          -
b6:db:91:95:fd:f6       eth1/33/5          4002          Validated          -
b6:db:91:95:fd:f6       eth1/33/1          4001          Validated          -
```

</div>

---

### lldp

Show LLDP (Link Layer Discovery Protocol) neighbor information.

**Usage:**
```bash
fboss2 show lldp
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 Local Int  Status  Expected-Peer           LLDP-Peer               Peer-Int   Platform         Peer Description
-----------------------------------------------------------------------------------------------------------------------------------------
 eth1/1/1   up      EMPTY                   ixia08.netcastle.ash6   1/13       KEYSIGHT Device
 eth1/1/5   up      EMPTY                   ixia08.netcastle.ash6   1/14       KEYSIGHT Device
 eth1/2/1   up      EMPTY                   ixia08.netcastle.ash6   1/15       KEYSIGHT Device
 eth1/2/5   up      EMPTY                   ixia08.netcastle.ash6   1/16       KEYSIGHT Device
 eth1/3/1   up      EMPTY                   ixia08.netcastle.ash6   1/17       KEYSIGHT Device
 eth1/3/5   up      EMPTY                   ixia08.netcastle.ash6   1/18       KEYSIGHT Device
 eth1/17/1  up      EMPTY                   ixia08.netcastle.ash6   1/19       KEYSIGHT Device
 eth1/17/5  up      EMPTY                   ixia08.netcastle.ash6   1/20       KEYSIGHT Device
 eth1/18/1  up      EMPTY                   ixia08.netcastle.ash6   1/21       KEYSIGHT Device
 eth1/18/5  up      EMPTY                   ixia08.netcastle.ash6   1/22       KEYSIGHT Device
 eth1/33/1  up      ftsw001.l102.c083.ash6  ftsw001.l102.c083.ash6  eth1/3/1   FBOSS            ftsw001.l102.c083.ash6:eth1/3/1
 eth1/33/5  up      ftsw001.l102.c083.ash6  ftsw001.l102.c083.ash6  eth1/3/5   FBOSS            ftsw001.l102.c083.ash6:eth1/3/5
 eth1/34/1  up      ftsw001.l102.c083.ash6  ftsw001.l102.c083.ash6  eth1/7/1   FBOSS            ftsw001.l102.c083.ash6:eth1/7/1
 eth1/34/5  up      ftsw001.l102.c083.ash6  ftsw001.l102.c083.ash6  eth1/7/5   FBOSS            ftsw001.l102.c083.ash6:eth1/7/5
 eth1/35/1  up      ftsw001.l102.c083.ash6  ftsw001.l102.c083.ash6  eth1/11/1  FBOSS            ftsw001.l102.c083.ash6:eth1/11/1
 eth1/35/5  up      ftsw001.l102.c083.ash6  ftsw001.l102.c083.ash6  eth1/11/5  FBOSS            ftsw001.l102.c083.ash6:eth1/11/5
 eth1/49/1  up      ftsw002.l102.c083.ash6  ftsw002.l102.c083.ash6  eth1/3/1   FBOSS            ftsw002.l102.c083.ash6:eth1/3/1
 eth1/49/5  up      ftsw002.l102.c083.ash6  ftsw002.l102.c083.ash6  eth1/3/5   FBOSS            ftsw002.l102.c083.ash6:eth1/3/5
```

</div>

---

### mirror

Show mirror session configurations for traffic mirroring/SPAN.

**Usage:**
```bash
fboss2 show mirror
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 Mirror                       Status      Egress Port  Egress Port Name  Tunnel Type  Src MAC  Src IP                    Src UDP Port  Dst MAC  Dst IP                 Dst UDP Port  DSCP  TTL
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 fboss_backend_traffic_sflow  Configured  -            -                 -            -        2401:db00:e206:b20::1d:0  12345         -        2401:db00:20ff:f001::  6346          42    -
```

</div>

---

### ndp

Show NDP (Neighbor Discovery Protocol) entries for IPv6.

**Usage:**
```bash
fboss2 show ndp
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 IP Address                  MAC Address        Interface  VLAN/InterfaceID  State      TTL    CLASSID  Voq Switch  Resolved Since
---------------------------------------------------------------------------------------------------------------------------------------------
 fe80::b4db:91ff:fe95:fdf6   b6:db:91:95:fd:f6  eth1/33/1  vlan4001 (4001)   REACHABLE  34745  0        --          --
 2401:db00:e206:b20::        b6:db:91:95:fd:f6  eth1/33/1  vlan4001 (4001)   REACHABLE  50237  0        --          --
 fe80::b4db:91ff:fe95:fdf6   b6:db:91:95:fd:f6  eth1/33/5  vlan4002 (4002)   REACHABLE  830    0        --          --
 2401:db00:e206:b20::2       b6:db:91:95:fd:f6  eth1/33/5  vlan4002 (4002)   REACHABLE  29717  0        --          --
 fe80::b4db:91ff:fe95:fdf6   b6:db:91:95:fd:f6  eth1/34/1  vlan4003 (4003)   REACHABLE  9460   0        --          --
 2401:db00:e206:b20::4       b6:db:91:95:fd:f6  eth1/34/1  vlan4003 (4003)   REACHABLE  38330  0        --          --
 fe80::b4db:91ff:fe95:fdf6   b6:db:91:95:fd:f6  eth1/34/5  vlan4004 (4004)   REACHABLE  24326  0        --          --
 2401:db00:e206:b20::6       b6:db:91:95:fd:f6  eth1/34/5  vlan4004 (4004)   REACHABLE  12632  0        --          --
 fe80::b4db:91ff:fe95:fdf6   b6:db:91:95:fd:f6  eth1/35/1  vlan4005 (4005)   REACHABLE  17635  0        --          --
 2401:db00:e206:b20::8       b6:db:91:95:fd:f6  eth1/35/1  vlan4005 (4005)   REACHABLE  10193  0        --          --
 fe80::b4db:91ff:fe95:fdf6   b6:db:91:95:fd:f6  eth1/35/5  vlan4006 (4006)   REACHABLE  57798  0        --          --
 2401:db00:e206:b20::a       b6:db:91:95:fd:f6  eth1/35/5  vlan4006 (4006)   REACHABLE  54736  0        --          --
 fe80::ac81:b5ff:fe03:4160   ae:81:b5:03:41:60  eth1/49/1  vlan4007 (4007)   REACHABLE  33396  0        --          --
 2401:db00:e206:b20:0:1::    ae:81:b5:03:41:60  eth1/49/1  vlan4007 (4007)   REACHABLE  55632  0        --          --
 fe80::ac81:b5ff:fe03:4160   ae:81:b5:03:41:60  eth1/49/5  vlan4008 (4008)   REACHABLE  35555  0        --          --
 2401:db00:e206:b20:0:1:0:2  ae:81:b5:03:41:60  eth1/49/5  vlan4008 (4008)   REACHABLE  58222  0        --          --
 fe80::ac81:b5ff:fe03:4160   ae:81:b5:03:41:60  eth1/50/1  vlan4009 (4009)   REACHABLE  47999  0        --          --
 2401:db00:e206:b20:0:1:0:4  ae:81:b5:03:41:60  eth1/50/1  vlan4009 (4009)   REACHABLE  51421  0        --          --
 fe80::ac81:b5ff:fe03:4160   ae:81:b5:03:41:60  eth1/50/5  vlan4010 (4010)   REACHABLE  23619  0        --          --
 2401:db00:e206:b20:0:1:0:6  ae:81:b5:03:41:60  eth1/50/5  vlan4010 (4010)   REACHABLE  33850  0        --          --
 fe80::ac81:b5ff:fe03:4160   ae:81:b5:03:41:60  eth1/51/1  vlan4011 (4011)   REACHABLE  70289  0        --          --
 2401:db00:e206:b20:0:1:0:8  ae:81:b5:03:41:60  eth1/51/1  vlan4011 (4011)   REACHABLE  40438  0        --          --
 fe80::ac81:b5ff:fe03:4160   ae:81:b5:03:41:60  eth1/51/5  vlan4012 (4012)   REACHABLE  45742  0        --          --
 2401:db00:e206:b20:0:1:0:a  ae:81:b5:03:41:60  eth1/51/5  vlan4012 (4012)   REACHABLE  16518  0        --          --
```

</div>

---

### port

Show Port information including admin/link state, speed, and profiles.

**Usage:**
```bash
fboss2 show port
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 ID   Name       AdminState  LinkState  ActiveState  Transceiver  TcvrID  Speed  ProfileID                             HwLogicalPortId  Drained  PeerSwitchDrained  PeerPortDrainedOrDown  Errors  Core Id  Virtual device Id  Cable Len meters
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 9    eth1/1/1   Enabled     Up         --           Present      0       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  9                No       --                 --                     --      --       --                 --
 10   eth1/1/5   Enabled     Up         --           Present      0       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  10               No       --                 --                     --      --       --                 --
 1    eth1/2/1   Enabled     Up         --           Present      1       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  1                No       --                 --                     --      --       --                 --
 5    eth1/2/5   Enabled     Up         --           Present      1       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  5                No       --                 --                     --      --       --                 --
 11   eth1/3/1   Enabled     Up         --           Present      2       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  11               No       --                 --                     --      --       --                 --
 15   eth1/3/5   Enabled     Up         --           Present      2       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  15               No       --                 --                     --      --       --                 --
 19   eth1/4/1   Enabled     Down       --           Absent       3       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  19               No       --                 --                     --      --       --                 --
 20   eth1/4/5   Enabled     Down       --           Absent       3       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  20               No       --                 --                     --      --       --                 --
 30   eth1/5/1   Enabled     Down       --           Absent       4       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  30               No       --                 --                     --      --       --                 --
 31   eth1/5/5   Enabled     Down       --           Absent       4       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  31               No       --                 --                     --      --       --                 --
 22   eth1/6/1   Enabled     Down       --           Absent       5       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  22               No       --                 --                     --      --       --                 --
 26   eth1/6/5   Enabled     Down       --           Absent       5       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  26               No       --                 --                     --      --       --                 --
 33   eth1/7/1   Enabled     Down       --           Absent       6       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  33               No       --                 --                     --      --       --                 --
 37   eth1/7/5   Enabled     Down       --           Absent       6       400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  37               No       --                 --                     --      --       --                 --
 96   eth1/17/1  Enabled     Up         --           Present      16      400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  96               No       --                 --                     --      --       --                 --
 97   eth1/17/5  Enabled     Up         --           Present      16      400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  97               No       --                 --                     --      --       --                 --
 88   eth1/18/1  Enabled     Up         --           Present      17      400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  88               No       --                 --                     --      --       --                 --
 92   eth1/18/5  Enabled     Up         --           Present      17      400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  92               No       --                 --                     --      --       --                 --
 99   eth1/19/1  Enabled     Down       --           Present      18      400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  99               No       --                 --                     --      --       --                 --
 103  eth1/19/5  Enabled     Down       --           Present      18      400G   PROFILE_400G_4_PAM4_RS544X2N_OPTICAL  103              No       --                 --                     --      --       --                 --
```

</div>

---


### rif

Show RIF (Router Interface) information.

**Usage:**
```bash
fboss2 show rif
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 RIF             RIFID  VlanID  RouterID  MAC                MTU   TYPE  Liveness  Scope  Ports
---------------------------------------------------------------------------------------------------------------
 Interface 10    10     10      0         b6:db:91:95:fd:da  9000  --    --        LOCAL
 Interface 2001  2001   2001    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/1/1
 Interface 2002  2002   2002    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/1/5
 Interface 2003  2003   2003    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/2/1
 Interface 2004  2004   2004    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/2/5
 Interface 2005  2005   2005    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/3/1
 Interface 2006  2006   2006    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/3/5
 Interface 2007  2007   2007    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/4/1
 Interface 2008  2008   2008    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/4/5
 Interface 2009  2009   2009    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/5/1
 Interface 2010  2010   2010    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/5/5
 Interface 2011  2011   2011    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/6/1
 Interface 2012  2012   2012    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/6/5
 Interface 2013  2013   2013    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/7/1
 Interface 2014  2014   2014    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/7/5
 Interface 2015  2015   2015    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/8/1
 Interface 2016  2016   2016    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/8/5
 Interface 2017  2017   2017    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/9/1
 Interface 2018  2018   2018    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/9/5
 Interface 2019  2019   2019    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/10/1
 Interface 2020  2020   2020    0         b6:db:91:95:fd:da  9000  --    --        LOCAL  eth1/10/5
```

</div>

---

### route

Show Route information from the routing table.

**Usage:**
```bash
fboss2 show route
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
Network Address: 2401:db00:209b:10::/64
    via 2401:db00:209b:10::a dev fboss2017 weight 1
Network Address: 2401:db00:209b:11::/64
    via 2401:db00:209b:11::a dev fboss2018 weight 1
Network Address: 2401:db00:209b:12::/64
    via 2401:db00:209b:12::a dev fboss2019 weight 1
Network Address: 2401:db00:209b:13::/64
    via 2401:db00:209b:13::a dev fboss2020 weight 1
Network Address: 2401:db00:209b:14::/64
    via 2401:db00:209b:14::a dev fboss2021 weight 1
Network Address: 2401:db00:209b:15::/64
    via 2401:db00:209b:15::a dev fboss2022 weight 1
Network Address: 2401:db00:209b:16::/64
    via 2401:db00:209b:16::a dev fboss2023 weight 1
Network Address: 2401:db00:209b:17::/64
    via 2401:db00:209b:17::a dev fboss2024 weight 1
Network Address: 2401:db00:209b:18::/64
    via 2401:db00:209b:18::a dev fboss2025 weight 1
Network Address: 2401:db00:209b:19::/64
    via 2401:db00:209b:19::a dev fboss2026 weight 1
Network Address: 2401:db00:209b:1::/64
    via 2401:db00:209b:1::a dev fboss2002 weight 1
Network Address: 2401:db00:209b:1a::/64
    via 2401:db00:209b:1a::a dev fboss2027 weight 1
Network Address: 2401:db00:209b:1b::/64
    via 2401:db00:209b:1b::a dev fboss2028 weight 1
Network Address: 2401:db00:209b:1c::/64
    via 2401:db00:209b:1c::a dev fboss2029 weight 1
Network Address: 2401:db00:209b:1d::/64
    via 2401:db00:209b:1d::a dev fboss2030 weight 1
Network Address: 2401:db00:209b:1e::/64
    via 2401:db00:209b:1e::a dev fboss2031 weight 1
Network Address: 2401:db00:209b:1f::/64
    via 2401:db00:209b:1f::a dev fboss2032 weight 1
Network Address: 2401:db00:209b:20::/64
    via 2401:db00:209b:20::a dev fboss2033 weight 1
Network Address: 2401:db00:209b:21::/64
    via 2401:db00:209b:21::a dev fboss2034 weight 1
Network Address: 2401:db00:209b:22::/64
    via 2401:db00:209b:22::a dev fboss2035 weight 1
Network Address: 2401:db00:209b:23::/64
    via 2401:db00:209b:23::a dev fboss2036 weight 1
Network Address: 2401:db00:209b:24::/64
    via 2401:db00:209b:24::a dev fboss2037 weight 1
Network Address: 2401:db00:209b:25::/64
    via 2401:db00:209b:25::a dev fboss2038 weight 1
Network Address: 2401:db00:209b:26::/64
    via 2401:db00:209b:26::a dev fboss2039 weight 1
Network Address: 2401:db00:209b:27::/64
    via 2401:db00:209b:27::a dev fboss2040 weight 1
```

</div>

---



### transceiver

Show Transceiver information including optics details, power levels, and diagnostics.

**Usage:**
```bash
fboss2 show transceiver
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 Interface  Status  Transceiver  CfgValidated  Reason  Vendor         Serial        Part Number       FW App Version  FW DSP Version  Temperature (C)  Voltage (V)  Current (mA)                Tx Power (dBm)        Rx Power (dBm)               Rx SNR
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 eth1/1/1   Up      FR4_2x400G   --            --      Eoptolink      UQAB040183    EOLO-168HG-02-1B  3.12            209.28          60.61            3.27         105.23,92.23,109.13,109.06  1.06,1.10,0.89,1.16   -0.92,-1.09,-1.21,-1.21      22.72,22.69,22.66,22.21
 eth1/1/5   Up      FR4_2x400G   --            --      Eoptolink      UQAB040183    EOLO-168HG-02-1B  3.12            209.28          60.61            3.27         108.08,93.00,98.12,110.40   1.06,1.32,1.22,0.52   -1.11,-1.28,-0.97,-1.24      22.64,22.43,23.05,22.67
 eth1/10/1  Down    Absent       --            --                                                                                     0.00             0.00
 eth1/10/5  Down    Absent       --            --                                                                                     0.00             0.00
 eth1/17/1  Up      FR4_2x400G   --            --      INNOLIGHT      INOAFC301840  T-OL8CNT-NF2      103.137         1.6             62.86            3.30         67.50,67.50,67.50,67.50     1.46,0.88,0.49,0.71   -1.87,-2.04,-1.73,-1.63      22.47,22.05,22.18,21.66
 eth1/17/5  Up      FR4_2x400G   --            --      INNOLIGHT      INOAFC301840  T-OL8CNT-NF2      103.137         1.6             62.86            3.30         67.50,67.50,67.50,67.50     0.98,1.08,1.13,1.09   -1.39,-1.48,-1.49,-1.00      22.47,22.32,22.32,22.62
 eth1/18/1  Up      FR4_2x400G   --            --      INNOLIGHT      INOAFC300620  T-OL8CNT-NF2      103.137         1.6             64.79            3.32         67.50,67.50,67.50,67.50     1.36,1.17,1.36,0.77   -1.89,-2.09,-1.75,-1.87      22.32,21.91,22.47,20.97
 eth1/18/5  Up      FR4_2x400G   --            --      INNOLIGHT      INOAFC300620  T-OL8CNT-NF2      103.137         1.6             64.79            3.32         67.50,67.50,67.50,67.50     1.58,1.24,1.85,1.49   -2.87,-2.48,-2.16,-2.23      21.91,22.18,22.32,21.41
 eth1/19/1  Down    FR4_2x400G   --            --      INNOLIGHT      INOAFC230286  T-OL8CNT-NF2      103.137         1.6             58.93            3.32         67.50,67.50,67.50,67.50     0.66,0.66,0.64,0.84   -40.00,-40.00,-40.00,-40.00  0.00,0.00,0.00,0.00
 eth1/19/5  Down    FR4_2x400G   --            --      INNOLIGHT      INOAFC230286  T-OL8CNT-NF2      103.137         1.6             58.93            3.32         67.00,67.50,67.50,67.50     -0.39,0.15,0.95,0.64  -40.00,-40.00,-40.00,-40.00  0.00,0.00,0.00,0.00
 eth1/2/1   Up      FR4_2x400G   --            --      Eoptolink      UQAB040181    EOLO-168HG-02-1B  3.12            209.28          60.63            3.28         98.34,85.30,94.01,93.01     1.27,1.27,1.28,1.25   -2.19,-2.38,-2.74,-2.79      23.16,23.48,22.63,21.96
 eth1/2/5   Up      FR4_2x400G   --            --      Eoptolink      UQAB040181    EOLO-168HG-02-1B  3.12            209.28          60.63            3.28         75.10,82.00,105.80,110.20   1.05,0.90,1.13,0.74   -0.80,-1.28,-0.69,-1.14      23.61,22.85,23.65,22.66
```

</div>

---




### config

Show config information for various binaries.

**Usage:**
```bash
fboss2 show config <subcommand>
```

#### Subcommands

| Subcommand | Description |
|------------|-------------|
| `running` | Show running config for various binaries |

---



### fsdb

Show operational information from FSDB (FBOSS State Database).

**Usage:**
```bash
fboss2 show fsdb <subcommand>
```

#### Subcommands

| Subcommand | Description |
|------------|-------------|
| `publishers` | Show fsdb publishers |
| `state` | Show fsdb operational state |
| `stats` | Show fsdb operational stats |
| `subscribers` | Show fsdb subscribers |

#### fsdb publishers

Show fsdb publishers.

**Usage:**
```bash
fboss2 show fsdb publishers
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
 Publishers Id   Type   Raw Path        isStats
-----------------------------------------------------
 agent           PATH   agent           1
 agent           PATCH  agent           0
 bgpd            PATCH  bgp             0
 qsfp_service    PATH   qsfp_service    1
 qsfp_service    PATCH  qsfp_service    0
 sensor_service  PATH   sensor_service  1
```

</div>

---

### version

Show versions information for various binaries.

**Usage:**
```bash
fboss2 show version <subcommand>
```

#### Subcommands

| Subcommand | Description |
|------------|-------------|
| `agent` | Show agent information |
| `bgp` | Show bgp information |
| `coop` | Show coop information |
| `data_corral` | Show data corral service information |
| `fsdb` | Show fsdb information |
| `led` | Show led information |
| `mka` | Show mka information |
| `qsfp` | Show qsfp information |
| `rackmon` | Show rackmon information |
| `sdk` | Show sdk information |
| `sensors` | Show sensor information |

#### version agent

Show agent version information.

**Usage:**
```bash
fboss2 show version agent
```

**Sample Output:**

<div style={{maxHeight: '200px', overflowY: 'auto', fontFamily: 'Courier New, monospace'}}>

```text
Package Name: neteng.fboss.wedge_agent
Package Info: neteng.fboss.wedge_agent:ea4de14482501f000e9c338e0dcf65b5
Package Version: neteng.fboss.wedge_agent:799
Build Details:
     Host: 7071-88e2-0004-0000.twshared71028.02.ncg3.tw.fbinfra.net
     Time: Sun Feb  8 18:33:06 2026
     User: root
     Path: /data/sandcastle/boxes/trunk-hg-fbcode-fbsource
     Platform: platform010
     Revision: 1243ac8e7d8b3536736c3649f1a889bac2c4ed22
```

</div>

---


## clear

Reset counters, clear tables, and remove temporary state.

**Usage:**
```bash
fboss2 clear <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `arp` | Clear ARP information |
| `interface` | Clear Interface Information |
| `ndp` | Clear NDP information |
| `mirror` | Clear mirror |

### clear arp

Clear ARP table entries.

```bash
fboss2 clear arp
```

### clear interface

Clear Interface information (e.g., counters).

```bash
fboss2 clear interface counters
```

### clear ndp

Clear NDP table entries.

```bash
fboss2 clear ndp
```

### clear mirror

Clear mirror session.

```bash
fboss2 clear mirror
```

---

## set

Modify object properties and settings.

**Usage:**
```bash
fboss2 set <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `interface` | Set Interface information |
| `port` | Set Port information |
| `debug` | Set debug level |
| `fanhold` | Set fan hold PWM |

### set interface

Set Interface information.

```bash
fboss2 set interface <interface_name> [options]
```

### set port

Set Port information.

```bash
fboss2 set port <port_name> [options]
```

### set debug

Set debug level for logging.

```bash
fboss2 set debug [options]
```

### set fanhold

Set fan hold PWM value.

```bash
fboss2 set fanhold <value>
```

---

## get

Retrieve specific object information.

**Usage:**
```bash
fboss2 get <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `pcap` | Get Packet Capture |

### get pcap

Get packet capture data.

```bash
fboss2 get pcap [options]
```

---

## config

Configuration management commands for viewing and modifying switch configuration.

**Usage:**
```bash
fboss2 config <subcommand> [options]
```

*Note: Use `fboss2 show config` for viewing configuration information.*

---

## create

Create new objects on the switch.

**Usage:**
```bash
fboss2 create <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `config` | Create wedge_agent, bgp, openr configs |
| `mirror` | Create mirror |

### create config

Create configuration for various binaries.

```bash
fboss2 create config [options]
```

### create mirror

Create a mirror session.

```bash
fboss2 create mirror [options]
```

---

## delete

Remove objects from the switch.

**Usage:**
```bash
fboss2 delete <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `config` | Delete config objects |

### delete config

Delete configuration objects.

```bash
fboss2 delete config [options]
```

---

## bounce

Disable and re-enable objects. Useful for resetting ports or interfaces without manual disable/enable steps.

**Usage:**
```bash
fboss2 bounce <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `interface` | Shut/No-Shut Interface |

### bounce interface

Bounce (shut/no-shut) an interface.

```bash
fboss2 bounce interface <interface_name>
```

**Example:**
```bash
fboss2 bounce interface eth1/1/1    # Bounce a specific interface
```

---

## reload

Reload configuration or state for objects.

**Usage:**
```bash
fboss2 reload <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `config` | Reload binary configuration file |

### reload config

Reload binary configuration file.

```bash
fboss2 reload config [options]
```

---

## debug

Access debugging and diagnostic functions for advanced troubleshooting.

**Usage:**
```bash
fboss2 debug <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `bgp` | Debug BGP Configuration |

### debug bgp

Debug BGP configuration.

```bash
fboss2 debug bgp [options]
```

---

## start

Start events or processes.

**Usage:**
```bash
fboss2 start <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `pcap` | Start Packet Capture |

### start pcap

Start packet capture.

```bash
fboss2 start pcap [options]
```

---

## stop

Stop events or processes.

**Usage:**
```bash
fboss2 stop <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `pcap` | Stop Packet Capture |

### stop pcap

Stop packet capture.

```bash
fboss2 stop pcap [options]
```

---

## stream

Continuously stream real-time data from the switch. Useful for monitoring events as they occur.

**Usage:**
```bash
fboss2 stream <subcommand> [options]
```

### Subcommands

| Subcommand | Description |
|------------|-------------|
| `bgp` | Continuously stream BGP session information |
| `fsdb` | Stream fsdb operational information |

### stream bgp

Continuously stream BGP session information.

```bash
fboss2 stream bgp [options]
```

### stream fsdb

Stream fsdb operational information.

```bash
fboss2 stream fsdb [options]
```

---

## help

Display help information for commands and objects.

**Usage:**
```bash
fboss2 help
fboss2 help <command>
fboss2 <command> --help
```
---

## See Also

- [CLIs for L1 Debugging](/docs/debugging/clis_for_l1_debugging.md)
