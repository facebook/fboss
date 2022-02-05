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
#include <folly/logging/Init.h>

#include "fboss/platform/rackmon/RackmonThriftHandler.h"

using namespace facebook;
using namespace facebook::services;
using namespace facebook::fboss::platform;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

DEFINE_int32(port, 7910, "Port for the thrift service");

int main(int argc, char** argv) {
  // Set version info
  gflags::SetVersionString(BuildInfo::toDebugString());

  // Init FB and export build values
  initFacebook(&argc, &argv);
  services::BuildValues::setExportedValues();
  fb303::registerFollyLoggingOptionHandlers();

  // Setup thrift handler and server
  auto handler = std::make_shared<rackmonsvc::ThriftHandler>();
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(handler);
  services::ServiceFrameworkLight service("Rackmon Service");
  /*
   * With TLS enforcements set to Required on Thrift Servers, plain text
   * traffic on local host will be dropped without this option. Allow plain
   * text traffic for local host communication over loopback.
   */
  server->setAllowPlaintextOnLoopback(true);
  service.addThriftService(server, handler.get(), FLAGS_port);

  /* Insert addon thrift modules*/
  service.addModule(BuildModule::kModuleName, new BuildModule(&service));
  service.addModule(
      ThriftStatsModule::kModuleName, new ThriftStatsModule(&service));
  service.addModule(Fb303Module::kModuleName, new Fb303Module(&service));
  auto aclCheckerModuleConfig =
      std::make_shared<ThriftAclCheckerModuleConfig>();
  // Skip ACL checks/enforcements for localhost communication.
  aclCheckerModuleConfig->setAclCheckerModuleSkipOnLoopback(true);
  service.addModule(
      AclCheckerModule::kModuleName,
      new AclCheckerModule(&service, aclCheckerModuleConfig));

  service.go();

  return 0;
}
