#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.platform_config
namespace py.asyncio neteng.fboss.asyncio.platform_config
namespace cpp2 facebook.fboss.cfg

include "fboss/agent/hw/bcm/bcm_config.thrift"
include "fboss/agent/hw/sai/config/asic_config.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/agent/switch_config.thrift"

enum PlatformAttributes {
  CONNECTION_HANDLE = 1,
}

union ChipConfig {
  1: bcm_config.BcmConfig bcm,
  2: asic_config.AsicConfig asic,
}

struct FrontPanelResources {
  1: i32 transceiverId,
  2: list<i16> channels,
}

struct PlatformPortSettings {
  // Settings for configuring an external phy at the given speed, if the platform has external phys
  1: optional phy.PhyPortSettings xphy,

  // Corresponding front panel resources for the port at this speed
  2: optional FrontPanelResources frontPanelResources,

  // Ports that are subsumed and cannot be enabled at this speed
  3: list<i32> subsumedPorts = [],

  // Settings for asic side internal phy
  4: optional phy.PhyPortSideSettings iphy,
}

struct PlatformPort {
  1: i32 id,
  2: string name,
  // may need to extend this map to be keyed on speed + tranmsitter tech...
  3: map<switch_config.PortSpeed, PlatformPortSettings> supportedSpeeds,
}

struct PlatformConfig {
  1: ChipConfig chip,
  2: map<i32, PlatformPort> ports,
  3: map<PlatformAttributes, string> platformSettings,
}
