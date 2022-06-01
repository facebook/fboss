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

class FanServiceThriftHandler;

// Defining the main test handler class
class FanServiceTest : public ::testing::Test {
 public:
  ~FanServiceTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  FanService* getFanService();
  std::shared_ptr<apache::thrift::ThriftServer> thriftServer_;
  std::shared_ptr<FanServiceHandler> thriftHandler_;
  std::unique_ptr<services::ServiceFrameworkLight> service_;
};
} // namespace facebook::fboss::platform
