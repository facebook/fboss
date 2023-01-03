/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once
#include <gflags/gflags.h>
#include <memory>

namespace facebook::services {
class ServiceFrameworkLight;
}
namespace apache::thrift {
class ThriftServer;
}

DECLARE_int32(thrift_port);

namespace facebook::fboss::platform {
class FanServiceHandler;

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<FanServiceHandler>>
setupThrift();
// Helper function definition
void startServiceAndRunServer(
    std::string serviceName,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler,
    bool loopForever);
void runServer(
    facebook::services::ServiceFrameworkLight& service,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler,
    bool loopForever = true);
} // namespace facebook::fboss::platform
