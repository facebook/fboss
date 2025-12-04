// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

using namespace facebook::fboss::platform::sensor_service;
using namespace facebook::fboss::platform::sensor_config;

namespace facebook::fboss {

class SensorServiceImplPowerConsumptionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.pmUnitName() = "MOCK_PDB";
    config_.pmUnitSensorsList() = {pmUnitSensors};

    // Initialize empty PowerConfig
    PowerConfig powerConfig;
    powerConfig.perSlotPowerConfigs() = {};
    powerConfig.otherPowerSensorNames() = {};
    powerConfig.powerDelta() = 0.0;
    powerConfig.inputVoltageSensors() = {};
    config_.powerConfig() = powerConfig;
  }

  PerSlotPowerConfig addPerSlotPowerConfig(
      const std::string& name,
      const std::optional<std::string>& pwrSensorName,
      const std::optional<std::string>& voltSensorName,
      const std::optional<std::string>& currSensorName) {
    PerSlotPowerConfig perSlotConfig;
    perSlotConfig.name() = name;
    perSlotConfig.powerSensorName().from_optional(pwrSensorName);
    perSlotConfig.voltageSensorName().from_optional(voltSensorName);
    perSlotConfig.currentSensorName().from_optional(currSensorName);
    config_.powerConfig()->perSlotPowerConfigs()->push_back(perSlotConfig);
    return perSlotConfig;
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

TEST_F(SensorServiceImplPowerConsumptionTest, PowerConsumptionWithPowerSensor) {
  addPerSlotPowerConfig("PSU1", "PSU1_POWER", std::nullopt, std::nullopt);
  addPerSlotPowerConfig("PSU2", "PSU2_POWER", std::nullopt, std::nullopt);
  addSensors({"PSU1_POWER", "PSU2_POWER"});

  auto polledData =
      createPolledData({{"PSU1_POWER", 150.5}, {"PSU2_POWER", 175.2}});

  auto impl = SensorServiceImpl{config_};
  impl.processPower(polledData, *config_.powerConfig());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU1_POWER")),
      150);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU2_POWER")),
      175);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "TOTAL_POWER")),
      325);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU1_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU2_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "TOTAL_POWER")),
      0);
}

TEST_F(
    SensorServiceImplPowerConsumptionTest,
    PowerConsumptionWithVoltageAndCurrent) {
  addPerSlotPowerConfig("PSU1", std::nullopt, "PSU1_VOLTAGE", "PSU1_CURRENT");
  addPerSlotPowerConfig("PSU2", std::nullopt, "PSU2_VOLTAGE", "PSU2_CURRENT");
  addSensors({"PSU1_VOLTAGE", "PSU1_CURRENT", "PSU2_VOLTAGE", "PSU2_CURRENT"});
  auto polledData = createPolledData(
      {{"PSU1_VOLTAGE", 12.0},
       {"PSU1_CURRENT", 10.5},
       {"PSU2_VOLTAGE", 11.8},
       {"PSU2_CURRENT", 15.2}});

  auto impl = SensorServiceImpl{config_};
  impl.processPower(polledData, *config_.powerConfig());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU1_POWER")),
      126);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU2_POWER")),
      179);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "TOTAL_POWER")),
      305);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU1_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU2_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "TOTAL_POWER")),
      0);
}

TEST_F(
    SensorServiceImplPowerConsumptionTest,
    PowerConsumptionWithMissingSensorValue) {
  addPerSlotPowerConfig("PSU1", "PSU1_POWER", std::nullopt, std::nullopt);
  addPerSlotPowerConfig("PSU2", "PSU2_POWER", std::nullopt, std::nullopt);
  addSensors({"PSU1_POWER", "PSU2_POWER"});
  auto polledData = createPolledData({{"PSU2_POWER", 175.2}});

  auto impl = SensorServiceImpl{config_};
  impl.processPower(polledData, *config_.powerConfig());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU1_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU1_POWER")),
      1);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU2_POWER")),
      175);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU2_POWER")),
      0);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "TOTAL_POWER")),
      175);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "TOTAL_POWER")),
      0);
}

TEST_F(SensorServiceImplPowerConsumptionTest, PowerConsumptionMixedSources) {
  addPerSlotPowerConfig("PSU1", "PSU1_POWER", std::nullopt, std::nullopt);
  addPerSlotPowerConfig("PSU2", std::nullopt, "PSU2_VOLTAGE", "PSU2_CURRENT");
  addSensors({"PSU1_POWER", "PSU2_VOLTAGE", "PSU2_CURRENT"});

  auto polledData = createPolledData(
      {{"PSU1_POWER", 150.0}, {"PSU2_VOLTAGE", 12.0}, {"PSU2_CURRENT", 10.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processPower(polledData, *config_.powerConfig());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU1_POWER")),
      150);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU2_POWER")),
      120);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "TOTAL_POWER")),
      270);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU1_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU2_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "TOTAL_POWER")),
      0);
}

TEST_F(SensorServiceImplPowerConsumptionTest, EmptyPowerConsumptionConfig) {
  auto impl = SensorServiceImpl{config_};
  PowerConfig emptyPowerConfig;
  emptyPowerConfig.perSlotPowerConfigs() = {};
  emptyPowerConfig.otherPowerSensorNames() = {};
  emptyPowerConfig.powerDelta() = 0.0;
  emptyPowerConfig.inputVoltageSensors() = {};
  impl.processPower({}, emptyPowerConfig);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "TOTAL_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "TOTAL_POWER")),
      0);
}

TEST_F(SensorServiceImplPowerConsumptionTest, PowerConsumptionWithZeroValues) {
  addPerSlotPowerConfig("PSU1", std::nullopt, "PSU1_VOLTAGE", "PSU1_CURRENT");
  addPerSlotPowerConfig("PSU2", "PSU2_POWER", std::nullopt, std::nullopt);
  addSensors({"PSU1_VOLTAGE", "PSU1_CURRENT", "PSU2_POWER"});

  auto polledData = createPolledData(
      {{"PSU1_VOLTAGE", 0.0}, {"PSU1_CURRENT", 10.0}, {"PSU2_POWER", 0.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processPower(polledData, *config_.powerConfig());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU1_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "PSU2_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedValue, "TOTAL_POWER")),
      0);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU1_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "PSU2_POWER")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(SensorServiceImpl::kDerivedFailure, "TOTAL_POWER")),
      0);
}

} // namespace facebook::fboss
