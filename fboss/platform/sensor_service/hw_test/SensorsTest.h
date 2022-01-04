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

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

namespace apache::thrift {
class ThriftServer;
}
namespace facebook::fboss::platform::sensor_service {

class SensorServiceThriftHandler;

class SensorsTest : public ::testing::Test {
 public:
  ~SensorsTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  SensorServiceImpl* getService();
  std::shared_ptr<apache::thrift::ThriftServer> thriftServer_;
  std::shared_ptr<SensorServiceThriftHandler> thriftHandler_;
};
} // namespace facebook::fboss::platform::sensor_service
