/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/SetupThrift.h"

#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"

#include "common/services/cpp/AclCheckerModule.h"
#include "common/services/cpp/BuildModule.h"
#include "common/services/cpp/Fb303Module.h"
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "common/services/cpp/ThriftAclCheckerModuleConfig.h"
#include "common/services/cpp/ThriftStatsModule.h"

DEFINE_int32(thrift_port, 7001, "Port for the thrift service");

DEFINE_string(
    config_path,
    "/etc/sensor_service/darwin_sensor_config.json",
    "Platform Sensor Configuration File Path, e.g. /etc/sensor_service/darwin_sensor_config.json");

namespace facebook::fboss::platform::sensor_service {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<SensorServiceThriftHandler>>
setupThrift() {
  // Init SensorService
  std::shared_ptr<SensorServiceImpl> sensorService =
      std::make_shared<SensorServiceImpl>(FLAGS_config_path);

  // Fetch sensor data once to warmup
  sensorService->fetchSensorData();

  // Setup thrift handler and server
  auto handler = std::make_shared<SensorServiceThriftHandler>(sensorService);
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);

  return {server, handler};
}

void runServer(
    facebook::services::ServiceFrameworkLight& service,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    SensorServiceThriftHandler* handler) {
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
}

} // namespace facebook::fboss::platform::sensor_service
