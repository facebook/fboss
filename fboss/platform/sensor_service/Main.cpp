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

#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/SetupThrift.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

DECLARE_int32(thrift_port);
DEFINE_uint32(
    sensor_fetch_interval,
    5,
    "The interval between each sensor data fetch");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

// runServer : a helper function to run Sensor Service as Thrift Server.
int runServer(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    SensorServiceThriftHandler* handler) {
  facebook::services::ServiceFrameworkLight service("Sensor Service");
  thriftServer->setAllowPlaintextOnLoopback(true);
  service.addThriftService(thriftServer, handler, FLAGS_thrift_port);
  service.addModule(
      facebook::services::BuildModule::kModuleName,
      new facebook::services::BuildModule(&service));
  service.addModule(
      facebook::services::ThriftStatsModule::kModuleName,
      new facebook::services::ThriftStatsModule(&service));
  service.addModule(
      facebook::services::Fb303Module::kModuleName,
      new facebook::services::Fb303Module(&service));
  service.addModule(
      facebook::services::AclCheckerModule::kModuleName,
      new facebook::services::AclCheckerModule(&service));
  service.go();
  return 0;
}

int main(int argc, char** argv) {
  // Set version info
  gflags::SetVersionString(BuildInfo::toDebugString());
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  // Init FB and export build values
  initFacebook(&argc, &argv);
  services::BuildValues::setExportedValues();
  fb303::registerFollyLoggingOptionHandlers();

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

  // Finally, run the Thrift server
  int rc = runServer(server, handler.get());

  return rc;
}
