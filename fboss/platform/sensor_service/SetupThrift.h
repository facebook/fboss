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
#include <memory>

namespace apache::thrift {
class ThriftServer;
}

namespace facebook::fboss::platform::sensor_service {
class SensorServiceThriftHandler;

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<SensorServiceThriftHandler>>
setupThrift();

void runServer(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    SensorServiceThriftHandler* handler);
} // namespace facebook::fboss::platform::sensor_service
