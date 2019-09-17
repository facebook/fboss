#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.agent_config
namespace py.asyncio neteng.fboss.asyncio.agent_config
namespace cpp2 facebook.fboss.cfg

include "fboss/agent/switch_config.thrift"
include "fboss/agent/platform_config.thrift"

struct AgentConfig {
  // This is used to override the default command line arguments we
  // pass to the agent.
  1: map<string, string> defaultCommandLineArgs = {},

  // This contains all of the hardware-independent configuration for a
  // single switch/router in the network.
  2: switch_config.SwitchConfig sw,

  // All configuration for the platform. This includes the chip
  // configuration (e.g broadcom config), as well as low-level port
  // tuning params.
  3: platform_config.PlatformConfig platform,
}
