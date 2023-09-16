#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace cpp2 facebook.fboss.cfg
namespace go neteng.fboss.led_service_config
namespace py neteng.fboss.led_service_config
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.led_service_config

struct LedServiceConfig {
  // This is used to override the default command line arguments we
  // pass to led_service.
  1: map<string, string> defaultCommandLineArgs = {};

  // Is the LED control through led_service enabled on the platform
  2: bool ledControlledThroughService = false;
}
