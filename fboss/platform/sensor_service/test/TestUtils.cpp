// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/FileUtil.h>
#include <folly/json/dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/sensor_service/test/TestUtils.h"

using namespace facebook::fboss::platform::sensor_service;

namespace {

std::string mockSensorConfig(const std::string& tmpPath) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/";
  pmUnitSensors.pmUnitName() = "MOCK";

  PmSensor mockFruSensor1, mockFruSensor2, mockFruSensor3;
  mockFruSensor1.name() = "MOCK_FRU_SENSOR1";
  mockFruSensor1.compute() = "@/1000";
  mockFruSensor1.type() = SensorType::TEMPERTURE;
  mockFruSensor1.sysfsPath() = tmpPath + "/mock_fru_sensor_1_path:temp1";
  folly::writeFile(std::string{"25"}, mockFruSensor1.sysfsPath()->c_str());
  mockFruSensor2.name() = "MOCK_FRU_SENSOR2";
  mockFruSensor2.type() = SensorType::FAN;
  mockFruSensor2.sysfsPath() = tmpPath + "/mock_fru_sensor_2_path:fan1";
  folly::writeFile(std::string{"11152"}, mockFruSensor2.sysfsPath()->c_str());
  mockFruSensor3.name() = "MOCK_FRU_SENSOR3";
  mockFruSensor3.compute() = "@ + 5";
  mockFruSensor3.type() = SensorType::VOLTAGE;
  mockFruSensor3.sysfsPath() = tmpPath + "/mock_fru_sensor_3_path:vin";
  folly::writeFile(std::string{"11.875"}, mockFruSensor3.sysfsPath()->c_str());

  pmUnitSensors.sensors() = {
      std::move(mockFruSensor1),
      std::move(mockFruSensor2),
      std::move(mockFruSensor3),
  };
  config.pmUnitSensorsList() = {std::move(pmUnitSensors)};

  std::string fileName = tmpPath + "/sensor_service.json";
  folly::writeFile(
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(config),
      fileName.c_str());
  return fileName;
}

} // namespace

std::shared_ptr<SensorServiceImpl> createSensorServiceImplForTest(
    const std::string& tmpDirPath) {
  auto confFileName = mockSensorConfig(tmpDirPath);
  FLAGS_config_file = confFileName;
  return std::make_shared<SensorServiceImpl>();
}

std::map<std::string, float> getDefaultMockSensorData() {
  return std::map<std::string, float>{
      {"MOCK_FRU_SENSOR1", 0.025 /* 25/10000 */},
      {"MOCK_FRU_SENSOR2", 11152},
      {"MOCK_FRU_SENSOR3", 16.875 /* 11.875 + 5*/},
  };
}
