#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.phy
namespace py.asyncio neteng.fboss.asyncio.phy
namespace cpp2 facebook.fboss.phy

include "fboss/agent/switch_config.thrift"

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
}

struct PhySettings {
  1: list<PhyPortSettings> ports = []
  2: optional PhyFwVersion expectedFw
}

struct Phys {
  1: map<i16, PhySettings> phys,
}
