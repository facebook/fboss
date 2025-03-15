// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

std::shared_ptr<facebook::fboss::platform::sensor_service::SensorServiceImpl>
createSensorServiceImplForTest(const std::string& tmpDirPath);

std::map<std::string, float> getDefaultMockSensorData();
