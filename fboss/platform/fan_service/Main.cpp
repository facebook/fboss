// Copyright 2021- Facebook. All rights reserved.

#include <memory>
#include <mutex>
#include <string>

#include <fb303/FollyLoggingHandler.h>
#include <folly/executors/FunctionScheduler.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/fan_service/Utils.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fan_service;

DEFINE_int32(thrift_port, 5972, "Port for the thrift service");
DEFINE_int32(
    control_interval,
    1,
    "How often we will check whether sensor read and pwm control is needed");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(&argc, &argv);

  auto platformName = helpers::PlatformNameLib().getPlatformName();
  std::string fanServiceConfJson =
      ConfigLib().getFanServiceConfig(platformName);
  auto config =
      apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
          fanServiceConfJson);
  XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config);
  if (!Utils().isValidConfig(config)) {
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

  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(server, handler, "FanService", FLAGS_thrift_port);

  return 0;
}
