// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

std::string mockSensorConfig(
    const std::string& tmpPath,
    const std::string& source);

std::unique_ptr<facebook::fboss::platform::sensor_service::SensorServiceImpl>
createSensorServiceImplForTest(
    const std::string& tmpDirPath,
    const std::string& source = std::string("mock"));

std::string createMockSensorDataFile(const std::string& tmpDirPath);

std::map<std::string, float> getDefaultMockSensorData();
