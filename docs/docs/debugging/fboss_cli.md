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
