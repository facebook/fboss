#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.platform_config
namespace py.asyncio neteng.fboss.asyncio.platform_config
namespace cpp2 facebook.fboss.cfg

include "fboss/agent/hw/bcm/bcm_config.thrift"
include "fboss/lib/phy/external_phy.thrift"
include "fboss/agent/switch_config.thrift"

union ChipConfig {
  1: bcm_config.BcmConfig bcm,
}

struct FrontPanelResources {
  1: i32 transceiverId,
  2: list<i16> channels,
}

struct PlatformPortSettings {
  // Settings for configuring an external phy at the given speed, if the platform has external phys
  1: optional external_phy.ExternalPhyPortSettings xphy,

  // Corresponding front panel resources for the port at this speed
  2: optional FrontPanelResources frontPanelResources,

  // Ports that are subsumed and cannot be enabled at this speed
  3: list<i32> subsumedPorts = [],

  // Settings for asic side internal phy
  4: optional external_phy.ExternalPhyPortSideSettings iphy,
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
}
