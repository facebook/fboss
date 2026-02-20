// Copyright 2021- Facebook. All rights reserved.

#include <memory>
#include <string>

#include <fb303/FollyLoggingHandler.h>
#include <folly/executors/FunctionScheduler.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/ConfigValidator.h"
#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/helpers/StructuredLogger.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fan_service;
using facebook::fboss::platform::helpers::StructuredLogger;

DEFINE_int32(thrift_port, 5972, "Port for the thrift service");
DEFINE_int32(
    control_interval,
    1,
    "How often we will check whether sensor read and pwm control is needed");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(&argc, &argv);

  auto platformName = helpers::PlatformNameLib().getPlatformName();

  try {
    std::string fanServiceConfJson =
        ConfigLib().getFanServiceConfig(platformName);
    auto config =
        apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
            fanServiceConfJson);
    XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
        config);
    if (!ConfigValidator().isValid(config)) {
      XLOG(ERR) << "Invalid config! Aborting...";
      throw std::runtime_error("Invalid Config.  Aborting...");
    }

    auto pBsp = std::make_shared<Bsp>(config);
    auto server = std::make_shared<apache::thrift::ThriftServer>();
    auto controlLogic = std::make_shared<ControlLogic>(config, pBsp);
    auto handler = std::make_shared<FanServiceHandler>(controlLogic);

    folly::FunctionScheduler scheduler;
    scheduler.addFunction(
        [controlLogic]() { controlLogic->controlFan(); },
        std::chrono::seconds(FLAGS_control_interval),
        "FanControl");
    scheduler.start();

    helpers::runThriftService(server, handler, "FanService", FLAGS_thrift_port);
  } catch (const std::exception& ex) {
    StructuredLogger structuredLogger("fan_service");
    structuredLogger.logAlert("unexpected_crash", ex.what());
    throw;
  }

  XLOG(INFO) << "================ STOPPED PLATFORM BINARY ================";
  return 0;
}
