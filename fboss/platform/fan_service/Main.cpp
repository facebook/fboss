// Copyright 2021- Facebook. All rights reserved.

#include <memory>
#include <mutex>
#include <string>

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/fan_service/FanServiceImpl.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::fan_service;

DEFINE_int32(thrift_port, 5972, "Port for the thrift service");
DEFINE_int32(
    control_interval,
    5,
    "How often we will read sensors and change fan pwm");
DEFINE_string(
    config_file,
    "",
    "Optional platform fan service configuration file. "
    "If not specified, default platform config will be used");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(argc, argv);

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto fanServiceImpl = std::make_unique<FanServiceImpl>(FLAGS_config_file);
  auto handler = std::make_shared<FanServiceHandler>(std::move(fanServiceImpl));

  folly::FunctionScheduler scheduler;
  scheduler.addFunction(
      [handler]() { handler->getFanServiceImpl()->controlFan(); },
      std::chrono::seconds(FLAGS_control_interval),
      "FanControl");
  scheduler.start();

  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(server, handler, "FanService", FLAGS_thrift_port);

  return 0;
}
