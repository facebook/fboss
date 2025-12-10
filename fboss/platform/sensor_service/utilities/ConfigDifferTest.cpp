// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/utilities/ConfigDiffer.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform::sensor_service;
namespace sensor_config = facebook::fboss::platform::sensor_config;

class ConfigDifferTest : public ::testing::Test {
 protected:
  ConfigDiffer differ_;
};

TEST_F(ConfigDifferTest, ComparePmUnitSensors_Identical) {
  sensor_config::PmUnitSensors config1;
  config1.pmUnitName() = "TEST_UNIT";
  config1.slotPath() = "/";

  sensor_config::PmSensor sensor;
  sensor.name() = "TEST_SENSOR";
  sensor.sysfsPath() = "/run/devmap/sensors/TEST/temp1_input";
  sensor.type() = sensor_config::SensorType::TEMPERTURE;

  config1.sensors() = {sensor};

  sensor_config::PmUnitSensors config2;
  config2.pmUnitName() = "TEST_UNIT";
  config2.slotPath() = "/";
  config2.sensors() = {sensor};

  differ_.comparePmSensors(
      *config1.sensors(), *config2.sensors(), "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, ComparePmSensors_AddedSensor) {
  sensor_config::PmUnitSensors config1;
  config1.pmUnitName() = "TEST_UNIT";
  config1.slotPath() = "/";
  config1.sensors() = {};

  sensor_config::PmUnitSensors config2;
  config2.pmUnitName() = "TEST_UNIT";
  config2.slotPath() = "/";

  sensor_config::PmSensor sensor;
  sensor.name() = "TEST_SENSOR";
  sensor.sysfsPath() = "/run/devmap/sensors/TEST/temp1_input";
  sensor.type() = sensor_config::SensorType::TEMPERTURE;

  config2.sensors() = {sensor};

  differ_.comparePmSensors(
      *config1.sensors(), *config2.sensors(), "TEST_UNIT", "default → v1");
  auto diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::ADDED);
  EXPECT_EQ(diffs[0].sensorName, "TEST_SENSOR");
}

TEST_F(ConfigDifferTest, CompareVersionedSensors_NoPmUnit) {
  sensor_config::SensorConfig config;

  differ_.compareVersionedSensors(config, "NonExistent");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}
