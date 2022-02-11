// Copyright 2004-present Facebook. All Rights Reserved.
//
#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include "common/services/cpp/ServiceFrameworkLight.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/SetupThrift.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

DEFINE_uint32(
    sensor_fetch_interval,
    5,
    "The interval between each sensor data fetch");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

void exportBuildInfo(int argc, char** argv);

int main(int argc, char** argv) {
  // Set version info
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  fb303::registerFollyLoggingOptionHandlers();

  exportBuildInfo(argc, argv);
  // Setup thrift handler and server
  auto [server, handler] = setupThrift();

  folly::FunctionScheduler scheduler;

  // To fetch sensor data at define cadence
  auto sensorService = handler->getServiceImpl();
  scheduler.addFunction(
      [sensorService]() { sensorService->fetchSensorData(); },
      std::chrono::seconds(FLAGS_sensor_fetch_interval),
      "fetchSensorData");

  scheduler.start();

  facebook::services::ServiceFrameworkLight service("Sensor Service");
  // Finally, run the Thrift server
  runServer(service, server, handler.get());

  return 0;
}
