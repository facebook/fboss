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
  pmUnitSensors1.sensors() = {
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
