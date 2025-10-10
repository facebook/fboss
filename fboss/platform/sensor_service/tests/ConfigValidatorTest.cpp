// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/ConfigValidator.h"

using namespace ::testing;
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

TEST(ConfigValidatorTest, ValidConfig) {
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.sensors() = {
      createPmSensor("sensor2", "/run/devmap/sensors/BCB_FAN_CPLD")};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, SlotPaths) {
  // Valid (SlotPath,PmUnitName) pairing.
  SensorConfig config;
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.pmUnitName() = "BCB";
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "BCB2";
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_TRUE(ConfigValidator().isValid(config));
  // Duplicate (SlotPath, PmUnitName) pairing
  pmUnitSensors2.pmUnitName() = "BCB";
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPmSensors) {
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.sensors() = {
      createPmSensor("", "/run/devmap/sensors/CPU_CORE_TEMP")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));
  pmUnitSensors.sensors() = {
      createPmSensor("sensor1", "/run/devmap/eeproms/BCB_EEPROMS")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));
  pmUnitSensors.sensors() = {
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP"),
      createPmSensor("sensor1", "/run/devmap/sensors/CPU_CORE_TEMP2")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

PowerConsumptionConfig createPowerConsumptionConfig(
    const std::string& name,
    const std::optional<std::string>& powerSensorName = std::nullopt,
    const std::optional<std::string>& voltageSensorName = std::nullopt,
    const std::optional<std::string>& currentSensorName = std::nullopt) {
  PowerConsumptionConfig config;
  config.name() = name;

  config.powerSensorName().from_optional(powerSensorName);
  config.voltageSensorName().from_optional(voltageSensorName);
  config.currentSensorName().from_optional(currentSensorName);
  return config;
}

SensorConfig createBasicSensorConfig() {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";
  pmUnitSensors.sensors() = {
      createPmSensor("voltage_sensor", "/run/devmap/sensors/VOLTAGE"),
      createPmSensor("current_sensor", "/run/devmap/sensors/CURRENT"),
      createPmSensor("power_sensor", "/run/devmap/sensors/POWER")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  return config;
}

TEST(ConfigValidatorTest, ValidPowerConsumptionConfig) {
  auto config = createBasicSensorConfig();

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "power_sensor")};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig(
      "PSU2", std::nullopt, "voltage_sensor", "current_sensor")};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PEM1", "power_sensor")};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "power_sensor"),
      createPowerConsumptionConfig(
          "PSU2", std::nullopt, "voltage_sensor", "current_sensor")};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConsumptionConfigNaming) {
  auto config = createBasicSensorConfig();

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("INVALID1", "power_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU", "power_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU0", "power_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("POWER_UNIT", "power_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConsumptionConfigDuplicateName) {
  auto config = createBasicSensorConfig();

  // Duplicate power consumption config name
  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "power_sensor"),
      createPowerConsumptionConfig(
          "PSU1", std::nullopt, "voltage_sensor", "current_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConsumptionConfigNameConflictWithSensor) {
  auto config = createBasicSensorConfig();

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "power_sensor")};

  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU1", "/run/devmap/sensors/PSU1_SENSOR"));

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(
    ConfigValidatorTest,
    InvalidPowerConsumptionConfigInvalidSensorReferences) {
  auto config = createBasicSensorConfig();

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "nonexistent_power_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig(
      "PSU1", std::nullopt, "nonexistent_voltage_sensor", "current_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig(
      "PSU1", std::nullopt, "voltage_sensor", "nonexistent_current_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConsumptionConfigMissingSensors) {
  auto config = createBasicSensorConfig();

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig(
      "PSU1", std::nullopt, std::nullopt, "current_sensor")};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig(
      "PSU1", std::nullopt, "voltage_sensor", std::nullopt)};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig("PSU1")};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, GetAllSensorNames) {
  auto config = createBasicSensorConfig();

  PmUnitSensors pmUnitSensors2;
  pmUnitSensors2.slotPath() = "/SCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "SCB";
  pmUnitSensors2.sensors() = {
      createPmSensor("temp_sensor", "/run/devmap/sensors/TEMP"),
      createPmSensor("fan_sensor", "/run/devmap/sensors/FAN")};
  config.pmUnitSensorsList()->emplace_back(pmUnitSensors2);

  auto sensorNames = ConfigValidator().getAllSensorNames(config);

  EXPECT_EQ(sensorNames.size(), 5);
  EXPECT_TRUE(sensorNames.contains("voltage_sensor"));
  EXPECT_TRUE(sensorNames.contains("current_sensor"));
  EXPECT_TRUE(sensorNames.contains("power_sensor"));
  EXPECT_TRUE(sensorNames.contains("temp_sensor"));
  EXPECT_TRUE(sensorNames.contains("fan_sensor"));
}

TEST(ConfigValidatorTest, ValidPowerConsumptionConfigComplexScenario) {
  auto config = createBasicSensorConfig();

  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("psu1_voltage", "/run/devmap/sensors/PSU1_VOLTAGE"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("psu1_current", "/run/devmap/sensors/PSU1_CURRENT"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("psu2_power", "/run/devmap/sensors/PSU2_POWER"));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig(
          "PSU1", std::nullopt, "psu1_voltage", "psu1_current"),
      createPowerConsumptionConfig("PSU2", "psu2_power"),
      createPowerConsumptionConfig(
          "PEM1", std::nullopt, "voltage_sensor", "current_sensor"),
      createPowerConsumptionConfig("PEM10", "power_sensor")};

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPowerConsumptionConfigWithVersionedSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";

  pmUnitSensors.sensors() = {
      createPmSensor("base_voltage", "/run/devmap/sensors/BASE_VOLTAGE"),
      createPmSensor("base_current", "/run/devmap/sensors/BASE_CURRENT")};

  // Add versioned sensors
  VersionedPmSensor versionedSensor;
  versionedSensor.productProductionState() = 1;
  versionedSensor.productVersion() = 2;
  versionedSensor.productSubVersion() = 3;
  versionedSensor.sensors() = {
      createPmSensor("versioned_power", "/run/devmap/sensors/VERSIONED_POWER")};
  pmUnitSensors.versionedSensors() = {versionedSensor};

  config.pmUnitSensorsList() = {pmUnitSensors};

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig(
          "PSU1", std::nullopt, "base_voltage", "base_current"),
      createPowerConsumptionConfig("PSU2", "versioned_power")};

  EXPECT_TRUE(ConfigValidator().isValid(config));
}
