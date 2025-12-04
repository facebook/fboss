// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

using namespace facebook::fboss::platform::sensor_service;
using namespace facebook::fboss::platform::sensor_config;

namespace facebook::fboss {

class SensorServiceImplTemperatureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.pmUnitName() = "MOCK_ASIC";
    config_.pmUnitSensorsList() = {pmUnitSensors};
  }

  TemperatureConfig addTemperatureConfig(
      const std::string& name,
      const std::vector<std::string>& temperatureSensorNames) {
    TemperatureConfig tempConfig;
    tempConfig.name() = name;
    tempConfig.temperatureSensorNames() = temperatureSensorNames;
    config_.temperatureConfigs()->push_back(tempConfig);
    return tempConfig;
  }

  void addSensors(const std::vector<std::string>& sensorNames) {
    for (const auto& name : sensorNames) {
      PmSensor sensor;
      sensor.name() = name;
      sensor.sysfsPath() = "/tmp/dummy/" + name;
      config_.pmUnitSensorsList()[0].sensors()->push_back(sensor);
    }
  }

  std::map<std::string, SensorData> createPolledData(
      const std::map<std::string, float>& sensorData) {
    std::map<std::string, SensorData> polledData;
    for (const auto& [sensorName, value] : sensorData) {
      SensorData sData;
      sData.value() = value;
      polledData.emplace(sensorName, sData);
    }
    return polledData;
  }
  SensorConfig config_;
};

TEST_F(SensorServiceImplTemperatureTest, TemperatureWithSingleSensor) {
  auto tempConfig1 = addTemperatureConfig("ASIC", {"ASIC_TEMP_SENSOR"});
  addSensors({"ASIC_TEMP_SENSOR"});

  auto polledData = createPolledData({{"ASIC_TEMP_SENSOR", 65.5}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC_TEMP")),
      65);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC_TEMP")),
      0);
}

TEST_F(SensorServiceImplTemperatureTest, TemperatureMaxSelection) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC", {"TEMP_1", "TEMP_2", "TEMP_3", "TEMP_4"});
  addSensors({"TEMP_1", "TEMP_2", "TEMP_3", "TEMP_4"});

  auto polledData = createPolledData(
      {{"TEMP_1", 45.0}, {"TEMP_2", 88.5}, {"TEMP_3", 62.0}, {"TEMP_4", 50.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC_TEMP")),
      88);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC_TEMP")),
      0);
}

TEST_F(SensorServiceImplTemperatureTest, TemperatureWithMissingSensorValue) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC1", {"ASIC1_TEMP_1", "ASIC1_TEMP_2"});
  addSensors({"ASIC1_TEMP_1", "ASIC1_TEMP_2"});

  auto polledData = createPolledData({{"ASIC1_TEMP_2", 75.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC1_TEMP")),
      75);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC1_TEMP")),
      0);
}

TEST_F(SensorServiceImplTemperatureTest, TemperatureWithAllSensorsMissing) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC1", {"ASIC1_TEMP_1", "ASIC1_TEMP_2"});
  addSensors({"ASIC1_TEMP_1", "ASIC1_TEMP_2"});

  auto polledData = createPolledData({});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC1_TEMP")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC1_TEMP")),
      1);
}

TEST_F(SensorServiceImplTemperatureTest, MultipleTemperatureConfigs) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC1", {"ASIC1_TEMP_1", "ASIC1_TEMP_2"});
  auto tempConfig2 =
      addTemperatureConfig("ASIC2", {"ASIC2_TEMP_1", "ASIC2_TEMP_2"});
  addSensors({"ASIC1_TEMP_1", "ASIC1_TEMP_2", "ASIC2_TEMP_1", "ASIC2_TEMP_2"});

  auto polledData = createPolledData(
      {{"ASIC1_TEMP_1", 60.0},
       {"ASIC1_TEMP_2", 65.0},
       {"ASIC2_TEMP_1", 70.0},
       {"ASIC2_TEMP_2", 72.5}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1, tempConfig2});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC1_TEMP")),
      65);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC1_TEMP")),
      0);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC2_TEMP")),
      72);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC2_TEMP")),
      0);
}

TEST_F(SensorServiceImplTemperatureTest, TemperatureWithPartialFailure) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC1", {"ASIC1_TEMP_1", "ASIC1_TEMP_2"});
  auto tempConfig2 =
      addTemperatureConfig("ASIC2", {"ASIC2_TEMP_1", "ASIC2_TEMP_2"});
  addSensors({"ASIC1_TEMP_1", "ASIC1_TEMP_2", "ASIC2_TEMP_1", "ASIC2_TEMP_2"});

  auto polledData =
      createPolledData({{"ASIC1_TEMP_1", 60.0}, {"ASIC1_TEMP_2", 65.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1, tempConfig2});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC1_TEMP")),
      65);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC1_TEMP")),
      0);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC2_TEMP")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC2_TEMP")),
      1);
}

TEST_F(SensorServiceImplTemperatureTest, EmptyTemperatureConfig) {
  auto impl = SensorServiceImpl{config_};
  impl.processTemperature({}, {});
}

TEST_F(SensorServiceImplTemperatureTest, TemperatureWithAllZeroValues) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC", {"ASIC_TEMP_1", "ASIC_TEMP_2"});
  addSensors({"ASIC_TEMP_1", "ASIC_TEMP_2"});

  auto polledData =
      createPolledData({{"ASIC_TEMP_1", 0.0}, {"ASIC_TEMP_2", 0.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC_TEMP")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC_TEMP")),
      0);
}

TEST_F(SensorServiceImplTemperatureTest, TemperatureWithNegativeValue) {
  auto tempConfig1 =
      addTemperatureConfig("ASIC", {"ASIC_TEMP_1", "ASIC_TEMP_2"});
  addSensors({"ASIC_TEMP_1", "ASIC_TEMP_2"});

  auto polledData =
      createPolledData({{"ASIC_TEMP_1", -5.0}, {"ASIC_TEMP_2", 10.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processTemperature(polledData, {tempConfig1});

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "ASIC_TEMP")),
      10);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "ASIC_TEMP")),
      0);
}

} // namespace facebook::fboss
