// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#endif
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/SensorStatsPub.h"
#include "fboss/platform/sensor_service/SetupThrift.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

DEFINE_uint32(
    sensor_fetch_interval,
    5,
    "The interval between each sensor data fetch");

DEFINE_int32(
    stats_publish_interval,
    60,
    "Interval (in seconds) for publishing stats");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {

  fb303::registerFollyLoggingOptionHandlers();

  helpers::fbInit(argc, argv);

  // Setup thrift handler and server
  auto [server, handler] = setupThrift();

  folly::FunctionScheduler scheduler;

  // To fetch sensor data at define cadence
  auto sensorService = handler->getServiceImpl();

  SensorStatsPub publisher(handler->getServiceImpl());

  scheduler.addFunction(
      [sensorService]() { sensorService->fetchSensorData(); },
      std::chrono::seconds(FLAGS_sensor_fetch_interval),
      "fetchSensorData");

  scheduler.addFunction(
      [&publisher]() { publisher.publishStats(); },
      std::chrono::seconds(FLAGS_stats_publish_interval),
      "SensorStatsPublish");

  scheduler.start();

#ifndef IS_OSS
  facebook::services::ServiceFrameworkLight service("Sensor Service");
  // Finally, run the Thrift server
  runServer(service, server, handler.get());
#endif

  return 0;
}
