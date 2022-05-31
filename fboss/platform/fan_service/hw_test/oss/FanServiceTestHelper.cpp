// Copyright 2021- Facebook. All rights reserved.
#include "fboss/platform/fan_service/hw_test/FanServiceTestHelper.h"
namespace facebook::fboss::platform {
void fsTestSetUp(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler) {}
void fsTestTearDown(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    facebook::fboss::platform::FanServiceHandler* handler) {
  thriftServer.reset();
  delete handler;
}
} // namespace facebook::fboss::platform
