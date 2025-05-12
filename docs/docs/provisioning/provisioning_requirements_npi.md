# Provisioning Requirements - NPI

## PE Network Provisioning

### Version 0.3.1

### October 2024

4-5 Grand Canal Quay

Dublin, D02 X525

Republic of Ireland

## Overview

The purpose of this document is to specify a set of requirements new devices
should be compliant with, so that any integration with the internal provisioning
process is simple and fast. Same kind of problems keep appearing during new
product introductions and instead of trying to resolve them every time a new
device is added to the fleet, the intention is to formally specify the features
and requirements that should be supported and tested by vendors before any
integration work would commence. That would save a lot of time and effort for
both the company and external vendors, since addressing these issues is
expensive and slow for all parties involved. All of the requirements in this
document are Must-Have (MH) and essential to the provisioning process.

## Goals

1. **Define required features**: These features are supported by the device but
    can often come disabled or misconfigured.

2. **Define acceptance tests**: Specify a set of tests and features that should
    work out-of-the-box for all devices.

### Table of Contents

- [Provisioning Requirements - NPI](#provisioning-requirements---npi)
  - [PE Network Provisioning](#pe-network-provisioning)
  - [Overview](#overview)
  - [Goals](#goals)
    - [Table of Contents](#table-of-contents)
  - [Required features](#required-features)
    - [1. DHCP Options](#1-dhcp-options)
    - [2.  IPv6 Stateless Address Autoconfiguration Support](#2--ipv6-stateless-address-autoconfiguration-support)
    - [3. SSH access capability](#3-ssh-access-capability)
    - [4. Console Auto Discovery](#4-console-auto-discovery)
    - [5. Bootloader](#5-bootloader)
    - [6. Boot sequence behavior](#6-boot-sequence-behavior)
    - [7. Weutil support](#7-weutil-support)
    - [Output format](#output-format)
    - [8. Layer 2 Packet Forwarding](#8-layer-2-packet-forwarding)
    - [9. wedge\_us\_mac.sh](#9-wedge_us_macsh)
    - [10. wedge\_power.sh script](#10-wedge_powersh-script)
    - [11. Firmware upgrade support](#11-firmware-upgrade-support)
    - [12.  Serial console speed](#12--serial-console-speed)

## Required features

### 1. DHCP Options

**Applies to: BMC**

To allow for the device to provision, we need to have a mapping between MAC
addresses and serial numbers. For this reason, devices are required to submit
their serial number within the DHCPv6 packet. For existing compatibility with
Facebook systems, the following information string format is used:

```bash
OpenBMC:key1=value1:key2=value2[...:keyN=valueN]
```

The keys we are currently using are:

**Prefix**: Device family - must be present

**Model**: Hardware model - mandatory

**Serial**: Serial Number – mandatory

The vendor should use the string “OpenBMC” as the prefix, since the BMC is the
device tasked with sending these DHCP options. If there are multiple
network-capable components (microserver with an attached BMC), only one device
should send the request - the BMC. The requests should have the serial number
that reflects the main (chassis) serial number of the device. Meta owns the
responsibility of extending this specification if needed, but in the current
version of the provisioning requirements specification prefix, model and serial
number are the fields required and vendors are not expected to deviate from this
specification.

Examples provided are for a BMC (1):

1. `send vendor-class-identifier "OpenBMC:model=MINIPACK-SMB:serial=1234";`

If DHCPv4 is agreed to be implemented, data is encoded in Option 60 (Vendor
Class Identifier, RFC 3925).

For DHCPv6, this information will be included as a sub-option in the
"Vendor-specific Information Option" (17). The sub-option number will be (1).
(RFC 3315, section 22.15). In this case, an Enterprise Number needs to be
included to be compliant.

Facebook has Enterprise Number 40981, and is provided merely as an example
here - it is **valid and expected** for the vendor to provide its own number
instead.

Example DHCP config:

```bash
send vendor-class-identifier "OpenBMC:model=MINIPACK-SMB";

option dhcp6.linuxboot-id code 17 = {

  integer 32, # enterprise number

  integer 16, # sub option

  integer 16, # length

  string

};

send dhcp6.linuxboot-id 40981 1 44 "OpenBMC:model=MINIPACK-SMB=serial=1234";
```

![DHCPv6 Option 17 Encoding Example](dhcp_v6.png)
*DHCPv6 Option 17 Encoding Example*

### 2.  IPv6 Stateless Address Autoconfiguration Support

#### Applies to: BMC

In order to be able to contact the device, the provisioning system needs to
correctly assume which address it needs to connect to. Therefore, it is
necessary to support stateless address autoconfiguration according to RFC 4862.

- System sends ICMPv6 Router Solicitation (RS) message
- Gets a Router Advertisement (RA) message
- Configures the SLAAC address on the network management interface out of the
  box, respecting the flags received within the RA message

### 3. SSH access capability

**Applies to: BMC**

Provisioning system needs to be able to log in out of the box and issue commands
to commence the provisioning process. Therefore, the SSH daemon on port 22 will
accept connections regardless of the originating address and allow access with
OpenBMC default credentials.

### 4. Console Auto Discovery

**Applies to: BMC**

At every boot time (even with the initial image shipped on the device from the
factory) and once per day, the OpenBMC should print a special string to the
serial console used to automatically detect the location and port of the console
server the device plugs into.

Ideally, the BMC bootloader would also output the console autodiscovery string
described in this document. If, from the implementation perspective, it’s too
complex to read the serial from a bootloader environment, it is a perfectly
acceptable alternative to do it in OpenBMC only.

The string begins with an exclamation mark, followed by the magic string
“serfmon” and 3 fields separated by colons - serial length, serial number, and
checksum, as follows:

```bash
!serfmon:{serial number length}:{serial number}:{serial number checksum}
```

**Example**:

```bash
!serfmon:10:AI21204498:8
```

**Remarks**

Both serial number length and checksum are printed as a decimal number.

Checksum is calculated by bitwise XORing all characters of the serial number
together.

Serial number refers to the main, chassis serial number of this device, not the
BMC’s serial number.

String is followed by a newline.

**Reference implementation:**

C

```c
char serial[]="AI21204498";
char checksum = '\0';

for (int i=0; i<strlen(serial); i++)
   checksum ^= serial[i];

printf("!serfmon:%d:%s:%d\n", strlen(serial), serial, checksum);
```

Python 3

```py3
from functools import reduce
serial = "AI21204498"

checksum = reduce(lambda a, b: a ^ ord(b), serial, 0)
print("!serfmon:{0}:{1}:{2}".format(len(serial), serial, checksum))
```

### 5. Bootloader

**Applies to: BMC**

BMC serial console speed is required to be 9600 bps, 8-N-1, no flow control.
Bootloader should offer CLI on interruption where a custom image can be
specified and booted by any means supported.

**Applies to: Microserver**

Bootloader should have a networking stack with IPv6 support and should be able
to support netboot from such a network. To netboot, devices should support TFTP
and HTTP/HTTPS. Also, it’s important to implement DNS resolving support, so if a
hostname is provided in the bootfile URL, the name servers returned by the DHCP
request should be used to resolve this hostname into an IP.
DHCPv6 implementation used to netboot should rely on DUID-LLT instead of
DUID-EN, since we rely on the client's link-layer address present within the
packet.

### 6. Boot sequence behavior

**Applies to: Microserver**

- PXE IPv6 boot will always be the first choice
- Local SSD/flash is the next choice
- USB flash drive is the next choice (optional, disabled by default).
- Start from beginning

There should be no unconventional requirements to boot the kernel, like
providing the ACPI tables as a binary blob, etc. Those should be defined by the
BIOS and loaded into the main memory for the OS to consume.

BIOS should, after loss of power, default to ON state when power is reapplied
and NOT try to return to "last known state".

### 7. Weutil support

**Applies to: BMC**

To retrieve the Serial Number, Asset Tag and other information essential for
device identification, the “weutil” command is used. The output is provided as
colon+space separated keys and values, one per line.

Weutil being able to read the **chassis EEPROM** is a hard requirement on the
BMC side. Format for these should be previously agreed with Meta.

**Applies to: MICROSERVER**

Similar commands are provided for major components in the system (e.g., peutil
tool to dump the information about PIMs) - these are expected to get implemented
on the x86 side and details agreed with Meta.

### Output format

Example command output:

```bash
            Wedge EEPROM :
            Version: 1
            Product Name: WEDGE100S12V
            Product Part Number: 20-002324
            System Assembly Part Number: 13-5000013-03
            Facebook PCBA Part Number: 132-000052-03
            Facebook PCB Part Number: 131-000025-04
            ODM PCBA Part Number: NP3FB7632001A
            ODM PCBA Serial Number: RRG43015754
            Product Production State: 4
            Product Version: 1
            Product Sub-Version: 2
            Product Serial Number: RRG43015754
            Product Asset Tag: 8377476
            System Manufacturer: Joytech
            System Manufacturing Date: 04-15-21
            PCB Manufacturer: ISU
            Assembled At: Joytech
            Local MAC: 90:3C:B3:17:08:4C
            Extended MAC Base: 90:3C:B3:17:08:4D
            Extended MAC Address Size: 128
            Location on Fabric: WEDGE100
            CRC8: 0xc3
```

Weutil **mandatory** fields:

- Product Serial Number
- Local MAC
- Product Name
- Product Asset Tag
- Product Part Number
- Product Version
- Product Sub-Version
- CRC8

As an optional argument, the tool should be able to take a path (eeprom readout
binary file) and parse/display the data as if it were reading the eeprom itself.

JSON output support - provide `weutil --json` to produce both machine and
human parsable output, like:

```json
{
    "Product Name": "ELBERT",
    "Product Part Number": "7388",
    "System Assembly Part Number": "ASY123456789",
    "Product Serial Number": "SJC1234567",
…
```

This reduces the chance of introducing bugs due to parsing.

Serial number needs to match the **chassis serial number**, even if the
component storing the serial number is a FRU and can be replaced itself. The
reasoning behind this is that the serial number in internal databases references
the chassis itself, regardless of which parts are swapped throughout the
product’s life cycle.

### 8. Layer 2 Packet Forwarding

**Applies to: BMC, Microserver**

The device must support the link-layer discovery protocol (LLDP). Therefore, it
needs to forward packets with a destination physical address of
01:80:c2:00:00:0e  to all other ports instead of dropping them.

Internal switch usually consists of an Ethernet IC like the BCM5389, which loads
its configuration through an externally connected EEPROM. Option to support MAC
multicast packets forwarding needs to be explicitly configured - one of the
things frequently missing in devices delivered.

BMC should support lldp-util command out of the box and this command is expected
to display the received lldp broadcasts in the following format:

```bash
OpenBMC# lldp-util
Listening for LLDP Packets…

LLDP: local_port=eth0 remote_system=rsw1aa.99.tst1 remote_port=Ethernet36
```

### 9. wedge_us_mac.sh

**Applies to: BMC**

This command, when executed on the BMC, returns the mac address of the
x86/microserver. The mac address is printed in the standard colon-separated hex
format followed by a newline. Exit code should differ from 0 in case there was a
problem reading/retrieving the mac due to e.g. communication problems with the
microserver.

Example:

```bash
bash$ wedge_us_mac.sh
de:ad:be:ef:ca:fe
bash$
```

If the x86/microserver has more than one management ethernet, the command with
no arguments will retain the existing behavior and when invoked with
`--interface2` or `--interfaceN` will return the N-th interface’s MAC address.

### 10. wedge_power.sh script

**Applies to: BMC**

One of the most important features the BMC is required to have on delivery is
the correct power control script behavior which is defined in FBOSS [Meta Switch Architecture](/docs/architecture/meta_switch_architecture)
document, section I-3 System Control, subsection Power.

It is expected that the wedge_power.sh off command takes no more than 30
seconds, and wedge_power.sh on and reset take no more than 10.

### 11. Firmware upgrade support

**Applies to: Microserver**

Upgrading the firmware can be done from the microserver side. A fixed order of
upgrade should be clearly defined, addressing potential issues where upgrading a
component’s controller might prevent further communication with the component
itself.

Upgrade binary that performs the actual write should be supplied by the vendor,
preferably with source code. If that is not possible, a static binary built for
the target platform is required. The entire tooling to upgrade the firmware
should not exceed 25 MB in size, as it might need to be included in the
universal ramdisk image which also applies to platforms with little memory.

Bootloader is required to have a recovery path in case of a bad/failed upgrade.
It can either be an active-backup image configuration or an additional read-only
failsafe image, with the latter preferred. There should exist no scenario in
which the device requires physical/JTAG access to recover a failed upgrade or a
broken firmware image. This applies to malicious intent scenarios as well as
accidents.

Upgrade time is to be considered during the hardware design phase and the entire
firmware upgrade procedure for the device, assuming version mismatch in every
component, should not exceed 20 minutes. Suggested strategy for programmable
logic devices is to avoid using offline mode and choosing devices which support
background programming by e.g., writing the SPI flash the device is bootstrapped
from. That way, all components’ versions change simultaneously on the next
reboot and reduce the number of different states the switch can be in.

Performing the upgrade should be an atomic operation, with two possible
outcomes - success or failure. After the operation finishes with a successful
outcome, we expect the component to be upgraded; staging the upgrade for future
writes violates this requirement, as there might be a discrepancy between
provisioning systems considering the component upgraded vs. the component
failing a staged future write.

We **require** the **ability to downgrade** the firmware versions, so a rollback
is possible in case of a bad/buggy firmware release.

### 12.  Serial console speed

BMC serial console is always 9600 bps, 8-N-1 as this is a fleetwide standard.

For compatibility reasons, serial output for both BIOS and OS on the x86 must
match the same speed - 57600 bps 8-N-1 (therefore, terminal implementation on
the BMC must support this).

For further information, please refer to [Meta Switch Architecture](/docs/architecture/meta_switch_architecture)
document section I-5 Console Connection.
