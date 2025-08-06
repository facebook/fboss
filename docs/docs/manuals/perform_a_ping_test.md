---
id: perform_a_ping_test
title: Perform a Ping Test
description: How to perform a ping test
keywords:
    - FBOSS
    - OSS
    - onboard
    - manual
    - ping
oncall: fboss_oss
---

## Overall Outcomes

- the DUT is connected to another host
- an agent config is generated based on the cable connections, with IPs assigned
to connected interfaces
- a QSFP config is generated
- on each device, you will have a directory `/opt/fboss` which contains a `bin`,
`lib`, and `share` directory
- the `qsfp_service` and `wedge_agent` binaries will be running on each device
- the link to the other host should be up
- ping works between the DUT and other host

## Step 1: Build a Ping Test Setup

### Step 1.1: Connect the DUT to Another Host

The first step is to connect the DUT to another host that has at least one link.

**Note:** The other host can be any device. It does not have to be an FBOSS
device.

### Step 1.2: Generate Agent Config

Generate an agent config based on the cable connection and assign IPs to
connected interfaces.

All agent config settings are the same as link test, except that we need to
assign IPs to the L3 interfaces connected to the other device.

For example, say port eth1/1/1 is connected to the other host. This port is put
in vlan interface 2001, with assigned IP addresses "2400::/64" and "10.0.0.0/24"
:

```json
   "ports": [
     {
       "logicalID": 9,
       "state": 2,
       "minFrameSize": 64,
       "maxFrameSize": 9412,
       "parserType": 1,
       "routable": true,
       "ingressVlan": 2001,
       "speed": 400000,
       "name": "eth1/1/1",
       ...
     },
     ...
   ]

   "interfaces": [
     {
       "intfID": 2001,
       "routerID": 0,
       "vlanID": 2001,
       "ipAddresses": [
         "2400::/64",
         "10.0.0.0/24"
       ],
       ...
     },
     ...
   ]
```

### Step 1.3: Generate QSFP Config

Please adhere to the guidelines found on
[this page](/docs/developing/qsfp_test_config/) when generating the QSFP config.

### Outcomes

- the DUT is connected to another host
- an agent config is generated based on the cable connections, with IPs assigned
to connected interfaces
- a QSFP config is generated

## Step 2: Build Relevant Binaries

In this step, we will use `getdeps.py` to build the `qsfp_service` and
`wedge_agent` binaries. Detailed instructions for building FBOSS are contained
in this [guide](/docs/build/building_fboss_on_docker_containers/). Read and
follow along, using instructions for building against a precompiled SDK.

### Outcomes

- an FBOSS Docker container
- within the container, the FBOSS repository cloned at `/var/FBOSS`
- within the container, a scratch directory in `/var/FBOSS` (probably named
`tmp_bld_dir`) with build artifacts
- build artifacts located under `/var/FBOSS/<$scratch_directory>/build/fboss/`

## Step 3: Package the Build Artifacts

Follow the instructions in this [section](/docs/build/packaging_and_running_fboss_hw_tests_on_switch/#package-fboss)
of our packaging guide to package the artifacts. Run the commands from within
the Docker container created in the previous step.

### Outcomes

- within the scratch directory at `/var/FBOSS`, you should have a directory or
tarball with the prefix `fboss_bins`

## Step 4: Send the Packaged Artifacts to the Devices

Follow the instructions in this [section](/docs/build/packaging_and_running_fboss_hw_tests_on_switch/#copy-the-directory-or-tarball-to-the-switch)
of our packaging guide for each device to send the packages and set them up.

### Outcomes

- on each device, you will have a directory `/opt/fboss` which contains a `bin`,
`lib`, and `share` directory

## Step 5: Run Binaries

On each device, run the binaries using the configs generated from previous
steps:
```bash
cd /opt/fboss
source ./bin/setup_fboss_env

./bin/qsfp_service --qsfp-config <$qsfp_config>
./bin/<$wedge_agent_name> --config <$agent_config>
```

### Outcomes

- the binaries will be running on each device

## Step 6: Perform the Ping Test

### Step 6.1: Verify Link to the Other Host

Verify that the link connected to the other host is up:
```bash
# Verify that the port is up
fboss2 show port

# Verify that the correct IP is assigned
fboss2 show interface
```

### Step 6.2: Verify that Ping Works

Verify that ping works between DUT and the other host:
```bash
ping <$remote_host_IP>
```

Commands for debugging:
```bash
# Verify that mac entry to the other host is learned
fboss2 show mac details

# Verify that ndp entry to the other host is learned
fboss2 show ndp

fboss2 show arp

# Start and stop capturing all control packets sent to/received by FBOSS control plane
fboss2 start|stop pcap
```

### Outcomes

- the link to the other host should be up
- ping works between the DUT and other host
