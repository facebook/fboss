// Copyright 2021- Facebook. All rights reserved.

#include <memory>
#include <mutex>
#include <string>

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/platform/fan_service/FanService.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/helpers/Init.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;

DEFINE_int32(thrift_port, 5972, "Port for the thrift service");
DEFINE_int32(
    control_interval,
    5,
    "How often we will read sensors and change fan pwm");
DEFINE_string(mock_input, "", "Mock Input File");
DEFINE_string(mock_output, "", "Mock Output File");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();
  helpers::init(argc, argv);

  // If Mock configuration is enabled, run Fan Service in Mock mode, then quit.
  // No Thrift service will be created at all.
  if (FLAGS_mock_input != "") {
    facebook::fboss::platform::FanService mockedFanService;
    mockedFanService.kickstart();
    return mockedFanService.runMock(FLAGS_mock_input, FLAGS_mock_output);
  }

  auto serviceImpl = std::make_unique<facebook::fboss::platform::FanService>();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<FanServiceHandler>(std::move(serviceImpl));
  handler->getFanService()->kickstart();

  folly::FunctionScheduler scheduler;
  scheduler.addFunction(
      [handler]() { handler->getFanService()->controlFan(); },
      std::chrono::seconds(FLAGS_control_interval),
      "FanControl");
  scheduler.start();

  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(server, handler, "FanService", FLAGS_thrift_port);

  return 0;
}
