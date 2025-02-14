/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossError.h"
#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform::fan_service;
using facebook::fboss::FbossError;

class BspTest : public ::testing::Test {
  static auto constexpr kSensorName = "sensor";

 protected:
  FanServiceConfig makeConfig(AccessMethod accessMethod) const {
    auto config = FanServiceConfig{};
    Sensor sensor;
    sensor.sensorName() = kSensorName;
    sensor.access() = accessMethod;
    config.sensors()->push_back(sensor);
    return config;
  }
};

TEST_F(BspTest, getSensorMethodUnset) {
  auto bsp = Bsp(makeConfig({}));
  EXPECT_THROW(bsp.getSensorData(std::make_shared<SensorData>()), FbossError);
}
