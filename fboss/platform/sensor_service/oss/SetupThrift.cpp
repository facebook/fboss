/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/SetupThrift.h"

namespace facebook::fboss::platform::sensor_service {

void runServer(
    facebook::services::ServiceFrameworkLight& /*service*/,
    std::shared_ptr<apache::thrift::ThriftServer> /*thriftServer*/,
    SensorServiceThriftHandler* /*handler*/,
    int32_t port,
    bool /*loopForever*/) {}

} // namespace facebook::fboss::platform::sensor_service
