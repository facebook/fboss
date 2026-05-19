# FBOSS CPO and ELSFP Support - High Level Design

## Table of Contents
- [Revision](#revision)
- [About This Document](#about-this-document)
- [Scope](#scope)
- [Definitions/Abbreviations](#definitionsabbreviations)
- [1. Overview](#1-overview)
- [2. Requirements](#2-requirements)
- [3. Architecture](#3-architecture)
- [4. Detailed Design](#4-detailed-design)
  - [4.1 Transceiver Type Extension](#41-transceiver-type-extension)
  - [4.2 CPO DOM Monitoring](#42-cpo-dom-monitoring)
  - [4.3 CPO Port Lane Mapping](#43-cpo-port-lane-mapping)
  - [4.4 CPO Joint Mode](#44-cpo-joint-mode)
  - [4.5 ELSFP Memory Map and Paging](#45-elsfp-memory-map-and-paging)
  - [4.6 ELSFP Banking Support](#46-elsfp-banking-support)
- [5. Configuration](#5-configuration)
- [6. CLI Commands](#6-cli_commands)
- [7. YANG/Thrift Changes](#7-yangthrift-changes)
- [8. Redis DB Schema Changes](#8-redis-db-schema-changes)
- [9. Platform Mapping Changes](#9-platform-mapping-changes)
- [10. Testing Plan](#10-testing-plan)
- [References](#references)

---

## Revision

| Rev | Date        | Author      | Description                   |
|-----|-------------|-------------|-------------------------------|
| 0.1 | 2025-04-21 | FBOSS Team  | Initial HLD for CPO/ELSFP    |

---

## About This Document

This document provides a High Level Design for adding Co-Packaged Optics (CPO) and External Laser SFP (ELSFP) support to the FBOSS (Facebook Open Switching System) platform.

## Scope

This HLD covers:
- Extension of FBOSS transceiver types to support ELSFP
- CPO DOM (Digital Optical Monitoring) with OE/ELSFP separation
- CPO port lane mapping with interleave modes
- CPO joint mode for hardware-controlled CPO
- ELSFP memory map and paging support
- ELSFP banking support for CMIS modules
- Platform mapping changes for CPO platforms

## Definitions/Abbreviations

| Term     | Definition                                      |
|----------|-------------------------------------------------|
| CPO      | Co-Packaged Optics                              |
| ELSFP    | External Laser SFP                              |
| OE       | Optical Engine                                  |
| ELSP     | External Laser Source                           |
| LPO      | Linear Pluggable Optics                         |
| DOM      | Digital Optical Monitoring                      |
| CMIS     | Common Management Interface Specification       |
| NPU      | Network Processing Unit                         |
| QSFP     | Quad Small Form-factor Pluggable                |
| OSFP     | Octal Small Form-factor Pluggable               |

---

## 1. Overview

Co-Packaged Optics (CPO) and External Laser SFP (ELSFP) are emerging technologies for high-speed optical interconnects in data center switches. CPO integrates optical engines directly on the switch PCB, while ELSFP uses an external laser source separate from the transceiver module.

This document describes the changes needed in FBOSS to support these technologies, building on the SONiC implementation as a reference.

### CPO Architecture

```
+---------------------------------------------+
|              Switch PCB                     |
|                                             |
|  +----------+        +------------------+  |
|  |   ASIC   |--------|  Optical Engine  |  |
|  |          |  electrical |  (OE)       |  |
|  +----------+        +---------+--------+  |
|                              |              |
|                       +------v------+       |
|                       |   ELSFP     |       |
|                       |  (Laser)    |       |
|                       +-------------+       |
+---------------------------------------------+
```

### ELSFP Architecture

```
+------------------+     Fiber      +------------------+
|   Transceiver    |<-------------->|   ELSFP (Laser)  |
|   (Modulator)    |                |   (External)     |
+---------+--------+                +---------+--------+
          |                                   |
          +------------+   +------------------+
                       |   |
                +------v---v------+
                |   Laser Fiber   |
                |   Connection    |
                +-----------------+
```

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority |
|----|-------------|----------|
| R1 | Support ELSFP as a distinct transceiver type | P0 |
| R2 | CPO DOM monitoring with separate OE and ELSFP readings | P0 |
| R3 | CPO port lane mapping with interleave modes | P0 |
| R4 | CPO joint mode for hardware-controlled CPO | P1 |
| R5 | ELSFP memory map and paging support | P0 |
| R6 | ELSFP banking support for CMIS modules | P0 |
| R7 | CLI commands for CPO/ELSFP management | P0 |
| R8 | Platform mapping changes for CPO platforms | P0 |
| R9 | Thrift model extensions for CPO/ELSFP | P0 |

### 2.2 Non-Functional Requirements

| ID | Requirement | Priority |
|----|-------------|----------|
| NR1 | Backward compatible with existing transceiver types | P0 |
| NR2 | No performance impact on non-CPO platforms | P0 |
| NR3 | Thread-safe DOM monitoring for CPO/ELSFP | P0 |

---

## 3. Architecture

### 3.1 Component Diagram

```
+---------------------------------------------------------+
|                    FBOSS System                         |
|                                                         |
|  +------------------+    +-------------------------+   |
|  |   fboss_agent    |    |   qsfp_service          |   |
|  |                  |    |                         |   |
|  |  +------------+  |    |  +------------------+   |   |
|  |  | SwSwitch   |  |    |  | TransceiverManager|   |   |
|  |  +-----+------+  |    |  +--------+---------+   |   |
|  |        |         |    |           |             |   |
|  |  +-----v------+  |    |  +--------v---------+   |   |
|  |  | HwSwitch   |  |    |  | CpoDomMonitor    |   |   |
|  |  | (SAI/Bcm)  |  |    |  | (new)            |   |   |
|  |  +-----+------+  |    |  +--------+---------+   |   |
|  |        |         |    |           |             |   |
|  |  +-----v------+  |    |  +--------v---------+   |   |
|  |  | Platform   |  |    |  | ElsfpMemoryMap   |   |   |
|  |  | Mapping    |  |    |  | (new)            |   |   |
|  |  +------------+  |    |  +------------------+   |   |
|  |                  |    |                         |   |
|  +------------------+    +-------------------------+   |
|           |                        |                   |
|  +--------v---------+    +---------v----------+       |
|  | Platform Config  |    | led_service       |       |
|  | (cpo_platform)   |    | (CpoLedManager)   |       |
|  +------------------+    +--------------------+       |
|                                                         |
+---------------------------------------------------------+
```

### 3.2 Module Interactions

```
+----------+     +----------+     +------------+     +--------+
| Platform |---->| QSFP     |---->| Transceiver |---->| SFP    |
| Mapping  |     | Service  |     | Manager     |     | Module |
+----------+     +----------+     +-----+------+     +---+----+
                                         |                |
                               +---------v----+     +-----v-----+
                               | CpoDomMonitor|     | CMIS      |
                               | (OE/ELSFP)   |     | Banking   |
                               +--------------+     +-----------+
```

---

## 4. Detailed Design

### 4.1 Transceiver Type Extension

**File:** `fboss/qsfp_service/if/transceiver.thrift`

Add new transceiver types:

```thrift
enum TransceiverType {
  ...
  QSFP = 1,
  OSFP = 2,
  SFP = 3,
  ELSFP = 4,      // NEW: External Laser SFP
  CPO_OE = 5,     // NEW: CPO Optical Engine
}

enum LaserSourceType {
  INTERNAL = 0,    // Traditional integrated laser
  EXTERNAL = 1,    // External laser (ELSFP)
  UNKNOWN = 2,
}
```

### 4.2 CPO DOM Monitoring

**File:** `fboss/qsfp_service/if/transceiver.thrift`

Extend DOM structure to support CPO:

```thrift
struct OpticalEngineDom {
  1: double temperature,
  2: list<double> txPowers,
  3: list<double> rxPowers,
  4: double voltage,
  5: list<double> snr,
  6: list<bool> laneAlarmFlags,
}

struct ElsfpDom {
  1: double temperature,
  2: double laserBiasCurrent,
  3: double opticalOutputPower,
  4: bool laserReady,
  5: bool laserFault,
}

struct CpoDomData {
  1: OpticalEngineDom oeDom,
  2: ElsfpDom elsfpDom,
  3: TransceiverType transceiverType,
  4: LaserSourceType laserSourceType,
}
```

### 4.3 CPO Port Lane Mapping

**File:** `fboss/agent/platforms/common/PlatformMapping.h`

Add CPO port lane mapping support:

```cpp
enum class InterleaveMode {
  NONE = 0,      // 1:1 port to lane mapping
  MODE1 = 1,     // Ports share lanes across different transceivers
  MODE2 = 2,     // Complex interleaving for CPO
};

struct CpoPortLaneMap {
  PortID portId;
  std::vector<int> laneIds;
  InterleaveMode interleaveMode;
  bool isCpoPort;       // True if this is a CPO port (OE connected)
  bool isElsfpPort;     // True if this uses ELSFP
};
```

### 4.4 CPO Joint Mode

**File:** `fboss/qsfp_service/platforms/wedge/WedgeManager.h`

Add joint mode support:

```cpp
enum class CpoJointMode {
  DISABLED = 0,
  HARDWARE = 1,   // HW-controlled CPO joint mode
  SOFTWARE = 2,   // SW-controlled CPO joint mode
};

class CpoManager {
 public:
  void setJointMode(PortID port, CpoJointMode mode);
  CpoJointMode getJointMode(PortID port);
  bool isJointModeSupported(PortID port);
};
```

### 4.5 ELSFP Memory Map and Paging

**File:** `fboss/lib/phy/CmisModule.h`

Extend CMIS module for ELSFP:

```cpp
class ElsfpMemoryMap {
 public:
  // ELSFP-specific registers (Page 01h)
  static constexpr uint8_t ELSFP_STATUS = 128;
  static constexpr uint8_t LASER_TEMP_HIGH = 130;
  static constexpr uint8_t LASER_TEMP_LOW = 131;
  static constexpr uint8_t LASER_BIAS_HIGH = 132;
  static constexpr uint8_t LASER_BIAS_LOW = 133;
  static constexpr uint8_t OPTICAL_OUTPUT_HIGH = 134;
  static constexpr uint8_t OPTICAL_OUTPUT_LOW = 135;
  static constexpr uint8_t LASER_CONTROL = 136;

  bool readElsfpStatus();
  double readLaserTemperature();
  double readLaserBiasCurrent();
  double readOpticalOutputPower();
  void setLaserControl(bool enable);
};
```

### 4.6 ELSFP Banking Support

**File:** `fboss/lib/phy/CmisModule.h`

Add banking support:

```cpp
class CmisBanking {
 public:
  static constexpr uint8_t BANK_SELECT_REG = 0xA0;
  static constexpr uint8_t NUM_BANKS = 4;

  void selectBank(uint8_t bank);
  uint8_t getCurrentBank();
  bool isBankSupported(uint8_t bank);

  // Read from banked page
  std::vector<uint8_t> readBankedPage(uint8_t bank, uint8_t page, uint8_t offset, uint8_t length);

  // Write to banked page
  void writeBankedPage(uint8_t bank, uint8_t page, uint8_t offset, const std::vector<uint8_t>& data);
};
```

---

## 5. Configuration

### 5.1 CPO Platform Configuration

**File:** `fboss/platform/configs/<platform>/platform_manager.json`

```json
{
  "cpoConfig": {
    "enabled": true,
    "interleaveMode": "MODE1",
    "oeType": "BAILLY",
    "elsfpType": "ELSFP_400G",
    "jointModeDefault": "HARDWARE",
    "domPollingIntervalMs": 1000,
    "oeDomThresholds": {
      "temperatureHigh": 75.0,
      "temperatureLow": -10.0,
      "txPowerHigh": 3.0,
      "txPowerLow": -10.0,
      "rxPowerHigh": 3.0,
      "rxPowerLow": -15.0
    },
    "elsfpDomThresholds": {
      "temperatureHigh": 70.0,
      "laserBiasHigh": 80.0,
      "opticalOutputHigh": 5.0,
      "opticalOutputLow": -5.0
    }
  }
}
```

### 5.2 CPO Port Configuration

**File:** `fboss/platform/configs/<platform>/cpo_port_config.json`

```json
{
  "cpoPortConfig": {
    "portLaneMaps": [
      {
        "portId": 1,
        "laneIds": [1, 2, 3, 4],
        "interleaveMode": "NONE",
        "isCpoPort": true,
        "isElsfpPort": true
      },
      {
        "portId": 2,
        "laneIds": [5, 6, 7, 8],
        "interleaveMode": "MODE1",
        "isCpoPort": true,
        "isElsfpPort": true
      }
    ]
  }
}
```

---

## 6. CLI Commands

### 6.1 CPO Management Commands

| Command | Description | Example |
|---------|-------------|---------|
| `fboss cpoe show dom <port>` | Show CPO DOM for OE and ELSFP | `fboss cpoe show dom eth1/1/1` |
| `fboss cpoe show port-lane-map` | Show CPO port lane mapping | `fboss cpoe show port-lane-map` |
| `fboss cpoe set joint-mode <port> <mode>` | Set CPO joint mode | `fboss cpoe set joint-mode eth1/1/1 hardware` |
| `fboss cpoe show joint-mode <port>` | Show joint mode status | `fboss cpoe show joint-mode eth1/1/1` |
| `fboss cpoe show status` | Show all CPO ports status | `fboss cpoe show status` |

### 6.2 ELSFP Management Commands

| Command | Description | Example |
|---------|-------------|---------|
| `fboss elsfp show status <port>` | Show ELSFP connection status | `fboss elsfp show status eth1/1/1` |
| `fboss elsfp read-memory <port> <page>` | Read ELSFP memory page | `fboss elsfp read-memory eth1/1/1 0x01` |
| `fboss elsfp read-register <port> <addr>` | Read ELSFP register | `fboss elsfp read-register eth1/1/1 0x80` |
| `fboss elsfp show banks <port>` | Show ELSFP bank info | `fboss elsfp show banks eth1/1/1` |
| `fboss elsfp select-bank <port> <bank>` | Select ELSFP bank | `fboss elsfp select-bank eth1/1/1 0` |

### 6.3 Command Examples

```bash
# Show CPO DOM for a port
$ fboss cpoe show dom eth1/1/1
Port: eth1/1/1
  Optical Engine (OE):
    Temperature: 45.2 C
    Tx Power (Lane 0): -2.1 dBm
    Tx Power (Lane 1): -2.3 dBm
    Tx Power (Lane 2): -2.0 dBm
    Tx Power (Lane 3): -2.2 dBm
    Rx Power (Lane 0): -3.5 dBm
    Rx Power (Lane 1): -3.2 dBm
    Rx Power (Lane 2): -3.4 dBm
    Rx Power (Lane 3): -3.3 dBm
    Voltage: 3.3 V
    SNR: [12.5, 12.3, 12.4, 12.6] dB
  ELSFP (External Laser):
    Connection: Connected
    Temperature: 35.8 C
    Laser Bias: 45.2 mA
    Optical Output: 2.1 dBm
    Laser Ready: Yes
    Laser Fault: No

# Show CPO port lane mapping
$ fboss cpoe show port-lane-map
Port        Lane IDs         Interleave    CPO    ELSFP
----------  ---------------  ------------  -----  -------
eth1/1/1    [1,2,3,4]       NONE          Yes    Yes
eth1/2/1    [5,6,7,8]       MODE1         Yes    Yes
eth1/3/1    [9,10,11,12]    NONE          Yes    Yes
eth1/4/1    [13,14,15,16]   MODE1         Yes    Yes

# Set joint mode
$ fboss cpoe set joint-mode eth1/1/1 hardware
Successfully set joint mode to HARDWARE on eth1/1/1

# Show ELSFP status
$ fboss elsfp show status eth1/1/1
Port: eth1/1/1
  ELSFP Status: Connected
  Laser Type: External
  Temperature: 35.8 C
  Bias Current: 45.2 mA
  Output Power: 2.1 dBm
  Laser Ready: Yes
  Fault Status: No Fault

# Read ELSFP memory
$ fboss elsfp read-memory eth1/1/1 0x01 --offset 128 --length 8
Page 0x01, Offset 128:
  0x80: 0x01 (ELSFP Status: Connected)
  0x81: 0x00
  0x82: 0x23 (Laser Temp High: 35)
  0x83: 0x33 (Laser Temp Low: 51)
  0x84: 0x00 (Bias High)
  0x85: 0x2D (Bias Low: 45)
  0x86: 0x00 (Output High)
  0x87: 0x15 (Output Low: 21)
```

---

## 7. Thrift Changes

### 7.1 Transceiver Type Extension

**File:** `fboss/qsfp_service/if/transceiver.thrift`

```thrift
// Existing types
enum TransceiverType {
  SFP = 0,
  QSFP = 1,
  OSFP = 2,
  SFP_DD = 3,
  // New types for CPO/ELSFP
  ELSFP = 4,
  CPO_OE = 5,
}

enum LaserSourceType {
  UNKNOWN = 0,
  INTERNAL = 1,
  EXTERNAL = 2,
}

enum CpoInterleaveMode {
  NONE = 0,
  MODE1 = 1,
  MODE2 = 2,
}

enum CpoJointMode {
  DISABLED = 0,
  HARDWARE = 1,
  SOFTWARE = 2,
}

struct OpticalEngineDomData {
  1: double temperature,
  2: list<double> txPowers,
  3: list<double> rxPowers,
  4: double voltage,
  5: list<double> snr,
  6: list<LaneAlarmFlags> laneAlarmFlags,
}

struct ElsfpDomData {
  1: double temperature,
  2: double laserBiasCurrent,
  3: double opticalOutputPower,
  4: bool laserReady,
  5: bool laserFault,
}

struct CpoPortConfig {
  1: PortID portId,
  2: list<i32> laneIds,
  3: CpoInterleaveMode interleaveMode,
  4: bool isCpoPort,
  5: bool isElsfpPort,
}

struct CpoConfig {
  1: bool enabled,
  2: CpoInterleaveMode interleaveMode,
  3: string oeType,
  4: string elsfpType,
  5: CpoJointMode jointModeDefault,
  6: i32 domPollingIntervalMs,
}
```

### 7.2 QSFP Service Thrift Changes

**File:** `fboss/qsfp_service/if/qsfp.thrift`

```thrift
service QsfpService {
  // Existing methods
  ...

  // NEW: CPO DOM methods
  CpoDomData getCpoDomData(1: PortID port);
  list<CpoDomData> getAllCpoDomData();

  // NEW: CPO Joint Mode methods
  void setCpoJointMode(1: PortID port, 2: CpoJointMode mode);
  CpoJointMode getCpoJointMode(1: PortID port);

  // NEW: CPO Port Lane Map methods
  list<CpoPortConfig> getCpoPortLaneMap();

  // NEW: ELSFP methods
  ElsfpDomData getElsfpDomData(1: PortID port);
  bool getElsfpStatus(1: PortID port);
  list<byte> readElsfpMemory(1: PortID port, 2: byte page, 3: byte offset, 4: byte length);

  // NEW: ELSFP Banking methods
  void selectElsfpBank(1: PortID port, 2: byte bank);
  byte getCurrentElsfpBank(1: PortID port);
}
```

---

## 8. Redis DB Schema Changes

### 8.1 CPO DOM Table

```
Table: TRANSCEIVER_CPO_DOM_SENSOR
Key: <port_name>
Fields:
  - oe_temperature (float, Celsius)
  - oe_tx_power_lane<N> (float, dBm)
  - oe_rx_power_lane<N> (float, dBm)
  - oe_voltage (float, V)
  - oe_snr_lane<N> (float, dB)
  - elsfp_connected (bool)
  - elsfp_temperature (float, Celsius)
  - elsfp_laser_bias (float, mA)
  - elsfp_optical_output (float, dBm)
  - elsfp_laser_ready (bool)
  - elsfp_laser_fault (bool)
```

### 8.2 CPO Port Lane Map Table

```
Table: TRANSCEIVER_CPO_PORT_LANE_MAP
Key: <port_name>
Fields:
  - port_id (int)
  - lane_ids (list of int, space-separated)
  - interleave_mode (string: NONE, MODE1, MODE2)
  - is_cpo_port (bool)
  - is_elsfp_port (bool)
```

### 8.3 CPO Joint Mode Table

```
Table: TRANSCEIVER_CPO_JOINT_MODE
Key: <port_name>
Fields:
  - joint_mode (string: DISABLED, HARDWARE, SOFTWARE)
  - supported (bool)
```

### 8.4 ELSFP Banking Table

```
Table: TRANSCEIVER_ELSFP_BANK
Key: <port_name>
Fields:
  - current_bank (int)
  - num_banks (int)
  - bank_0_page_11h (hex string)
  - bank_1_page_11h (hex string)
```

---

## 9. Platform Mapping Changes

### 9.1 CPO Platform Mapping JSON

**File:** `fboss/agent/platforms/common/<platform>/<Platform>PlatformMapping.cpp`

```json
{
  "ports": {
    "1": {
      "mapping": {
        "id": 1,
        "name": "eth1/1/1",
        "controllingPort": 1,
        "pins": [...],
        "scope": 0,
        "cpoConfig": {
          "isCpoPort": true,
          "interleaveMode": "NONE",
          "elsfpLaneIds": [1, 2, 3, 4]
        }
      },
      "supportedProfiles": {...}
    }
  }
}
```

### 9.2 BSP Mapping for CPO/ELSFP

**File:** `fboss/lib/bsp/<platform>/<Platform>BspPlatformMapping.cpp`

```json
{
  "pimMapping": {
    "1": {
      "pimID": 1,
      "tcvrMapping": {
        "1": {
          "tcvrId": 1,
          "transceiverType": "CPO_OE",
          "laserSourceType": "EXTERNAL",
          "accessControl": {...},
          "io": {...},
          "tcvrLaneToLedId": {...},
          "cpoConfig": {
            "jointModeSupported": true,
            "defaultJointMode": "HARDWARE"
          }
        }
      }
    }
  }
}
```

---

## 10. Testing Plan

### 10.1 Unit Tests

| Test | Description |
|------|-------------|
| CpoDomMonitorTest | Test CPO DOM data collection |
| ElsfpMemoryMapTest | Test ELSFP memory read operations |
| CmisBankingTest | Test CMIS bank selection and access |
| CpoPortLaneMapTest | Test CPO port lane mapping |
| CpoJointModeTest | Test joint mode configuration |

### 10.2 Hardware Tests

| Test | Description |
|------|-------------|
| CpoDomHwTest | Verify CPO DOM readings on hardware |
| ElsfpConnectionTest | Verify ELSFP connection detection |
| CpoJointModeHwTest | Verify joint mode on hardware |
| ElsfpBankingHwTest | Verify banking operations on hardware |
| CpoLedTest | Verify LED management for CPO ports |

### 10.3 Integration Tests

| Test | Description |
|------|-------------|
| CpoPlatformMappingTest | Verify CPO platform mapping |
| CpoQsfpServiceTest | Verify QSFP service with CPO |
| ElsfpTransceiverManagerTest | Verify transceiver manager with ELSFP |

---

## References

- [SONiC CPO Support PR #2152](https://github.com/sonic-net/SONiC/pull/2152)
- [SONiC ELSFP Memory Map PR #2207](https://github.com/sonic-net/SONiC/pull/2207)
- [SONiC CPO Port Mapping PR #2211](https://github.com/sonic-net/SONiC/pull/2211)
- [SONiC CPO Joint Mode PR #2273](https://github.com/sonic-net/SONiC/pull/2273)
- [SONiC CPO DOM Monitoring PR #2297](https://github.com/sonic-net/SONiC/pull/2297)
- [SONiC CMIS Banking PR #2183](https://github.com/sonic-net/SONiC/pull/2183)
- [SONiC CMIS LPO Enhancement PR #2205](https://github.com/sonic-net/SONiC/pull/2205)
