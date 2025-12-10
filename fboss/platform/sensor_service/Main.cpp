// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>
#include <folly/executors/FunctionScheduler.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/Flags.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/Utils.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

int main(int argc, char** argv) {
  fb303::registerFollyLoggingOptionHandlers();

  helpers::init(&argc, &argv);

  SensorConfig sensorConfig = Utils().getConfig();
  auto serviceImpl = std::make_shared<SensorServiceImpl>(sensorConfig);

  // Fetch sensor data once to warmup
  serviceImpl->fetchSensorData();

  folly::FunctionScheduler scheduler;

  scheduler.addFunction(
      [serviceImpl]() { serviceImpl->fetchSensorData(); },
      std::chrono::seconds(FLAGS_sensor_fetch_interval),
      "fetchSensorData");

  scheduler.start();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<SensorServiceThriftHandler>(serviceImpl);
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);

  auto evb = server->getEventBaseManager()->getEventBase();
  helpers::SignalHandler signalHandler(evb, server);

  helpers::runThriftService(
      server, handler, "SensorService", FLAGS_thrift_port);

  XLOG(INFO) << "================ STOPPED PLATFORM BINARY ================";
  return 0;
}
