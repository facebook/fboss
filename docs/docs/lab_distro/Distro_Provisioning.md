---
id: distro_provisioning
title: FBOSS Distro Provisioning
description: Provisioning requirements and options for FBOSS Distro
keywords:
    - FBOSS
    - OSS
    - provisioning
    - PXE
    - ONIE
    - USB
oncall: fboss_oss
---

# FBOSS Distro Provisioning

## Provisioning Requirements

To provision FBOSS Distro, you require one of the following boot options:

- **USB Boot**
- **PXE Boot**
- **ONIE Boot**

## How FBOSS Distro Works

FBOSS Distro is a single ISO-based image which contains services that automatically start based on hardware requirements detected during early provisioning. The following services are brought up in a specific sequence required for the NOS.

### Platform Stack

| Service | Description |
|---------|-------------|
| **Platform Manager Service** | Sets up base-level services and installs a BSP kernel module to configure all hardware on the box |
| **Fan Service** | Controls fan speed |
| **Sensor Service** | Controls sensor data and temperatures |
| **Data Corral Service** | Controls legacy sensor metadata |

**CLIs Distributed:**
- `weutil`
- `fwutil`
- `sensor_service_data`

### FBOSS Forwarding Stack

| Service | Description |
|---------|-------------|
| **FSDB Service** | Pub/Sub system that communicates with all services required for critical components |
| **FBOSS Agent** | Core service that programs the ASIC and communicates with other network configuration tools |
| **FBOSS HW Agent** | Controls an individual ASIC and its internal state. Multiple instances may run depending on the hardware |
| **FBOSS SW Agent** | Primary web endpoint that communicates with all HW agents for route programming |
| **FBOSS QSFP Service** | Primarily responsible for programming optics/transceivers from FBOSS SW Agent requests |
| **FBOSS BGP** | BGP routing daemon that communicates with FBOSS SW Agent (Available Q3 2026) |

**CLIs Distributed:**
- `fboss_bcm_shell`
- `bcm_shell`

## Provisioning Options

### General Requirements

- **BMC IP Address**: Must be SLAAC assigned in your environment on the management network
- **ETH0 IP Address**: Will also use SLAAC assignment during provisioning

### USB Boot

A USB-based ISO is available on the FBOSS GitHub page for easy reproducibility. Use any USB drive imaging tool with the ISO and the system will boot automatically.

### PXE Boot

#### Requirements

- PXE requests must not have any race conditions for the ETH0 IP allocation with other DHCP servers on the network

#### Option 1: Configure Your Own DHCP Server

Configure your own DHCP server in your lab to serve the ISO for a targeted set of MAC addresses.

- Refer to your respective DHCP server documentation (e.g., Kea, DHCPD)
- Your lab environment must contain a DHCP server where you can point the Distro ISO

#### Option 2: Configure a Local Provisioning DHCP Server

Configure a local provisioning DHCP server only targeting PXE requests for a target set of MAC addresses.

- Only use this option if you foresee challenges with DHCP server isolation in your environment
- Refer to respective DHCP server documentation
- Requires a dedicated server on the L2 management network

#### Option 3: Use Distro Provisioning Container and Distro CLI

The FBOSS team provides the Distro Container which uses dnsmasq as a PXE environment to serve PXE requests.

- Similar to Option 2, this requires a dedicated provisioning server
- Use the Distro CLI to deploy the image to the environment

### ONIE Boot

ONIE-based ISOs will be available. Refer to the [ONIE documentation](https://opencomputeproject.github.io/onie/overview/) for details.

:::note
ONIE boot support is currently in development.
:::

## Supporting New NPIs and Custom Binary Deployments

For deploying custom binaries or controllers alongside FBOSS Distro, you have two options:

### Option 1: Run Custom Binaries in Docker Container

Run your custom controller or binary in a Docker container within the FBOSS OSS Distro. This is the recommended approach for most use cases as it provides isolation and easier deployment management.

### Option 2: Build and Deploy Custom Binaries Directly

1. Use the FBOSS OSS build environment as described in [Building FBOSS on Docker Containers](/docs/build/building_fboss_on_docker_containers/)
2. Rebuild your custom binary using the build environment
3. Copy the binaries to the switch running FBOSS OSS Distro
4. Run the binary directly on the switch
