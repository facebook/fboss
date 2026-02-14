// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/tests/TestUtils.h"

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>

using namespace facebook::fboss::platform::sensor_service;

namespace {
void createSensorSysfsFiles(const SensorConfig& config) {
  for (const auto& pmUnitSensors : *config.pmUnitSensorsList()) {
    for (const auto& sensor : *pmUnitSensors.sensors()) {
      if (sensor.name() == "MOCK_FRU_SENSOR1") {
        folly::writeFile(std::string{"25"}, sensor.sysfsPath()->c_str());
      } else if (sensor.name() == "MOCK_FRU_SENSOR2") {
        folly::writeFile(std::string{"11152"}, sensor.sysfsPath()->c_str());
      } else if (sensor.name() == "MOCK_FRU_SENSOR3") {
        folly::writeFile(std::string{"11.875"}, sensor.sysfsPath()->c_str());
      }
    }
  }
}
} // namespace

SensorConfig getMockSensorConfig(const std::string& tmpDir) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/";
  pmUnitSensors.pmUnitName() = "MOCK";

  PmSensor mockFruSensor1, mockFruSensor2, mockFruSensor3;
  mockFruSensor1.name() = "MOCK_FRU_SENSOR1";
  mockFruSensor1.compute() = "@/1000";
  mockFruSensor1.type() = SensorType::TEMPERATURE;
  mockFruSensor1.sysfsPath() = tmpDir + "/mock_fru_sensor_1_path:temp1";
  Thresholds thresholds1;
  thresholds1.upperCriticalVal() = 100.0;
  thresholds1.lowerCriticalVal() = -40.0;
  thresholds1.maxAlarmVal() = 85.0;
  thresholds1.minAlarmVal() = -20.0;
  mockFruSensor1.thresholds() = thresholds1;

  mockFruSensor2.name() = "MOCK_FRU_SENSOR2";
  mockFruSensor2.type() = SensorType::FAN;
  mockFruSensor2.sysfsPath() = tmpDir + "/mock_fru_sensor_2_path:fan1";
  Thresholds thresholds2;
  thresholds2.upperCriticalVal() = 10000.0;
  thresholds2.lowerCriticalVal() = 1000.0;
  mockFruSensor2.thresholds() = thresholds2;

  mockFruSensor3.name() = "MOCK_FRU_SENSOR3";
  mockFruSensor3.compute() = "@ + 5";
  mockFruSensor3.type() = SensorType::VOLTAGE;
  mockFruSensor3.sysfsPath() = tmpDir + "/mock_fru_sensor_3_path:vin";
  Thresholds thresholds3;
  thresholds3.maxAlarmVal() = 14.0;
  thresholds3.minAlarmVal() = 10.0;
  mockFruSensor3.thresholds() = thresholds3;

  pmUnitSensors.sensors() = {
      std::move(mockFruSensor1),
      std::move(mockFruSensor2),
      std::move(mockFruSensor3),
  };
  config.pmUnitSensorsList() = {std::move(pmUnitSensors)};
  createSensorSysfsFiles(config);
  return config;
}

std::map<std::string, float> getMockSensorData() {
  return std::map<std::string, float>{
      {"MOCK_FRU_SENSOR1", 0.025 /* 25/10000 */},
      {"MOCK_FRU_SENSOR2", 11152},
      {"MOCK_FRU_SENSOR3", 16.875 /* 11.875 + 5*/},
  };
}
