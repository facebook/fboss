// Copyright 2004-present Facebook. All Rights Reserved.
//
//
#include "fboss/platform/helpers/Init.h"

namespace facebook::fboss::platform::helpers {
void fbInit(int /*argc*/, char** /*argv*/) {}
void addCommonModules(facebook::services::ServiceFrameworkLight& /*service*/) {}

void runThriftService(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<apache::thrift::ServerInterface> /* handler */,
    const std::string& /* serviceName */,
    uint32_t /* port */) {
  server->serve();
}
} // namespace facebook::fboss::platform::helpers
