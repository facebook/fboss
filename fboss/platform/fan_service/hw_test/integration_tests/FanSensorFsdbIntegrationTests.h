// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gtest/gtest.h>

#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#else
// In the case of OSS, the actual empty class definition is needed,
// as we use unique_ptr, which uses "sizeof" of the target class.
namespace facebook::services {
class ServiceFrameworkLight {};
} // namespace facebook::services
#endif // IS_OSS

#include "fboss/platform/fan_service/hw_test/FanServiceTestHelper.h"

namespace apache::thrift {
class ThriftServer;
}

namespace facebook::fboss::platform {

// Defining the main test handler class
class FanSensorFsdbIntegrationTests : public ::testing::Test {
 public:
  ~FanSensorFsdbIntegrationTests() override {}
  void SetUp() override;
  void TearDown() override;

 protected:
  FanService* getFanService();
  std::shared_ptr<apache::thrift::ThriftServer> thriftServer_;
  std::shared_ptr<FanServiceHandler> thriftHandler_;
  std::unique_ptr<services::ServiceFrameworkLight> service_;
};
} // namespace facebook::fboss::platform
