// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/SetupThrift.h"

namespace facebook::fboss::platform {

void startServiceAndRunServer(
    std::string serviceName,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler,
    bool loopForever) {}

// runServer : a helper function to run Fan Service as Thrift Server.
void runServer(
    facebook::services::ServiceFrameworkLight& /*service*/,
    std::shared_ptr<apache::thrift::ThriftServer> /*thriftServer*/,
    facebook::fboss::platform::FanServiceHandler* /*handler*/,
    bool /*loopForever*/) {}

} // namespace facebook::fboss::platform
