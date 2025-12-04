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

PerSlotPowerConfig createPerSlotPowerConfig(
    const std::string& name,
    const std::optional<std::string>& powerSensorName = std::nullopt,
    const std::optional<std::string>& voltageSensorName = std::nullopt,
    const std::optional<std::string>& currentSensorName = std::nullopt) {
  PerSlotPowerConfig config;
  config.name() = name;

  config.powerSensorName().from_optional(powerSensorName);
  config.voltageSensorName().from_optional(voltageSensorName);
  config.currentSensorName().from_optional(currentSensorName);
  return config;
}

PowerConfig createPowerConfig(
    const std::vector<PerSlotPowerConfig>& perSlotConfigs = {},
    const std::vector<std::string>& otherPowerSensorNames = {},
    double powerDelta = 0.0,
    const std::vector<std::string>& inputVoltageSensors = {}) {
  PowerConfig config;
  config.perSlotPowerConfigs() = perSlotConfigs;
  config.otherPowerSensorNames() = otherPowerSensorNames;
  config.powerDelta() = powerDelta;
  config.inputVoltageSensors() = inputVoltageSensors;
  return config;
}
TemperatureConfig createTemperatureConfig(
    const std::string& name,
    const std::vector<std::string>& temperatureSensorNames) {
  TemperatureConfig config;
  config.name() = name;
  config.temperatureSensorNames() = temperatureSensorNames;
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
      createPmSensor("POWER_SENSOR", "/run/devmap/sensors/POWER"),
      createPmSensor("TEMP_SENSOR", "/run/devmap/sensors/TEMP")};
  config.pmUnitSensorsList() = {pmUnitSensors};
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"TEMP_SENSOR"})};
  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "PSU1", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")});
  return config;
}

} // namespace

TEST(ConfigValidatorTest, ValidConfig) {
  auto config = SensorConfig();
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/";
  pmUnitSensors1.sensors() = {
      createPmSensor("SENSOR1", "/run/devmap/sensors/CPU_CORE_TEMP"),
      createPmSensor("POWER_SENSOR", "/run/devmap/sensors/POWER")};
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.sensors() = {
      createPmSensor("SENSOR2", "/run/devmap/sensors/BCB_FAN_CPLD")};
  config.pmUnitSensorsList() = {pmUnitSensors1, pmUnitSensors2};
  config.temperatureConfigs() = {createTemperatureConfig("ASIC", {"SENSOR1"})};
  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PSU1", "POWER_SENSOR")});
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, SlotPaths) {
  // Valid (SlotPath,PmUnitName) pairing.
  PmUnitSensors pmUnitSensors1, pmUnitSensors2;
  pmUnitSensors1.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors1.pmUnitName() = "BCB";
  pmUnitSensors2.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors2.pmUnitName() = "BCB2";
  std::vector<PmUnitSensors> pmUnitSensorsList = {
      pmUnitSensors1, pmUnitSensors2};
  EXPECT_TRUE(ConfigValidator().isValidPmUnitSensorsList(pmUnitSensorsList));
  // Duplicate (SlotPath, PmUnitName) pairing
  pmUnitSensors2.pmUnitName() = "BCB";
  pmUnitSensorsList = {pmUnitSensors1, pmUnitSensors2};
  EXPECT_FALSE(ConfigValidator().isValidPmUnitSensorsList(pmUnitSensorsList));
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

TEST(ConfigValidatorTest, ValidPowerConfig) {
  auto config = createBasicSensorConfig();

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PSU1", "POWER_SENSOR")});
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "PSU2", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")});
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PEM1", "POWER_SENSOR")});
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig(
      {createPerSlotPowerConfig("PSU1", "POWER_SENSOR"),
       createPerSlotPowerConfig(
           "PSU2", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")});
  EXPECT_TRUE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "HSC", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")});
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConfigMissingPerSlotPowerConfigs) {
  auto config = createBasicSensorConfig();

  // Test 1: PowerConfig with no perSlotPowerConfigs (empty vector)
  config.powerConfig() = createPowerConfig({});
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConfigNaming) {
  auto config = createBasicSensorConfig();

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("INVALID1", "power_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PSU", "power_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PSU0", "power_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig(
      {createPerSlotPowerConfig("POWER_UNIT", "power_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("HSC1", "power_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConfigDuplicateName) {
  auto config = createBasicSensorConfig();

  // Duplicate per-slot power config name
  config.powerConfig() = createPowerConfig(
      {createPerSlotPowerConfig("PSU1", "POWER_SENSOR"),
       createPerSlotPowerConfig(
           "PSU1", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR")});
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConfigNameConflictWithSensor) {
  auto config = createBasicSensorConfig();

  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PSU1", "power_sensor")});

  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU1", "/run/devmap/sensors/PSU1_SENSOR"));

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConfigInvalidSensorReferences) {
  auto config = createBasicSensorConfig();

  config.powerConfig() = createPowerConfig(
      {createPerSlotPowerConfig("PSU1", "nonexistent_power_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "PSU1", std::nullopt, "nonexistent_voltage_sensor", "current_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "PSU1", std::nullopt, "voltage_sensor", "nonexistent_current_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPowerConfigMissingSensors) {
  auto config = createBasicSensorConfig();

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "PSU1", std::nullopt, std::nullopt, "current_sensor")});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig(
      "PSU1", std::nullopt, "voltage_sensor", std::nullopt)});
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config.powerConfig() = createPowerConfig({createPerSlotPowerConfig("PSU1")});
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InValidPowerConfigWithVersionedSensors) {
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
  config.powerConfig() = createPowerConfig(
      {createPerSlotPowerConfig(
           "PSU1", std::nullopt, "BASE_VOLTAGE", "BASE_CURRENT"),
       createPerSlotPowerConfig("PSU2", "VERSIONED_POWER")});

  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, GetAllSensorNames) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";
  pmUnitSensors.sensors() = {
      createPmSensor("BASE_SENSOR_1", "/run/devmap/sensors/BASE_1"),
      createPmSensor("BASE_SENSOR_2", "/run/devmap/sensors/BASE_2")};
  VersionedPmSensor versionedPmSensor;
  versionedPmSensor.productProductionState() = 1;
  versionedPmSensor.sensors() = {
      createPmSensor("VERSIONED_SENSOR_1", "/run/devmap/sensors/VERSIONED_1"),
      createPmSensor("VERSIONED_SENSOR_2", "/run/devmap/sensors/VERSIONED_2")};
  pmUnitSensors.versionedSensors() = {versionedPmSensor};
  config.pmUnitSensorsList() = {pmUnitSensors};
  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_CMD";
  asicCommand.cmd() = "echo 42";
  config.asicCommand() = asicCommand;

  auto allSensorNames = ConfigValidator().getAllSensorNames(config);
  EXPECT_EQ(allSensorNames.size(), 5);
  EXPECT_TRUE(allSensorNames.contains("BASE_SENSOR_1"));
  EXPECT_TRUE(allSensorNames.contains("BASE_SENSOR_2"));
  EXPECT_TRUE(allSensorNames.contains("VERSIONED_SENSOR_1"));
  EXPECT_TRUE(allSensorNames.contains("VERSIONED_SENSOR_2"));
  EXPECT_TRUE(allSensorNames.contains("ASIC_TEMP_CMD"));
}

TEST(ConfigValidatorTest, GetAllUniversalSensorNames) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/SCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "SCB";
  pmUnitSensors.sensors() = {
      createPmSensor("TEMP_SENSOR", "/run/devmap/sensors/TEMP"),
      createPmSensor("FAN_SENSOR", "/run/devmap/sensors/FAN")};
  VersionedPmSensor versionedPmSensor;
  versionedPmSensor.productProductionState() = 1;
  versionedPmSensor.sensors() = {
      createPmSensor("VERSIONED_TEMP", "/run/devmap/sensors/VERSIONED_TEMP"),
      createPmSensor("VERSIONED_FAN", "/run/devmap/sensors/VERSIONED_FAN")};
  pmUnitSensors.versionedSensors() = {versionedPmSensor};
  config.pmUnitSensorsList() = {pmUnitSensors};
  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_CMD";
  asicCommand.cmd() = "echo 42";
  config.asicCommand() = asicCommand;

  auto universalSensorNames =
      ConfigValidator().getAllUniversalSensorNames(config);
  EXPECT_EQ(universalSensorNames.size(), 3);
  EXPECT_TRUE(universalSensorNames.contains("TEMP_SENSOR"));
  EXPECT_TRUE(universalSensorNames.contains("FAN_SENSOR"));
  EXPECT_TRUE(universalSensorNames.contains("ASIC_TEMP_CMD"));
}

TEST(ConfigValidatorTest, ValidPowerConfigComplexScenario) {
  auto config = createBasicSensorConfig();

  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU1_VOLTAGE", "/run/devmap/sensors/PSU1_VOLTAGE"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU1_CURRENT", "/run/devmap/sensors/PSU1_CURRENT"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("PSU2_POWER", "/run/devmap/sensors/PSU2_POWER"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("FAN1_POWER", "/run/devmap/sensors/FAN1_POWER"));

  config.powerConfig() = createPowerConfig(
      {createPerSlotPowerConfig(
           "PSU1", std::nullopt, "PSU1_VOLTAGE", "PSU1_CURRENT"),
       createPerSlotPowerConfig("PSU2", "PSU2_POWER"),
       createPerSlotPowerConfig(
           "PEM1", std::nullopt, "VOLTAGE_SENSOR", "CURRENT_SENSOR"),
       createPerSlotPowerConfig("PEM10", "POWER_SENSOR")},
      {"FAN1_POWER"},
      10.5);

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
  EXPECT_FALSE(ConfigValidator().isValidAsicCommand(config));

  // Non-conflicting name should work
  asicCommand.sensorName() = "ASIC_CMD_SENSOR";
  config.asicCommand() = asicCommand;
  EXPECT_TRUE(ConfigValidator().isValidAsicCommand(config));
}

TEST(ConfigValidatorTest, ValidTemperatureConfig) {
  auto config = createBasicSensorConfig();

  // Add temperature sensors
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_1", "/run/devmap/sensors/TEMP1"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_2", "/run/devmap/sensors/TEMP2"));

  // Test 1: Valid ASIC config
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"TEMP_SENSOR_1"})};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 2: Valid ASIC1 config
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC1", {"TEMP_SENSOR_1"})};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 3: Valid ASIC with multiple sensors
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"TEMP_SENSOR_1", "TEMP_SENSOR_2"})};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 4: Multiple ASIC configs
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC1", {"TEMP_SENSOR_1"}),
      createTemperatureConfig("ASIC2", {"TEMP_SENSOR_2"})};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 5: Higher numbered ASIC
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC100", {"TEMP_SENSOR_1"})};
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigEmpty) {
  auto config = createBasicSensorConfig();
  // Test: Empty temperatureConfigs should be invalid
  config.temperatureConfigs() = {};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigNaming) {
  auto config = createBasicSensorConfig();

  // Add temperature sensors
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_1", "/run/devmap/sensors/TEMP1"));

  // Test 1: Invalid name pattern - lowercase
  config.temperatureConfigs() = {
      createTemperatureConfig("asic1", {"TEMP_SENSOR_1"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 2: Invalid name pattern - wrong prefix
  config.temperatureConfigs() = {
      createTemperatureConfig("CHIP1", {"TEMP_SENSOR_1"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 3: Invalid name pattern - ASIC0
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC0", {"TEMP_SENSOR_1"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 4: Invalid name pattern - ASIC with underscore
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC_1", {"TEMP_SENSOR_1"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 5: Invalid name pattern - just a number
  config.temperatureConfigs() = {
      createTemperatureConfig("1", {"TEMP_SENSOR_1"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigDuplicateName) {
  auto config = createBasicSensorConfig();

  // Add temperature sensors
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_1", "/run/devmap/sensors/TEMP1"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_2", "/run/devmap/sensors/TEMP2"));

  // Duplicate temperature config name
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC1", {"TEMP_SENSOR_1"}),
      createTemperatureConfig("ASIC1", {"TEMP_SENSOR_2"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigNameConflictWithSensor) {
  auto config = createBasicSensorConfig();

  // Add a sensor named "ASIC1"
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("ASIC1", "/run/devmap/sensors/ASIC1_SENSOR"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_1", "/run/devmap/sensors/TEMP1"));

  // Temperature config name conflicts with existing sensor
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC1", {"TEMP_SENSOR_1"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigInvalidSensorReferences) {
  auto config = createBasicSensorConfig();

  // Add temperature sensors
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_1", "/run/devmap/sensors/TEMP1"));

  // Test 1: Reference to non-existent sensor
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"NONEXISTENT_TEMP_SENSOR"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 2: One valid and one invalid sensor reference
  config.temperatureConfigs() = {createTemperatureConfig(
      "ASIC", {"TEMP_SENSOR_1", "NONEXISTENT_TEMP_SENSOR"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigEmptySensorList) {
  auto config = createBasicSensorConfig();

  // Test 1: Empty temperature sensor list
  config.temperatureConfigs() = {createTemperatureConfig("ASIC", {})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 2: Multiple configs, one with empty list
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("TEMP_SENSOR_1", "/run/devmap/sensors/TEMP1"));

  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC1", {"TEMP_SENSOR_1"}),
      createTemperatureConfig("ASIC2", {})};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidTemperatureConfigWithVersionedSensors) {
  SensorConfig config;
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";

  // Add base sensor
  pmUnitSensors.sensors() = {
      createPmSensor("BASE_TEMP", "/run/devmap/sensors/BASE_TEMP"),
      createPmSensor("POWER_SENSOR", "/run/devmap/sensors/POWER")};

  // Add versioned sensor
  VersionedPmSensor versionedPmSensor;
  versionedPmSensor.sensors() = {
      createPmSensor("VERSIONED_TEMP", "/run/devmap/sensors/VERSIONED_TEMP")};
  pmUnitSensors.versionedSensors() = {versionedPmSensor};

  config.pmUnitSensorsList() = {pmUnitSensors};
  config.powerConfig() =
      createPowerConfig({createPerSlotPowerConfig("PSU1", "POWER_SENSOR")});

  // Test 1: Valid config with base sensor
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"BASE_TEMP"})};
  EXPECT_TRUE(ConfigValidator().isValid(config));

  // Test 2: Invalid config with versioned sensor
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"VERSIONED_TEMP"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));

  // Test 3: Invalid config with mix of base and versioned sensors
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC", {"BASE_TEMP", "VERSIONED_TEMP"})};
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, ValidTemperatureConfigComplexScenario) {
  auto config = createBasicSensorConfig();

  // Add multiple temperature sensors
  auto& pmUnitSensors = config.pmUnitSensorsList()->at(0);
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("ASIC1_TEMP_1", "/run/devmap/sensors/ASIC1_TEMP_1"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("ASIC1_TEMP_2", "/run/devmap/sensors/ASIC1_TEMP_2"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("ASIC2_TEMP_1", "/run/devmap/sensors/ASIC2_TEMP_1"));
  pmUnitSensors.sensors()->emplace_back(
      createPmSensor("ASIC2_TEMP_2", "/run/devmap/sensors/ASIC2_TEMP_2"));

  // Multiple ASIC configs with multiple sensors each
  config.temperatureConfigs() = {
      createTemperatureConfig("ASIC1", {"ASIC1_TEMP_1", "ASIC1_TEMP_2"}),
      createTemperatureConfig("ASIC2", {"ASIC2_TEMP_1", "ASIC2_TEMP_2"}),
      createTemperatureConfig("ASIC", {"ASIC1_TEMP_1"})};

  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, IsValidSensorName) {
  auto config = createBasicSensorConfig();

  ConfigValidator validator;

  // Test 1: Regular sensor names should be valid
  EXPECT_TRUE(validator.isValidSensorName(config, "VOLTAGE_SENSOR"));
  EXPECT_TRUE(validator.isValidSensorName(config, "CURRENT_SENSOR"));
  EXPECT_TRUE(validator.isValidSensorName(config, "POWER_SENSOR"));

  // Test 2: Non-existent sensor name should be invalid
  EXPECT_FALSE(validator.isValidSensorName(config, "NONEXISTENT_SENSOR"));

  // Test 3: AsicCommand sensor name should be valid
  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_SENSOR";
  asicCommand.cmd() = "echo 42";
  config.asicCommand() = asicCommand;

  EXPECT_TRUE(validator.isValidSensorName(config, "ASIC_TEMP_SENSOR"));
  EXPECT_TRUE(validator.isValidSensorName(config, "VOLTAGE_SENSOR"));
  EXPECT_FALSE(validator.isValidSensorName(config, "NONEXISTENT_SENSOR"));

  // Test 4: After removing AsicCommand, its sensor name should be invalid
  config.asicCommand().reset();
  EXPECT_FALSE(validator.isValidSensorName(config, "ASIC_TEMP_SENSOR"));
  EXPECT_TRUE(validator.isValidSensorName(config, "VOLTAGE_SENSOR"));
}

// Test sensor name uppercase validation
TEST(ConfigValidatorTest, isValidPmSensor) {
  // Test 1: empty sensor name - should fail
  auto pmSensor = createPmSensor("", "/run/devmap/sensors/SENSOR1");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 2: empty sysfsPath - should fail
  pmSensor = createPmSensor("SENSOR_NAME", "");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 3: lowercase sensor name - should fail
  pmSensor = createPmSensor("lowercase_sensor", "/run/devmap/sensors/SENSOR1");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 4: mixed case sensor name - should fail
  pmSensor = createPmSensor("MixedCase_Sensor", "/run/devmap/sensors/SENSOR1");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 5: hyphen instead of underscore - should fail
  pmSensor = createPmSensor("SENSOR-NAME", "/run/devmap/sensors/SENSOR1");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 6: space in name - should fail
  pmSensor = createPmSensor("SENSOR NAME", "/run/devmap/sensors/SENSOR1");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 7: dot in name - should fail (dots are not allowed)
  pmSensor = createPmSensor("SENSOR.NAME", "/run/devmap/sensors/SENSOR1");
  EXPECT_FALSE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 8: multiple sensors with one lowercase - should fail
  std::vector<PmSensor> pmSensors8 = {
      createPmSensor("SENSOR_1", "/run/devmap/sensors/SENSOR1"),
      createPmSensor("SENSOR_2", "/run/devmap/sensors/SENSOR2"),
      createPmSensor("lowercase", "/run/devmap/sensors/SENSOR3")};
  EXPECT_FALSE(ConfigValidator().isValidPmSensors(pmSensors8));

  // Test 9: versioned sensor with lowercase name - should fail
  PmUnitSensors pmUnitSensors;
  pmUnitSensors.slotPath() = "/BCB_SLOT@0";
  pmUnitSensors.pmUnitName() = "BCB";
  pmUnitSensors.sensors() = {
      createPmSensor("BASE_SENSOR", "/run/devmap/sensors/BASE")};
  VersionedPmSensor versionedSensor;
  versionedSensor.productProductionState() = 1;
  versionedSensor.sensors() = {
      createPmSensor("versioned_sensor", "/run/devmap/sensors/VERSIONED")};
  pmUnitSensors.versionedSensors() = {versionedSensor};
  EXPECT_FALSE(ConfigValidator().isValidPmUnitSensorsList({pmUnitSensors}));

  // Test 10: uppercase with numbers - should pass
  pmSensor = createPmSensor("SENSOR_123", "/run/devmap/sensors/SENSOR1");
  EXPECT_TRUE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 11: all uppercase - should pass
  pmSensor = createPmSensor("UPPERCASE_SENSOR", "/run/devmap/sensors/SENSOR1");
  EXPECT_TRUE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 12: only underscores and uppercase - should pass
  pmSensor = createPmSensor("SENSOR_NAME_TEST", "/run/devmap/sensors/SENSOR1");
  EXPECT_TRUE(ConfigValidator().isValidPmSensor(pmSensor));

  // Test 13: multiple uppercase sensors - should pass
  std::vector<PmSensor> pmSensors13 = {
      createPmSensor("SENSOR_1", "/run/devmap/sensors/SENSOR1"),
      createPmSensor("SENSOR_2", "/run/devmap/sensors/SENSOR2"),
      createPmSensor("SENSOR_3", "/run/devmap/sensors/SENSOR3")};
  EXPECT_TRUE(ConfigValidator().isValidPmSensors(pmSensors13));

  // Test 14: versioned sensor with uppercase name - should pass
  versionedSensor.sensors() = {
      createPmSensor("VERSIONED_SENSOR", "/run/devmap/sensors/VERSIONED")};
  pmUnitSensors.versionedSensors() = {versionedSensor};
  EXPECT_TRUE(ConfigValidator().isValidPmUnitSensorsList({pmUnitSensors}));
}
