/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/sensor_service/hw_test/SensorsTest.h"

DEFINE_string(
    config,
    "/etc/sensor_service/darwin_sensor_config.json",
    "Platform Sensor Configuration File Path, e.g. /etc/sensor_service/darwin_sensor_config.json");

namespace facebook::fboss::platform::sensor_service {

void SensorsTest::SetUp() {
  sensorServiceImpl_ = std::make_unique<SensorServiceImpl>(FLAGS_config);
}

TEST_F(SensorsTest, getAllSensors) {
  sensorServiceImpl_->fetchSensorData();
  sensorServiceImpl_->getAllSensorData();
}

TEST_F(SensorsTest, getBogusSensor) {
  sensorServiceImpl_->fetchSensorData();
  EXPECT_EQ(sensorServiceImpl_->getSensorData("bogusSensor_foo"), std::nullopt);
}
} // namespace facebook::fboss::platform::sensor_service
