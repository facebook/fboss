// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/FileUtil.h>
#include <folly/dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/sensor_service/test/TestUtils.h"

using namespace facebook::fboss::platform::sensor_service;

std::string mockSensorConfig(const std::string& tmpPath) {
  SensorConfig config;
  config.source_ref() = "mock";

  Sensor mock_fru_1_sensor_1, mock_fru_1_sensor_2, mock_fru_2_sensor_1;
  mock_fru_1_sensor_1.path_ref() = tmpPath + "/mock_fru_1_sensor_1_path:temp1";
  mock_fru_1_sensor_2.path_ref() = tmpPath + "/mock_fru_1_sensor_2_path:fan1";
  mock_fru_2_sensor_1.path_ref() = tmpPath + "/mock_fru_2_sensor_1_path:vin";
  std::string dummyData = "dummy";
  folly::writeFile(dummyData, (*mock_fru_1_sensor_1.path()).c_str());
  folly::writeFile(dummyData, (*mock_fru_1_sensor_2.path()).c_str());
  folly::writeFile(dummyData, (*mock_fru_2_sensor_1.path()).c_str());

  Thresholds thresholds{};
  thresholds.lowerCriticalVal_ref() = 105;
  mock_fru_1_sensor_1.thresholds_ref() = thresholds;
  mock_fru_1_sensor_2.thresholds_ref() = thresholds;
  mock_fru_2_sensor_1.thresholds_ref() = thresholds;

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

std::string mockSensorData(const std::string& tmpPath) {
  folly::dynamic sensorJson = folly::dynamic::object;
  sensorJson[tmpPath + "/mock_fru_1_sensor_1_path"] = folly::parseJson(R"({
    "Adapter" : "Blah",
    "temp1" : {
      "temp1_input" : 25.000,
      "temp1_max" : 255.750,
      "temp1_min" : -256.000,
      "temp1_max_alarm" : 0.000,
      "temp1_min_alarm" : 0.000
    }
  })");
  sensorJson[tmpPath + "/mock_fru_1_sensor_2_path"] = folly::parseJson(R"({
    "Adapter": "Blah",
    "fan1": {
        "fan1_input": 11152.000,
        "fan1_fault": 0.000
    }
  })");
  sensorJson[tmpPath + "/mock_fru_2_sensor_1_path"] = folly::parseJson(R"({
    "Adapter": "Blah",
    "vin": {
        "in1_input": 11.875,
        "in1_min": 9.500,
        "in1_crit": 14.000,
        "in1_min_alarm": 0.000,
        "in1_crit_alarm": 0.000
    }
  })");
  std::string fileName = tmpPath + "/mock_sensor_data";
  folly::writeFile(folly::toJson(sensorJson), fileName.c_str());
  return fileName;
}

std::unique_ptr<SensorServiceImpl> createSensorServiceImplForTest(
    const std::string& tmpDirPath) {
  auto impl = std::make_unique<SensorServiceImpl>(mockSensorConfig(tmpDirPath));
  return impl;
}

std::string createMockSensorDataFile(const std::string& tmpDirPath) {
  return mockSensorData(tmpDirPath);
}
