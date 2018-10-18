#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.platform_config
namespace py.asyncio neteng.fboss.asyncio.platform_config
namespace cpp2 facebook.fboss.cfg

include "fboss/agent/hw/bcm/bcm_config.thrift"

union ChipConfig {
  1: bcm_config.BcmConfig bcm,
}

struct PlatformConfig {
  1: ChipConfig chip,
}
