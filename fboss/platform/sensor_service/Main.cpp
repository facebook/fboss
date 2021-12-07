// Copyright 2004-present Facebook. All Rights Reserved.

#include "common/base/BuildInfo.h"
#include "common/init/Init.h"
#include "common/services/cpp/AclCheckerModule.h"
#include "common/services/cpp/BuildModule.h"
#include "common/services/cpp/BuildValues.h"
#include "common/services/cpp/Fb303Module.h"
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "common/services/cpp/ThriftAclCheckerModuleConfig.h"
#include "common/services/cpp/ThriftStatsModule.h"

#include <fb303/FollyLoggingHandler.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"
//#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
//#include "fboss/platform/sensor_service/SensorStatsPublisher.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

DEFINE_int32(thrift_port, 7001, "Port for the thrift service");

DEFINE_string(
    config_path,
    "/etc/sensor_service/darwin_sensor_config.json",
    "Platform Sensor Configuration File Path, e.g. /etc/sensor_service/darwin_sensor_config.json");

DEFINE_uint32(
    ods_pub_interval,
    15,
    "Interval for streaming sensor data to ODS, default is 15s");

DEFINE_uint32(
    sensor_fetch_interval,
    5,
    "The interval between each sensor data fetch");

DEFINE_string(
    ods_tier,
    "ods_router.script",
    "ODS Tier, default is ods_router.script");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char** argv) {
  // Set version info
  gflags::SetVersionString(BuildInfo::toDebugString());

  // Init SensorService

  std::shared_ptr<sensor_service::SensorServiceImpl> sensorService =
      std::make_shared<sensor_service::SensorServiceImpl>(FLAGS_config_path);

  // Fetch sensor data once to warmup
  sensorService->fetchSensorData();

  // Init FB and export build values
  initFacebook(&argc, &argv);
  services::BuildValues::setExportedValues();
  fb303::registerFollyLoggingOptionHandlers();

  // Setup thrift handler and server
  // auto handler =
  // std::make_shared<sensor_service::SensorServiceThriftHandler>(
  //    pSensorService);

  folly::FunctionScheduler scheduler;

  // To fetch sensor data at define cadence
  scheduler.addFunction(
      [&sensorService]() { sensorService->fetchSensorData(); },
      std::chrono::seconds(FLAGS_sensor_fetch_interval),
      "fetchSensorData");

  scheduler.start();

  // Finally, run the Thrift server

  return 0;
}
