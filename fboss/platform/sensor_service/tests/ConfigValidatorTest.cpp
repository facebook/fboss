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
      createPmSensor("SENSOR1", "/run/devmap/sensors/CPU_CORE_TEMP")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.sensors() = {
      createPmSensor("SENSOR2", "/run/devmap/sensors/BCB_FAN_CPLD")};
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
      createPmSensor("SENSOR1", "/run/devmap/eeproms/BCB_EEPROMS")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR1", "/run/devmap/sensors/CPU_CORE_TEMP"),
      createPmSensor("SENSOR1", "/run/devmap/sensors/CPU_CORE_TEMP2")};
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
      createPmSensor("VOLTAGE_SENSOR", "/run/devmap/sensors/VOLTAGE"),
      createPmSensor("CURRENT_SENSOR", "/run/devmap/sensors/CURRENT"),
      createPmSensor("POWER_SENSOR", "/run/devmap/sensors/POWER")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  return config;
}

TEST(ConfigValidatorTest, ValidPowerConsumptionConfig) {
  auto config = createBasicSensorConfig();

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "POWER_SENSOR")};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {createPowerConsumptionConfig(
      "PSU2", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PEM1", "POWER_SENSOR")};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig("PSU1", "POWER_SENSOR"),
      createPowerConsumptionConfig(
          "PSU2", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")};
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
      createPowerConsumptionConfig("PSU1", "POWER_SENSOR"),
      createPowerConsumptionConfig(
          "PSU1", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")};
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
      createPmSensor("TEMP_SENSOR", "/run/devmap/sensors/TEMP"),
      createPmSensor("FAN_SENSOR", "/run/devmap/sensors/FAN")};
  config.pmUnitSensorsList()->emplace_back(pmUnitSensors2);

  auto sensorNames = ConfigValidator().getAllSensorNames(config);

  EXPECT_EQ(sensorNames.size(), 5);
  EXPECT_TRUE(sensorNames.contains("VOLTAGE_SENSOR"));
  EXPECT_TRUE(sensorNames.contains("CURRENT_SENSOR"));
  EXPECT_TRUE(sensorNames.contains("POWER_SENSOR"));
  EXPECT_TRUE(sensorNames.contains("TEMP_SENSOR"));
  EXPECT_TRUE(sensorNames.contains("FAN_SENSOR"));
}

TEST(ConfigValidatorTest, ValidPowerConsumptionConfigComplexScenario) {
  auto config = createBasicSensorConfig();

  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU1_VOLTAGE", "/run/devmap/sensors/PSU1_VOLTAGE"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU1_CURRENT", "/run/devmap/sensors/PSU1_CURRENT"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU2_POWER", "/run/devmap/sensors/PSU2_POWER"));

  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig(
          "PSU1", std::nullopt, "PSU1_VOLTAGE", "PSU1_CURRENT"),
      createPowerConsumptionConfig("PSU2", "PSU2_POWER"),
      createPowerConsumptionConfig(
          "PEM1", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR"),
      createPowerConsumptionConfig("PEM10", "POWER_SENSOR")};

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidPowerConsumptionConfigWithVersionedSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";

  pmUnitSensors.sensors() = {
      createPmSensor("BASE_VOLTAGE", "/run/devmap/sensors/BASE_VOLTAGE"),
      createPmSensor("BASE_CURRENT", "/run/devmap/sensors/BASE_CURRENT")};

  VersionedPmSensor versionedPmSensor;
  versionedPmSensor.sensors() = {
      createPmSensor("VERSIONED_POWER", "/run/devmap/sensors/VERSIONED_POWER")};
  pmUnitSensors.versionedSensors() = {versionedPmSensor};

  config.pmUnitSensorsList() = {pmUnitSensors};
  config.powerConsumptionConfigs() = {
      createPowerConsumptionConfig(
          "PSU1", std::nullopt, "BASE_VOLTAGE", "BASE_CURRENT"),
      createPowerConsumptionConfig("PSU2", "VERSIONED_POWER")};

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

// Tests for AsicCommand validation
TEST(ConfigValidatorTest, ValidAsicCommand) {
  auto config = createBasicSensorConfig();

  // Test 1: Valid config without AsicCommand (optional field)
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 2: Valid config with AsicCommand
  AsicCommand asicCommand;
  asicCommand.sensorName() = "MGET_TEMP_CMD";
  asicCommand.cmd() = "mget_temp -d 0000:01:00.0";
  config.asicCommand() = asicCommand;
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidAsicCommand) {
  auto config = createBasicSensorConfig();
  AsicCommand asicCommand;

  // Test 1: Empty sensorName
  asicCommand.sensorName() = "";
  asicCommand.cmd() = "mget_temp -d 0000:01:00.0";
  config.asicCommand() = asicCommand;
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 2: Empty cmd
  asicCommand.sensorName() = "MGET_TEMP_CMD";
  asicCommand.cmd() = "";
  config.asicCommand() = asicCommand;
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 3: Missing sensorName
  asicCommand = AsicCommand();
  asicCommand.cmd() = "mget_temp -d 0000:01:00.0";
  config.asicCommand() = asicCommand;
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 4: Missing cmd
  asicCommand = AsicCommand();
  asicCommand.sensorName() = "MGET_TEMP_CMD";
  config.asicCommand() = asicCommand;
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 5: Sensor name conflicts with existing sensor
  asicCommand.sensorName() = "VOLTAGE_SENSOR";
  asicCommand.cmd() = "mget_temp -d 0000:01:00.0";
  config.asicCommand() = asicCommand;
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, AsicCommandWithVersionedSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";

  pmUnitSensors.sensors() = {
      createPmSensor("BASE_SENSOR", "/run/devmap/sensors/BASE")};

  // Add versioned sensor
  VersionedPmSensor versionedSensor;
  versionedSensor.productProductionState() = 1;
  versionedSensor.sensors() = {
      createPmSensor("VERSIONED_SENSOR", "/run/devmap/sensors/VERSIONED")};
  pmUnitSensors.versionedSensors() = {versionedSensor};

  config.pmUnitSensorsList() = {pmUnitSensors};

  // AsicCommand should not conflict with versioned sensor names
  AsicCommand asicCommand;
  asicCommand.sensorName() = "VERSIONED_SENSOR";
  asicCommand.cmd() = "echo 42";
  asicCommand.sensorType() = SensorType::TEMPERTURE;
  config.asicCommand() = asicCommand;
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Non-conflicting name should work
  asicCommand.sensorName() = "ASIC_CMD_SENSOR";
  config.asicCommand() = asicCommand;
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

// Test sensor name uppercase validation
TEST(ConfigValidatorTest, isValidPmSensor) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";

  // Test 1: empty sensor name - should fail
  pmUnitSensors.sensors() = {createPmSensor("", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 2: empty sysfsPath - should fail
  pmUnitSensors.sensors() = {createPmSensor("SENSOR_1", "")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 3: lowercase sensor name - should fail
  pmUnitSensors.sensors() = {
      createPmSensor("lowercase_sensor", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 4: mixed case sensor name - should fail
  pmUnitSensors.sensors() = {
      createPmSensor("MixedCase_Sensor", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 5: hyphen instead of underscore - should fail
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR-NAME", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 6: space in name - should fail
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR NAME", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 7: dot in name - should fail (dots are not allowed)
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR.NAME", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 8: multiple sensors with one lowercase - should fail
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR_1", "/run/devmap/sensors/SENSOR1"),
      createPmSensor("SENSOR_2", "/run/devmap/sensors/SENSOR2"),
      createPmSensor("lowercase", "/run/devmap/sensors/SENSOR3")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 9: versioned sensor with lowercase name - should fail
  pmUnitSensors.sensors() = {
      createPmSensor("BASE_SENSOR", "/run/devmap/sensors/BASE")};
  VersionedPmSensor versionedSensor;
  versionedSensor.productProductionState() = 1;
  versionedSensor.sensors() = {
      createPmSensor("versioned_sensor", "/run/devmap/sensors/VERSIONED")};
  pmUnitSensors.versionedSensors() = {versionedSensor};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 10: uppercase with numbers - should pass
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR_123", "/run/devmap/sensors/SENSOR1")};
  pmUnitSensors.versionedSensors() = {};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 11: all uppercase - should pass
  pmUnitSensors.sensors() = {
      createPmSensor("UPPERCASE_SENSOR", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 12: only underscores and uppercase - should pass
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR_NAME_TEST", "/run/devmap/sensors/SENSOR1")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 13: multiple uppercase sensors - should pass
  pmUnitSensors.sensors() = {
      createPmSensor("SENSOR_1", "/run/devmap/sensors/SENSOR1"),
      createPmSensor("SENSOR_2", "/run/devmap/sensors/SENSOR2"),
      createPmSensor("SENSOR_3", "/run/devmap/sensors/SENSOR3")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 14: versioned sensor with uppercase name - should pass
  versionedSensor.sensors() = {
      createPmSensor("VERSIONED_SENSOR", "/run/devmap/sensors/VERSIONED")};
  pmUnitSensors.versionedSensors() = {versionedSensor};
  config.pmUnitSensorsList() = {pmUnitSensors};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}
