// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/config_lib/CrossConfigValidator.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
namespace {
sensor_config::PmSensor createPmSensor(
    const std::string& name,
    const std::string& sysfsPath) {
  sensor_config::PmSensor pmSensor;
  pmSensor.name() = name;
  pmSensor.sysfsPath() = sysfsPath;
  return pmSensor;
}
} // namespace

class CrossConfigValidatorTest : public testing::Test {
 public:
  void SetUp() override {
    // PM config set up
    platform_manager::PlatformConfig platformConfig;
    platform_manager::SlotTypeConfig bcbSlotTypeConfig;
    platformConfig.rootSlotType() = "MCB_SLOT";
    bcbSlotTypeConfig.idpromConfig() = platform_manager::IdpromConfig();
    platformConfig.slotTypeConfigs() = {
        {"MCB_SLOT", platform_manager::SlotTypeConfig()},
        {"BCB_SLOT", bcbSlotTypeConfig}};

    platform_manager::PmUnitConfig mcbPmUnitConfig, bcbPmUnitConfig;
    mcbPmUnitConfig.pluggedInSlotType() = "MCB_SLOT";
    platform_manager::SlotConfig bcbSlotConfig;
    bcbSlotConfig.slotType() = "BCB_SLOT";
    mcbPmUnitConfig.outgoingSlotConfigs() = {{"BCB_SLOT@0", bcbSlotConfig}};
    bcbPmUnitConfig.pluggedInSlotType() = "BCB_SLOT";

    platformConfig.pmUnitConfigs() = {
        {"MCB", mcbPmUnitConfig}, {"BCB", bcbPmUnitConfig}};
    platformConfig.symbolicLinkToDevicePath() = {
        {"/run/devmap/sensors/CPU_CORE_TEMP", "/[CPU_CORE_TEMP]"},
        {"/run/devmap/sensors/BCB_FAN_CPLD", "/BCB_SLOT@0/[BCB_FAN_CPLD]"}};

    crossConfigValidator_ = CrossConfigValidator(platformConfig);
  }

  std::optional<CrossConfigValidator> crossConfigValidator_{std::nullopt};
};

TEST_F(CrossConfigValidatorTest, ValidSensorConfig) {
  auto config = sensor_config::SensorConfig();
  sensor_config::PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.pmUnitName() = "MCB";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP/input1")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "BCB";
  pmUnitSensors2.sensors() = {
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD/pwm3")};
  sensor_config::VersionedPmSensor versionedSensors2;
  versionedSensors2.sensors() = {
      createPmSensor("sensor3", "/run/devmap/sensors/BCB_FAN_CPLD/pwm4")};
  pmUnitSensors2.versionedSensors() = {versionedSensors2};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};

  EXPECT_TRUE(crossConfigValidator_->isValidSensorConfig(config));
}

TEST_F(CrossConfigValidatorTest, InvalidSensorConfig) {
  auto config = sensor_config::SensorConfig();
  sensor_config::PmUnitSensors pmUnitSensors;

  // Undefined runtime path in PM config.
  pmUnitSensors.slotPath() = "/";
  pmUnitSensors.pmUnitName() = "MCB";
  pmUnitSensors.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/SMB_E1S_SSD_TEMP/input1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(crossConfigValidator_->isValidSensorConfig(config));

  // Invalid PmUnit at SlotPath
  pmUnitSensors.slotPath() = "/";
  pmUnitSensors.pmUnitName() = "SMB";
  pmUnitSensors.sensors() = {};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(crossConfigValidator_->isValidSensorConfig(config));

  // Undefined SlotPath
  pmUnitSensors.slotPath() = "/SMB_SLOT@0";
  pmUnitSensors.pmUnitName() = "SMB";
  pmUnitSensors.sensors() = {};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(crossConfigValidator_->isValidSensorConfig(config));
}
