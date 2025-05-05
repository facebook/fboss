// Copyright 2021- Facebook. All rights reserved.

#include <csignal>
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

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fan_service;

DEFINE_int32(thrift_port, 5972, "Port for the thrift service");
DEFINE_int32(
    control_interval,
    1,
    "How often we will check whether sensor read and pwm control is needed");

// This signal handler is used to stop the thrift server and service, resulting
// in a clean exit of the program ensuring all resources are released/destroyed.
std::shared_ptr<apache::thrift::ThriftServer> server;
void handleSignal(int sig) {
  XLOG(INFO) << "Received signal " << sig << ", shutting down...";
  if (server) {
    server->stop();
  }
}

int main(int argc, char** argv) {
  std::signal(SIGTERM, handleSignal);
  std::signal(SIGINT, handleSignal);
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
  if (!ConfigValidator().isValid(config)) {
    XLOG(ERR) << "Invalid config! Aborting...";
    throw std::runtime_error("Invalid Config.  Aborting...");
  }

  auto pBsp = std::make_shared<Bsp>(config);
  server = std::make_shared<apache::thrift::ThriftServer>();
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

  XLOG(INFO) << "FanService stopped";
  return 0;
}
