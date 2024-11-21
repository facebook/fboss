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
  MOCK_METHOD(
      bool,
      isValidDeviceName,
      (const platform_manager::PlatformConfig&,
       const std::string&,
       const std::string&));
  MOCK_METHOD(
      bool,
      isValidPmUnitName,
      (const platform_manager::PlatformConfig&,
       const std::string&,
       const std::string&));
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
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP/input1")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.sensors() = {
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3")};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  platformConfig_.symbolicLinkToDevicePath() = {
      {"/run/devmap/sensors/CPU_CORE_TEMP", "/[CPU_CORE_TEMP]"},
      {"/run/devmap/sensors/BCB_FAN_CPLD", "/BCB_SLOT@0/[BCB_FAN_CPLD]"}};
  EXPECT_CALL(*mockPmConfigValidator_, isValidSlotPath(_, _))
      .Times(2 /* # of PmUnitSensors*/)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mockPmConfigValidator_, isValidPmUnitName(_, _, _))
      .Times(2 /* # of PmUnitSensors*/)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mockPmConfigValidator_, isValidDeviceName(_, _, _))
      .Times(2 /* # of PmUnitSensors*/)
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(configValidator_.isValid(config, platformConfig_));
}
TEST_F(ConfigValidatorTest, ValidConfigForDarwin) {
  EXPECT_CALL(*mockPmConfigValidator_, isValidSlotPath(_, _)).Times(0);
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP/input1")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.sensors() = {
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3")};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_TRUE(configValidator_.isValid(config, std::nullopt));
}

TEST_F(ConfigValidatorTest, ValidPmUnitSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.pmUnitName() = "BCB";
  pmUnitSensors1.sensors() = {};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "BCB2";
  pmUnitSensors2.sensors() = {};
  // Valid PmUnitSensors
  EXPECT_TRUE(configValidator_.isValidPmUnitSensorsList(
      {pmUnitSensors1, pmUnitSensors2}));
  // Invalid PmUnitSensors -- Duplicate (SlotPath, PmUnitName) Pairing
  pmUnitSensors2.pmUnitName() = "BCB";
  EXPECT_FALSE(configValidator_.isValidPmUnitSensorsList(
      {pmUnitSensors1, pmUnitSensors2}));
}

TEST_F(ConfigValidatorTest, PmValidPmUnitSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.pmUnitName() = "BCB";
  pmUnitSensors1.sensors() = {};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "BCB2";
  pmUnitSensors2.sensors() = {};
  // PmValid PmUnitSensors
  EXPECT_CALL(
      *mockPmConfigValidator_, isValidSlotPath(platformConfig_, "/BCB_SLOT@0"))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(
      *mockPmConfigValidator_,
      isValidPmUnitName(platformConfig_, "/BCB_SLOT@0", _))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(configValidator_.isPmValidPmUnitSensorList(
      platformConfig_, {pmUnitSensors1, pmUnitSensors2}));
  // Invalid PmUnitSensors -- Invalid SlotPath
  EXPECT_CALL(
      *mockPmConfigValidator_, isValidSlotPath(platformConfig_, "/BCB_SLOT@0"))
      .WillOnce(Return(false));
  EXPECT_FALSE(configValidator_.isPmValidPmUnitSensorList(
      platformConfig_, {pmUnitSensors1, pmUnitSensors2}));
  // Invalid PmUnitSensors -- Invalid PmUnitName
  EXPECT_CALL(
      *mockPmConfigValidator_, isValidSlotPath(platformConfig_, "/BCB_SLOT@0"))
      .WillOnce(Return(true));
  EXPECT_CALL(
      *mockPmConfigValidator_,
      isValidPmUnitName(platformConfig_, "/BCB_SLOT@0", _))
      .WillOnce(Return(false));
  EXPECT_FALSE(configValidator_.isPmValidPmUnitSensorList(
      platformConfig_, {pmUnitSensors1, pmUnitSensors2}));
}

TEST_F(ConfigValidatorTest, ValidPmSensors) {
  // Invalid PmSensor -- Empty sensor name
  EXPECT_FALSE(configValidator_.isValidPmSensors(
      {createPmSensor("", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3")}));
  // Invalid PmSensor -- Invalid sysfs path
  EXPECT_FALSE(configValidator_.isValidPmSensors(
      {createPmSensor("sensor1", "/run/devmap/eeproms/BCB_EEPROMS/xyz")}));
  // Invalid PmSensor -- Duplicate sensor name
  EXPECT_FALSE(configValidator_.isValidPmSensors(
      {createPmSensor("sensor1", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3"),
       createPmSensor(
           "sensor1", "/run/devmap/sensors/BCB_FAN_CPLD2/fan_input")}));
  // Valid PmSensor
  EXPECT_TRUE(configValidator_.isValidPmSensors(
      {createPmSensor("sensor1", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3"),
       createPmSensor(
           "sensor2", "/run/devmap/sensors/BCB_FAN_CPLD2/fan_input")}));
}

TEST_F(ConfigValidatorTest, PmValidPmSensors) {
  const auto slotPath = "/BCB_SLOT@0";
  const auto pmSensors = {
      createPmSensor("sensor1", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3"),
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD2/fan_input")};
  // Invalid PmSensor -- Unmatching SlotPaths for BCB_FAN_CPLD2
  platformConfig_.symbolicLinkToDevicePath() = {
      {"/run/devmap/sensors/BCB_FAN_CPLD", "/BCB_SLOT@0/[BCB_FAN_CPLD]"},
      {"/run/devmap/sensors/BCB_FAN_CPLD2", "/[BCB_FAN_CPLD2]"}};
  EXPECT_CALL(*mockPmConfigValidator_, isValidDeviceName(platformConfig_, _, _))
      .WillOnce(Return(true));
  EXPECT_FALSE(configValidator_.isPmValidPmSensors(
      platformConfig_, slotPath, pmSensors));
  // Invalid PmSensor -- Invalid Device Topology
  platformConfig_.symbolicLinkToDevicePath() = {
      {"/run/devmap/sensors/BCB_FAN_CPLD", "/BCB_SLOT@0/[BCB_FAN_CPLD]"},
      {"/run/devmap/sensors/BCB_FAN_CPLD2", "/BCB_SLOT@0/[BCB_FAN_CPLD2]"}};
  EXPECT_CALL(*mockPmConfigValidator_, isValidDeviceName(platformConfig_, _, _))
      .WillOnce(Return(false));
  EXPECT_FALSE(configValidator_.isPmValidPmSensors(
      platformConfig_, slotPath, pmSensors));
  // Valid PmSensor
  EXPECT_CALL(*mockPmConfigValidator_, isValidDeviceName(platformConfig_, _, _))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(configValidator_.isPmValidPmSensors(
      platformConfig_, slotPath, pmSensors));
}
