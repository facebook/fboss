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

  void runInputVoltageTest(std::map<std::string, SensorData>& polledData) {
    impl_->processInputVoltage(polledData, *config_.powerConfig());
  }

  void createImpl() {
    impl_ = std::make_unique<SensorServiceImpl>(config_);
  }

  void expectMaxVoltageStats(int expectedValue, int expectedFailures = 0) {
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(
                SensorServiceImpl::kDerivedValue,
                SensorServiceImpl::kMaxInputVoltage)),
        expectedValue);
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(
                SensorServiceImpl::kDerivedFailure,
                SensorServiceImpl::kMaxInputVoltage)),
        expectedFailures);
  }

  void expectInputPowerType(int expectedType) {
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(
                SensorServiceImpl::kDerivedValue,
                SensorServiceImpl::kInputPowerType)),
        expectedType);
  }

  SensorConfig config_;
  std::unique_ptr<SensorServiceImpl> impl_;
};

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithMultipleSensors) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData =
      createPolledData({{"PSU1_VIN", 220.5}, {"PSU2_VIN", 215.3}});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(220);
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithMissingSensor) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 225.0}});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(225);
}

TEST_F(SensorServiceImplInputVoltageTest, EmptyInputVoltageSensors) {
  createImpl();
  auto polledData = createPolledData({});
  runInputVoltageTest(polledData);
  // Verify no crash when no sensors are configured
  SUCCEED();
}

TEST_F(SensorServiceImplInputVoltageTest, NoSensorData) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData = createPolledData({});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(0, 1);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
}

TEST_F(SensorServiceImplInputVoltageTest, MaxInputVoltageWithZeroValue) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 0.0}, {"PSU2_VIN", 220.0}});
  runInputVoltageTest(polledData);

  expectMaxVoltageStats(220);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    InputPowerTypePersistsAcrossReadings) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();

  auto polledData1 =
      createPolledData({{"PSU1_VIN", 220.0}, {"PSU2_VIN", 215.0}});
  runInputVoltageTest(polledData1);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeAC);

  auto polledData2 = createPolledData({{"PSU1_VIN", 48.0}, {"PSU2_VIN", 50.0}});
  runInputVoltageTest(polledData2);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeAC);
}

TEST_F(SensorServiceImplInputVoltageTest, DCPowerTypeEstablished) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 50.0f}});
  runInputVoltageTest(polledData);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeDC);
}

TEST_F(SensorServiceImplInputVoltageTest, UnknownPowerTypeLowVoltage) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();
  auto polledData = createPolledData({{"PSU1_VIN", 5.0f}});
  runInputVoltageTest(polledData);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
}

TEST_F(SensorServiceImplInputVoltageTest, UnknownPowerTypeGapVoltage) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();
  // 75V is between dcVoltageMax (64) and acVoltageMin (90)
  auto polledData = createPolledData({{"PSU1_VIN", 75.0f}});
  runInputVoltageTest(polledData);
  expectInputPowerType(SensorServiceImpl::kInputPowerTypeUnknown);
  // No thresholds should be assigned when power type is unknown
  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->lowerCriticalVal().has_value());
  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->upperCriticalVal().has_value());
}

TEST_F(SensorServiceImplInputVoltageTest, ThresholdsSetOnPowerTypeEstablished) {
  addInputVoltageSensors({"PSU1_VIN", "PSU2_VIN"});
  createImpl();

  auto polledData =
      createPolledData({{"PSU1_VIN", 220.0}, {"PSU2_VIN", 215.0}});

  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->lowerCriticalVal().has_value());
  EXPECT_FALSE(
      polledData["PSU1_VIN"].thresholds()->upperCriticalVal().has_value());

  runInputVoltageTest(polledData);

  // Thresholds come from thrift PowerConfig defaults
  // (acVoltageMin=90, acVoltageMax=305)
  Thresholds expectedThresholds;
  expectedThresholds.lowerCriticalVal() = 90.0;
  expectedThresholds.upperCriticalVal() = 305.0;
  EXPECT_EQ(*polledData["PSU1_VIN"].thresholds(), expectedThresholds);
  EXPECT_EQ(*polledData["PSU2_VIN"].thresholds(), expectedThresholds);
}

TEST_F(SensorServiceImplInputVoltageTest, ThresholdsSetForDC) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();

  auto polledData = createPolledData({{"PSU1_VIN", 50.0}});
  runInputVoltageTest(polledData);

  // Thresholds come from thrift PowerConfig defaults
  // (dcVoltageMin=9, dcVoltageMax=64)
  Thresholds expectedThresholds;
  expectedThresholds.lowerCriticalVal() = 9.0;
  expectedThresholds.upperCriticalVal() = 64.0;
  EXPECT_EQ(*polledData["PSU1_VIN"].thresholds(), expectedThresholds);
}

TEST_F(
    SensorServiceImplInputVoltageTest,
    ThresholdsNotOverwrittenIfAlreadySet) {
  addInputVoltageSensors({"PSU1_VIN"});
  createImpl();

  auto polledData = createPolledData({{"PSU1_VIN", 220.0}});
  polledData["PSU1_VIN"].thresholds()->lowerCriticalVal() = 100.0f;
  polledData["PSU1_VIN"].thresholds()->upperCriticalVal() = 300.0f;

  runInputVoltageTest(polledData);

  Thresholds expectedThresholds;
  expectedThresholds.lowerCriticalVal() = 100.0f;
  expectedThresholds.upperCriticalVal() = 300.0f;
  EXPECT_EQ(*polledData["PSU1_VIN"].thresholds(), expectedThresholds);
}

} // namespace facebook::fboss
