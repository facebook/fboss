/*
 *  Copyright (c) 2004-present, Facebook, Inc.
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

#include "fboss/platform/data_corral_service/Flags.h"

namespace facebook::services {
class ServiceFrameworkLight;
}
namespace apache::thrift {
class ThriftServer;
}

namespace facebook::fboss::platform::data_corral_service {
class DataCorralServiceThriftHandler;

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<DataCorralServiceThriftHandler>>
setupThrift();

void runServer(
    facebook::services::ServiceFrameworkLight& service,
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    DataCorralServiceThriftHandler* handler,
    bool loopForever = true);
} // namespace facebook::fboss::platform::data_corral_service
