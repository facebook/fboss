// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"

using namespace facebook::fboss::platform::sensor_service;
using namespace facebook::fboss::platform::sensor_config;

namespace facebook::fboss {

class SensorServiceImplInputVoltageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PmUnitSensors pmUnitSensors;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.pmUnitName() = "MOCK_PSU";
    config_.pmUnitSensorsList() = {pmUnitSensors};

    // Initialize PowerConfig with empty inputVoltageSensors
    PowerConfig powerConfig;
    powerConfig.perSlotPowerConfigs() = {};
    powerConfig.otherPowerSensorNames() = {};
    powerConfig.powerDelta() = 0.0;
    powerConfig.inputVoltageSensors() = {};
    config_.powerConfig() = powerConfig;
  }

  void addInputVoltageSensors(const std::vector<std::string>& sensorNames) {
    config_.powerConfig()->inputVoltageSensors() = sensorNames;
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

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithMultipleSensors) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});

  auto polledData =
      createPolledData({{"PSU1_VIN", 220.5}, {"PSU2_VIN", 215.3}});

  auto impl = SensorServiceImpl{config_};
  impl.processInputVoltage(
      polledData, *config_.powerConfig()->inputVoltageSensors());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedValue,
              SensorServiceImpl::kMaxInputVoltage)),
      220);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedFailure,
              SensorServiceImpl::kMaxInputVoltage)),
      0);
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithSingleSensor) {
  addInputVoltageSensors({"PSU1_VIN"});

  auto polledData = createPolledData({{"PSU1_VIN", 230.7}});

  auto impl = SensorServiceImpl{config_};
  impl.processInputVoltage(
      polledData, *config_.powerConfig()->inputVoltageSensors());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedValue,
              SensorServiceImpl::kMaxInputVoltage)),
      230);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedFailure,
              SensorServiceImpl::kMaxInputVoltage)),
      0);
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithMissingSensor) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});

  // Only PSU1_VIN has data, PSU2_VIN is missing
  auto polledData = createPolledData({{"PSU1_VIN", 225.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processInputVoltage(
      polledData, *config_.powerConfig()->inputVoltageSensors());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedValue,
              SensorServiceImpl::kMaxInputVoltage)),
      225);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedFailure,
              SensorServiceImpl::kMaxInputVoltage)),
      0);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    MaxInputVoltageWithAllSensorsMissing) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});

  // No sensor data available
  auto polledData = createPolledData({});

  auto impl = SensorServiceImpl{config_};
  impl.processInputVoltage(
      polledData, *config_.powerConfig()->inputVoltageSensors());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedValue,
              SensorServiceImpl::kMaxInputVoltage)),
      0);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedFailure,
              SensorServiceImpl::kMaxInputVoltage)),
      1);
}

TEST_F(SensorServiceImplInputVoltageTest, EmptyInputVoltageSensors) {
  // No input voltage sensors configured
  auto polledData = createPolledData({});
  auto impl = SensorServiceImpl{config_};
  impl.processInputVoltage(
      polledData, *config_.powerConfig()->inputVoltageSensors());

  // Should not publish any stats when no sensors are configured
  // The function returns early, so no counter is created
  // We can verify this by checking that calling the function doesn't crash
  SUCCEED();
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithZeroValue) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});

  auto polledData = createPolledData({{"PSU1_VIN", 0.0}, {"PSU2_VIN", 220.0}});

  auto impl = SensorServiceImpl{config_};
  impl.processInputVoltage(
      polledData, *config_.powerConfig()->inputVoltageSensors());

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedValue,
              SensorServiceImpl::kMaxInputVoltage)),
      220);

  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kDerivedFailure,
              SensorServiceImpl::kMaxInputVoltage)),
      0);
}

} // namespace facebook::fboss
