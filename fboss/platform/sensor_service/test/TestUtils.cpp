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

  Sensor mock_fru_1_sensor_1, mock_fru_1_sensor_2, mock_fru_2_sensor_1;
  mock_fru_1_sensor_1.path_ref() = tmpPath + "/mock_fru_1_sensor_1_path:temp1";
  mock_fru_1_sensor_2.path_ref() = tmpPath + "/mock_fru_1_sensor_2_path:fan1";
  mock_fru_2_sensor_1.path_ref() = tmpPath + "/mock_fru_2_sensor_1_path:vin";
  std::string value = "25";
  folly::writeFile(value, (*mock_fru_1_sensor_1.path()).c_str());
  value = "11152";
  folly::writeFile(value, (*mock_fru_1_sensor_2.path()).c_str());
  value = "11.875";
  folly::writeFile(value, (*mock_fru_2_sensor_1.path()).c_str());

  mock_fru_1_sensor_1.compute() = "@/1000";
  mock_fru_2_sensor_1.compute() = "@ + 5";

  mock_fru_1_sensor_1.type_ref() = SensorType::TEMPERTURE;
  mock_fru_1_sensor_2.type_ref() = SensorType::FAN;
  mock_fru_2_sensor_1.type_ref() = SensorType::VOLTAGE;

  sensorMap sMapFru1, sMapFru2;
  sMapFru1["MOCK_FRU_1_SENSOR_1"] = mock_fru_1_sensor_1;
  sMapFru1["MOCK_FRU_1_SENSOR_2"] = mock_fru_1_sensor_2;
  sMapFru2["MOCK_FRU_2_SENSOR_1"] = mock_fru_2_sensor_1;
  config.sensorMapList_ref() = {
      {"MOCK_FRU1", sMapFru1}, {"MOCK_FRU2", sMapFru2}};

  std::string fileName = tmpPath + "/mock_sensor_config";
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
      {"MOCK_FRU_1_SENSOR_1", 0.025 /* 25/10000 */},
      {"MOCK_FRU_1_SENSOR_2", 11152},
      {"MOCK_FRU_2_SENSOR_1", 16.875 /* 11.875 + 5*/},
  };
}
