// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#endif
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/Flags.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/SensorStatsPub.h"
#include "fboss/platform/sensor_service/SetupThrift.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();

  helpers::fbInit(argc, argv);

  auto serviceImpl = std::make_shared<SensorServiceImpl>(FLAGS_config_path);

  // Fetch sensor data once to warmup
  serviceImpl->fetchSensorData();

  auto [server, handler] = helpers::setupThrift<SensorServiceThriftHandler>(
      serviceImpl, FLAGS_thrift_port);

  folly::FunctionScheduler scheduler;

  SensorStatsPub publisher(serviceImpl.get());

  scheduler.addFunction(
      [serviceImpl]() { serviceImpl->fetchSensorData(); },
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
  runServer(service, server, handler.get(), FLAGS_thrift_port);
#endif

  return 0;
}
