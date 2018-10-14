#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.agent_config
namespace py.asyncio neteng.fboss.asyncio.agent_config
namespace cpp2 facebook.fboss.cfg

include "fboss/agent/switch_config.thrift"
include "fboss/agent/hw/bcm/bcm_config.thrift"

union ChipConfig {
  1: bcm_config.BcmConfig bcm,
}

struct AgentConfig {
  // This is used to override the default command line arguments we
  // pass to the agent.
  1: map<string, string> defaultCommandLineArgs = {},

  // This contains all of the hardware-independent configuration for a
  // single switch/router in the network.
  2: switch_config.SwitchConfig swConfig,

  // Chip-specific configuration.
  3: optional ChipConfig chipConfig;
}
