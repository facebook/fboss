// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/ConfigValidator.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;
using namespace facebook::fboss::platform::sensor_config;
namespace {
PmSensor createPmSensor(const std::string& name, const std::string& sysfsPath) {
  PmSensor pmSensor;
  pmSensor.name() = name;
  pmSensor.sysfsPath() = sysfsPath;
  return pmSensor;
}
} // namespace
class MockPmConfigValidator : public platform_manager::ConfigValidator {
 public:
  MockPmConfigValidator() : platform_manager::ConfigValidator() {}
  MOCK_METHOD(
      bool,
      isValidSlotPath,
      (const platform_manager::PlatformConfig&, const std::string&));
};

class ConfigValidatorTest : public testing::Test {
 public:
  void SetUp() override {}

  std::shared_ptr<MockPmConfigValidator> mockPmConfigValidator_{
      std::make_shared<MockPmConfigValidator>()};
  ConfigValidator configValidator_{mockPmConfigValidator_};
  platform_manager::PlatformConfig platformConfig_;
};

TEST_F(ConfigValidatorTest, ValidConfig) {
  EXPECT_CALL(*mockPmConfigValidator_, isValidSlotPath(_, _))
      .Times(2 /* # of PmUnitSensors*/)
      .WillRepeatedly(Return(true));
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD")};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_TRUE(configValidator_.isValid(config, platformConfig_));
}

TEST_F(ConfigValidatorTest, ValidConfigForDarwin) {
  EXPECT_CALL(*mockPmConfigValidator_, isValidSlotPath(_, _)).Times(0);
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD")};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_TRUE(configValidator_.isValid(config, std::nullopt));
}

TEST_F(ConfigValidatorTest, PmUnitSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.pmUnitName() = "BCB";
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "BCB2";
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  // Valid PmUnitSensors
  EXPECT_CALL(
      *mockPmConfigValidator_, isValidSlotPath(platformConfig_, "/BCB_SLOT@0"))
      .Times(2 /* # of PmUnitSensors */)
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(configValidator_.isValid(config, platformConfig_));
  // Invalid PmUnitSensors -- Invalid SlotPath
  EXPECT_CALL(
      *mockPmConfigValidator_, isValidSlotPath(platformConfig_, "/BCB_SLOT@0"))
      .WillOnce(Return(false));
  EXPECT_FALSE(configValidator_.isValid(config, platformConfig_));
  // Invalid PmUnitSensors -- Duplicate (SlotPath, PmUnitName) Pairing
  EXPECT_CALL(
      *mockPmConfigValidator_, isValidSlotPath(platformConfig_, "/BCB_SLOT@0"))
      .Times(0);
  pmUnitSensors2.pmUnitName() = "BCB";
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_FALSE(configValidator_.isValid(config, platformConfig_));
}

TEST_F(ConfigValidatorTest, InvalidPmSensors) {
  // At this point, only interested in PmSensors, suppose SlotPaths are valid.
  EXPECT_CALL(*mockPmConfigValidator_, isValidSlotPath(_, _))
      .WillRepeatedly(Return(true));

  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.sensors() = {
      createPmSensor("", "/run/devmap/sensors/CPU_CORE_TEMP")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(configValidator_.isValid(config, platformConfig_));
  pmUnitSensors.sensors() = {
      createPmSensor("sensor1", "/run/devmap/eeproms/BCB_EEPROMS")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(configValidator_.isValid(config, platformConfig_));
  pmUnitSensors.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP"),
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP2")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(configValidator_.isValid(config, platformConfig_));
}
