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

#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/SensorServiceThrift.h"

namespace facebook::fboss::platform::sensor_service {
class SensorServiceHwTest : public ::testing::Test {
 public:
  ~SensorServiceHwTest() override;
  void SetUp() override;

 protected:
  SensorReadResponse getSensors(const std::vector<std::string>& sensors);
  std::vector<std::string> allSensorNamesFromConfig();
  std::vector<std::string> someSensorNamesFromConfig();
  std::shared_ptr<SensorServiceImpl> sensorServiceImpl_;
  std::shared_ptr<SensorServiceThriftHandler> sensorServiceHandler_;
  SensorConfig sensorConfig_;
};
} // namespace facebook::fboss::platform::sensor_service
