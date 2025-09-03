// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

std::map<std::string, float> getMockSensorData();

facebook::fboss::platform::sensor_service::SensorConfig getMockSensorConfig(
    const std::string& tmpDir);
