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

struct PlatformPortSettings {
  1: optional external_phy.ExternalPhyPortSettings xphy,
  // TODO: fill in more stuff here
}

struct PlatformPort {
  1: i32 id,
  2: string name,
  3: map<switch_config.PortSpeed, PlatformPortSettings> supportedSpeeds,
}

struct PlatformConfig {
  1: ChipConfig chip,
  2: map<i32, PlatformPort> ports,
}
