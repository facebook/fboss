// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/Flags.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/SensorStatsPub.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();

  helpers::init(argc, argv);

  auto serviceImpl = std::make_shared<SensorServiceImpl>(FLAGS_config_path);

  // Fetch sensor data once to warmup
  serviceImpl->fetchSensorData();

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

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<SensorServiceThriftHandler>(serviceImpl);
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);
  helpers::runThriftService(
      server, handler, "SensorService", FLAGS_thrift_port);

  return 0;
}
