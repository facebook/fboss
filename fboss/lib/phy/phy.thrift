#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.phy
namespace py.asyncio neteng.fboss.asyncio.phy
namespace cpp2 facebook.fboss.phy

include "fboss/agent/switch_config.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

enum IpModulation {
  NRZ = 1,
  PAM4 = 2,
}

enum FecMode {
  NONE = 1,
  CL74 = 74,
  CL91 = 91,
  RS528 = 528,
  RS544 = 544,
  RS544_2N = 11,
}

enum InterfaceMode {
  // Backplane
  KR = 1
  KR2 = 2
  KR4 = 3
  // Copper
  CR = 10
  CR2 = 11
  CR4 = 12
  // Optics
  SR = 20
  SR4 = 21
  // CAUI
  CAUI = 30
  CAUI4 = 31
  CAUI4_C2C = 32
  CAUI4_C2M = 33
  // Other
  XLAUI = 40
  SFI = 41
  GMII = 42
}

enum Side {
  SYSTEM = 1,
  LINE = 2,
}

enum Loopback {
  ON = 1,
  OFF = 2,
}

struct TxSettings {
  1: i16 pre = 0
  2: i16 pre2 = 0
  3: i16 main = 0
  4: i16 post = 0
  5: i16 post2 = 0
  6: i16 post3 = 0
  7: i16 amp = 0
}

struct PolaritySwap {
  1: bool rx = 0
  2: bool tx = 0
}

const PolaritySwap NO_POLARITY_SWAP = {}

struct LaneSettings {
  // TODO: may need to extend this to be a mapping from speed to tx/rx
  // settings to support dynamically changing speed
  1: optional TxSettings tx
  3: PolaritySwap polaritySwap = NO_POLARITY_SWAP
}

struct PhyPortSideSettings {
  // map of lanes that are part of this port, along with any
  // lane-specific settings
  1: map<i16, LaneSettings> lanes
  2: IpModulation modulation
  3: FecMode fec
}

struct PhyPortSettings {
  1: switch_config.PortSpeed speed
  2: PhyPortSideSettings line
  3: PhyPortSideSettings system
  4: i16 phyID
}

struct PhyFwVersion {
  1: i32 version
  2: i32 crc
  3: optional string versionStr
  4: optional i32 dateCode
}

struct PhySettings {
  1: list<PhyPortSettings> ports = []
  2: optional PhyFwVersion expectedFw
}

struct Phys {
  1: map<i16, PhySettings> phys,
}


/*
 * ===== Port speed profile configs =====
 * The following structs are used to define the profile configs for a port
 * speed profile. Usually these config should be static across all ports in
 * the same platform. Each platform supported PortProfileID should map to a
 * PortProfileConfig.
 */
struct ProfileSideConfig {
  1: i16 numLanes
  2: IpModulation modulation
  3: FecMode fec
  4: optional transceiver.TransmitterTechnology medium
  5: optional InterfaceMode interfaceMode
}

struct PortProfileConfig {
  1: switch_config.PortSpeed speed
  2: ProfileSideConfig iphy
  3: optional ProfileSideConfig xphyLine
}

/*
 * ===== Port PinConnection configs =====
 * The following structs are used to define the fixed port PinConnection of the
 * PlatformPortMapping for a port. Usually these configs should be static
 * for any single port, and it won't change because of speed, medium or etc.
 * Here, we break down a platform port to a list of PinConnection, which can
 * start from ASIC(iphy), and then connect either directly to transceiver(
 * non-xphy case) or to xphy system side. And then xphy system can connect to
 * one or multiple line ends and finally the line end will connect to the
 * transceiver.
 */
struct PinID {
  // For iphy, chip means the core id of the ASIC
  // For xphy, chip means the external phy id
  // For transceiver, chip means the transciver id
  1: i32 chip
  2: i32 lane
}

struct PinJunction {
  1: PinID system
  2: list<PinConnection> line
}

union Pin {
  1: PinID end
  2: PinJunction junction
}

struct PinConnection {
  1: PinID a
  // Some platform might not have z side, like Galaxy Fabric Card
  2: optional Pin z
  3: optional PolaritySwap polaritySwap
}

/*
 * ===== Port dynamic Pin configs =====
 * The following structs are used to define the dynamic pin config of the
 * PlatformPortConfig for a port. Usually these configs will change accordingly
 * based on the speed, medium or any other factors.
 */
struct PinConfig {
  1: PinID id
  2: optional TxSettings tx
}

struct PortPinConfig {
  1: list<PinConfig> iphy
  2: optional list<PinConfig> transceiver
  3: optional list<PinConfig> xphySys
  4: optional list<PinConfig> xphyLine
}
