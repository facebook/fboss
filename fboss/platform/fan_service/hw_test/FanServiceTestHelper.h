// Copyright 2021- Facebook. All rights reserved.
// This is the common header file included by all of
// 1. Test handler file
// 2. Meta specific implementation
// 3. OSS specific implementation
#include "fboss/platform/fan_service/FanService.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/fan_service/HelperFunction.h"
#include "fboss/platform/fan_service/Mokujin.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/ServiceConfig.h"
#include "fboss/platform/fan_service/SetupThrift.h"

namespace facebook::fboss::platform {
// Meta specific or OSS specific function definition
// will be listed here
void fsTestSetUp(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler);
void fsTestTearDown(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler);
} // namespace facebook::fboss::platform
