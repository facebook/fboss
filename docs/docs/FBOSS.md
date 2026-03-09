---
id: about_fboss
title: FBOSS Agent Architecture
description: FBOSS Agent Software Architecture Overview
sidebar_position: 1
keywords:
    - FBOSS
    - OSS
    - Architecture
    - Agent
oncall: fboss_oss
---

# FBOSS Agent Software Architecture

This document provides an overview of the FBOSS (Facebook Open Switching System) Agent software architecture, including the switch software stack, the detailed agent internal architecture, and how to interact with the system using CLI.

## FBOSS Switch Software Stack

FBOSS is a software stack that runs on the x86 CPU of a network switch. It is designed to be modular, scalable, and flexible, allowing for easy integration with various hardware platforms. The FBOSS software architecture consists of multiple micro-services that work together to provide a comprehensive networking solution.

### High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                   FSDB                                      │
│                        (FBOSS State Database Service)                       │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │  Central State Store  |  Pub/Sub Model  |  State + Stats Streams    │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└────────────┬───────────────────────────────────────────┬────────────────────┘
             │ Thrift Pub/Sub                            │ Thrift Pub/Sub
             ▼                                           ▼
┌────────────────────────────────────┐   ┌────────────────────────────────────┐
│          FBOSS Agent               │   │           QSFP Service             │
│                                    │   │                                    │
│  ┌──────────────────────────────┐  │   │  ┌──────────────────────────────┐  │
│  │    FbossCtrl Thrift API      │  │   │  │    QsfpService Thrift API    │  │
│  │    - Route Management        │  │   │  │    - Transceiver Management  │  │
│  │    - Port Control            │  │   │  │    - External PHY Control    │  │
│  │    - State Queries           │  │   │  │    - DOM Monitoring          │  │
│  └──────────────────────────────┘  │   │  └──────────────────────────────┘  │
│                                    │   │                                    │
│  Manages: Switch ASIC/NPU          │   │  Manages: Transceivers, PHYs       │
│  Forwarding & Control Plane        │   │                                    │
└────────────────────────────────────┘   └────────────────────────────────────┘
```

### Core Components

#### Agent (wedge_agent)
The central component responsible for managing the forwarding switch ASIC/NPU. It handles:
- Programming forwarding tables (routes, neighbors, ACLs)
- Control plane protocols (ARP, NDP)
- Port management and configuration
- Statistics collection and monitoring

#### FSDB (FBOSS State Database)
A central store for all state of the switch that follows a publish/subscribe model:
- **Publishers**: On-box services (Agent, QSFP Service, etc.) publish their state and stats
- **Subscribers**: Both on-box and off-box services can subscribe to this data
- **Transport**: Backed by Thrift streams for efficient data delivery
- **Data Model**: Organized into State (event-based changes) and Stats (periodic metrics)

#### QSFP Service
Manages transceivers and external PHYs:
- Transceiver detection and configuration
- DOM (Digital Optical Monitoring) data collection
- External PHY initialization and management
- Link diagnostics

### Simplified Layering View

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              SwSwitch                                       │
│            (Software Switch - manages state and control plane)              │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              HwSwitch                                       │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                    SaiSwitch (FBOSS SAI Layer)                      │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                    Vendor SAI  |  Vendor SDK                        │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ASIC Hardware                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Agent Architecture Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              SW Agent (SwSwitch)                            │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                        Thrift Handler Layer                         │   │
│   │    FbossCtrl API  |  MultiSwitchCtrl API  |  Test APIs              │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                        Core Logic Layer                             │   │
│   │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │   │
│   │  │   RIB    │ │ Neighbor │ │   Port   │ │   ACL    │ │   LLDP   │   │   │
│   │  │ Manager  │ │ Updater  │ │ Handler  │ │ Handler  │ │ Manager  │   │   │
│   │  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘   │   │
│   │                        ... (other managers)                         │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                       SwitchState (COW)                             │   │
│   │          Unified view of switch configuration and state             │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                    MultiHwSwitchHandler                             │   │
│   │         Manages multiple HW Switch instances (if applicable)        │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │ State Delta / OperDelta
                               │ (Thrift streams for split-agent mode)
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          HW Agent(s) (HwSwitchHandler)                      │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                      HwSwitch Interface                             │   │
│   │    Abstracts hardware-specific operations from SW Agent             │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   ┌──────────────────────────┐          ┌───────────────────────────────┐   │
│   │    HW Agent Instance 0   │   ...    │      HW Agent Instance N      │   │
│   │      (Switch ID 0)       │          │        (Switch ID N)          │   │
│   └──────────────────────────┘          └───────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                FBOSS SAI Layer (SaiSwitch) for HW Agent Instance 0          │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                     SaiManagerTable                                 │   │
│   │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │   │
│   │  │ SaiPort  │ │ SaiRoute │ │ SaiNeigh │ │  SaiACL  │ │ SaiQueue │   │   │
│   │  │ Manager  │ │ Manager  │ │ Manager  │ │ Manager  │ │ Manager  │   │   │
│   │  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘   │   │
│   │                        ... (other managers)                         │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                    SAI API Wrapper Layer                            │   │
│   │    Type-safe C++ wrappers around SAI C APIs (SaiAttribute, etc.)    │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                       SAI Object Store                              │   │
│   │    RAII wrappers and ref-counted storage for SAI objects            │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │ SAI API Calls
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Vendor SAI Implementation                           │
│                                                                             │
│   ┌───────────────────────────────────┐   ┌─────────────────────────────┐   │
│   │          Vendor1 SAI              │   │       Vendor2 SAI           │   │
│   │     (SAI Implementation)          │   │   (SAI Implementation)      │   │
│   └───────────────┬───────────────────┘   └──────────────┬──────────────┘   │
│                   │                                      │                  │
│   ┌───────────────▼───────────────────┐   ┌──────────────▼──────────────┐   │
│   │          Vendor1 SDK              │   │       Vendor2 SDK           │   │
│   └───────────────┬───────────────────┘   └──────────────┬──────────────┘   │
│                   │                                      │                  │
└───────────────────┼──────────────────────────────────────┼──────────────────┘
                    │                                      │
                    ▼                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ASIC Hardware                                  │
│                                                                             │
│   ┌───────────────────────────────────┐   ┌─────────────────────────────┐   │
│   │          Vendor1 ASIC             │   │       Vendor2 ASIC          │   │
│   └───────────────────────────────────┘   └─────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Layer Descriptions

#### SW Agent (SwSwitch)

The **SW Agent** is the software switch component that manages the overall switch state and coordinates with hardware.

**Key Responsibilities:**
- **Thrift Handler Layer**: Exposes `FbossCtrl` and `MultiSwitchCtrl` APIs for external services and CLI
- **Core Logic**: Contains feature-specific managers:
  - **RIB (Routing Information Base)**: Manages routing tables and route updates
  - **Neighbor Updater**: Handles ARP/NDP cache management
  - **Port Handler**: Manages port configuration and state
  - **ACL Handler**: Programs access control lists
  - **LLDP Manager**: Handles link layer discovery protocol
- **SwitchState**: A Copy-on-Write (COW) data structure that represents the entire switch configuration and operational state
- **MultiHwSwitchHandler**: Orchestrates state changes to one or more HW Agents

#### HW Agent (HwSwitchHandler)

The **HW Agent** is responsible for programming the actual hardware. In split-agent architecture, it runs as a separate process.

**Key Responsibilities:**
- Receives state deltas from SW Agent
- Translates state changes into hardware programming calls
- Reports hardware events (link state changes, FDB events) back to SW Agent
- Manages hardware-specific initialization and teardown

**Communication with SW Agent:**
- **Monolithic Mode**: Direct function calls within the same process
- **Split-Agent Mode**: Thrift streams for state synchronization (`MultiSwitchCtrl` service)
  - **OperDelta**: State changes from SW Agent to HW Agent
  - **Event Sinks**: Hardware events from HW Agent to SW Agent (FDB events, RX packets, link changes)

#### FBOSS SAI Layer (SaiSwitch)

The **FBOSS SAI Layer** implements the HwSwitch interface using the OCP (Open Compute Project) Switch Abstraction Interface (SAI).

**Components:**
- **SaiManagerTable**: Collection of managers for different SAI object types (ports, routes, neighbors, ACLs, queues, etc.)
- **SAI API Wrapper**: Type-safe C++ wrappers (`SaiAttribute`, `SaiApi`) around raw C SAI APIs
- **SAI Object Store**: RAII wrappers and ref-counted storage for SAI objects

**Key Features:**
- Provides a vendor-agnostic programming interface
- Handles object lifecycle management
- Supports warm boot through state serialization

#### Vendor SAI Implementation

Each hardware vendor provides their own SAI implementation that translates SAI API calls to their proprietary SDK:

- **Vendor SAI**: Implements the standard SAI APIs for the specific ASIC vendor
- **Vendor SDK**: The proprietary SDK provided by the ASIC vendor for low-level hardware access

#### ASIC Hardware

The actual forwarding ASIC that performs packet processing in hardware:
- Route lookups
- ACL matching
- QoS enforcement
- Packet forwarding at line rate

---

## CLI and Agent Communication

The `fboss2` CLI provides a unified interface for interacting with FBOSS services. Understanding how CLI communicates with the agent is essential for debugging and operations.

### Communication Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              fboss2 CLI                                     │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                       Command Handler                                 │  │
│  │   - Parses command arguments                                          │  │
│  │   - Validates input                                                   │  │
│  │   - Formats output                                                    │  │
│  └───────────────────────────────┬───────────────────────────────────────┘  │
│                                  │                                          │
│  ┌───────────────────────────────▼───────────────────────────────────────┐  │
│  │                       Thrift Client Factory                           │  │
│  │   createAgentClient()    createQsfpClient()    createFsdbClient()     │  │
│  └───────────────────────────────┬───────────────────────────────────────┘  │
└──────────────────────────────────┼──────────────────────────────────────────┘
                                   │ Thrift RPC (TCP/SSL)
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
        ▼                          ▼                          ▼
┌───────────────────┐    ┌───────────────────┐    ┌───────────────────┐
│   FBOSS Agent     │    │   QSFP Service    │    │      FSDB         │
│  (FbossCtrl API)  │    │ (QsfpService API) │    │  (FsdbService)    │
│                   │    │                   │    │                   │
│  Port: 5909       │    │  Port: 5910       │    │  Port: 5908       │
└───────────────────┘    └───────────────────┘    └───────────────────┘
```

### How CLI Queries Work

1. **Command Parsing**: User enters a command (e.g., `fboss2 show port`)
2. **Client Creation**: CLI creates a Thrift client to the appropriate service
3. **RPC Invocation**: CLI calls the corresponding Thrift API method
4. **Response Processing**: CLI formats and displays the response

### Example: Querying Port Information

```bash
# Show port status
fboss2 show port

# Show specific port
fboss2 show port eth1/1/1

# Show interface counters
fboss2 show interface counters
```

Under the hood:
1. CLI creates an `FbossCtrlAsyncClient` connected to the agent (port 5909)
2. Calls `getPortInfo()` or similar Thrift method
3. Agent queries SwitchState for port information
4. Response is formatted and displayed

### Thrift Services and Ports

| Service      | Default Port | Thrift Service Name | Purpose                            |
|--------------|--------------|---------------------|------------------------------------|
| Agent        | 5909         | FbossCtrl           | Switch management, routing, ports  |
| QSFP Service | 5910         | QsfpService         | Transceiver management             |
| FSDB         | 5908         | FsdbService         | State pub/sub                      |
| HW Agent     | 5931+        | FbossHwCtrl         | Direct HW queries (split-agent)    |

For complete CLI command reference and usage examples, see the [FBOSS CLI Documentation](/docs/debugging/fboss_cli/).

---

## Summary

The FBOSS Agent architecture is designed with the following principles:

1. **Modularity**: Clear separation between SW Agent, HW Agent, and SAI layers
2. **Vendor Abstraction**: SAI layer enables support for multiple ASIC vendors
3. **Scalability**: Multi-switch handler supports platforms with multiple ASICs
4. **Observability**: FSDB provides centralized state and stats for monitoring
5. **Operability**: fboss2 CLI provides comprehensive management interface

This layered architecture allows FBOSS to support a wide variety of network switches while maintaining a consistent programming model and operational experience.
