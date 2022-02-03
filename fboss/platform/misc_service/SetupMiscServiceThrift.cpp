/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/misc_service/SetupMiscServiceThrift.h"

#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "fboss/platform/misc_service/MiscServiceImpl.h"
#include "fboss/platform/misc_service/MiscServiceThriftHandler.h"

#include "common/services/cpp/AclCheckerModule.h"
#include "common/services/cpp/BuildModule.h"
#include "common/services/cpp/Fb303Module.h"
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "common/services/cpp/ThriftAclCheckerModuleConfig.h"
#include "common/services/cpp/ThriftStatsModule.h"

DEFINE_int32(thrift_port, 5911, "Port for the thrift service");

namespace facebook::fboss::platform::misc_service {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<MiscServiceThriftHandler>>
setupThrift() {
  // Init MiscService
  std::shared_ptr<MiscServiceImpl> miscService =
      std::make_shared<MiscServiceImpl>();

  // Fetch data once to warmup
  // ToDo miscService->fetchData();

  // Setup thrift handler and server
  auto handler = std::make_shared<MiscServiceThriftHandler>(miscService);
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);

  return {server, handler};
}

void runServer(
    facebook::services::ServiceFrameworkLight& service,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    MiscServiceThriftHandler* handler,
    bool loopForever) {
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
  service.go(loopForever);
}

} // namespace facebook::fboss::platform::misc_service
