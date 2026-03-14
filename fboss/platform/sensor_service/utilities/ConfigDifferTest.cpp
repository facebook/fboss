// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/utilities/ConfigDiffer.h"

#include <fmt/core.h>
#include <gtest/gtest.h>

using namespace facebook::fboss::platform::sensor_service;
namespace sensor_config = facebook::fboss::platform::sensor_config;

class ConfigDifferTest : public ::testing::Test {
 protected:
  static constexpr const char* kTestUnit = "TEST_UNIT";
  static constexpr const char* kSensorPathPrefix = "/run/devmap/sensors/";

  void SetUp() override {
    differ_ = ConfigDiffer();
  }

  sensor_config::PmSensor CreateTestSensor(
      const std::string& name,
      const std::string& pathSuffix = "temp1_input") {
    sensor_config::PmSensor sensor;
    sensor.name() = name;
    sensor.sysfsPath() =
        fmt::format("{}{}/{}", kSensorPathPrefix, name, pathSuffix);
    sensor.type() = sensor_config::SensorType::TEMPERATURE;
    return sensor;
  }

  sensor_config::VersionedPmSensor CreateVersionedPmSensor(
      int productionState,
      int version,
      int subVersion,
      const std::vector<sensor_config::PmSensor>& sensors) {
    sensor_config::VersionedPmSensor versionedPmSensor;
    versionedPmSensor.productProductionState() = productionState;
    versionedPmSensor.productVersion() = version;
    versionedPmSensor.productSubVersion() = subVersion;
    versionedPmSensor.sensors() = sensors;
    return versionedPmSensor;
  }

  sensor_config::PmUnitSensors CreatePmUnitSensors(
      const std::string& name,
      const std::vector<sensor_config::PmSensor>& sensors,
      const std::vector<sensor_config::VersionedPmSensor>& versionedSensors =
          {}) {
    sensor_config::PmUnitSensors pmUnitSensors;
    pmUnitSensors.pmUnitName() = name;
    pmUnitSensors.slotPath() = "/";
    pmUnitSensors.sensors() = sensors;
    pmUnitSensors.versionedSensors() = versionedSensors;
    return pmUnitSensors;
  }

  std::string CaptureOutput(const std::function<void()>& func) {
    testing::internal::CaptureStdout();
    func();
    return testing::internal::GetCapturedStdout();
  }

  ConfigDiffer differ_;
};

TEST_F(ConfigDifferTest, ComparePmSensors_Identical) {
  auto sensor = CreateTestSensor("TEST_SENSOR");
  std::vector<sensor_config::PmSensor> sensors = {sensor};

  differ_.comparePmSensors(sensors, sensors, kTestUnit, "default → v1");
  EXPECT_TRUE(differ_.getDiffs().empty());
}

TEST_F(ConfigDifferTest, ComparePmSensors_AddedSensor) {
  std::vector<sensor_config::PmSensor> emptySensors = {};
  std::vector<sensor_config::PmSensor> newSensors = {
      CreateTestSensor("TEST_SENSOR")};

  differ_.comparePmSensors(emptySensors, newSensors, kTestUnit, "default → v1");

  const auto& diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::ADDED);
  EXPECT_EQ(diffs[0].sensorName, "TEST_SENSOR");
  EXPECT_EQ(diffs[0].pmUnitName, kTestUnit);
  EXPECT_EQ(diffs[0].versionInfo, "default → v1");
  EXPECT_EQ(diffs[0].fieldName, "[ADDED]");
  EXPECT_FALSE(diffs[0].oldValue.has_value());
  EXPECT_FALSE(diffs[0].newValue.has_value());
}

TEST_F(ConfigDifferTest, CompareVersionedSensors_NoPmUnit) {
  sensor_config::SensorConfig config;

  differ_.compareVersionedSensors(config, "NonExistent");
  auto diffs = differ_.getDiffs();
  EXPECT_TRUE(diffs.empty());
}

TEST_F(ConfigDifferTest, CompareVersionedSensors_EmptyVersionedSensors) {
  sensor_config::SensorConfig config;
  config.pmUnitSensorsList() = {
      CreatePmUnitSensors(kTestUnit, {CreateTestSensor("DEFAULT_SENSOR")})};

  differ_.compareVersionedSensors(config, kTestUnit);
  EXPECT_TRUE(differ_.getDiffs().empty());
}

TEST_F(ConfigDifferTest, CompareVersionedSensors_WithVersionedSensors) {
  auto defaultSensor = CreateTestSensor("DEFAULT_SENSOR");
  auto versionedPmSensor = CreateVersionedPmSensor(
      1,
      2,
      3,
      {defaultSensor, CreateTestSensor("VERSIONED_SENSOR", "temp2_input")});

  sensor_config::SensorConfig config;
  config.pmUnitSensorsList() = {
      CreatePmUnitSensors(kTestUnit, {defaultSensor}, {versionedPmSensor})};

  differ_.compareVersionedSensors(config, kTestUnit);

  const auto& diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 1);
  EXPECT_EQ(diffs[0].type, DiffType::ADDED);
  EXPECT_EQ(diffs[0].sensorName, "VERSIONED_SENSOR");
  EXPECT_EQ(diffs[0].pmUnitName, kTestUnit);
  EXPECT_EQ(diffs[0].versionInfo, "default → v1.2.3");
  EXPECT_EQ(diffs[0].fieldName, "[ADDED]");
}

TEST_F(ConfigDifferTest, CompareVersionedSensors_AllPmUnits) {
  auto sensor1 = CreateTestSensor("SENSOR_1");
  auto sensor3 = CreateTestSensor("SENSOR_3", "temp3_input");

  auto versionedPmSensor1 =
      CreateVersionedPmSensor(1, 0, 0, {sensor1, CreateTestSensor("SENSOR_2")});
  auto versionedPmSensor2 = CreateVersionedPmSensor(
      2, 1, 0, {sensor3, CreateTestSensor("SENSOR_4", "temp4_input")});

  sensor_config::SensorConfig config;
  config.pmUnitSensorsList() = {
      CreatePmUnitSensors("UNIT_1", {sensor1}, {versionedPmSensor1}),
      CreatePmUnitSensors("UNIT_2", {sensor3}, {versionedPmSensor2})};

  differ_.compareVersionedSensors(config);

  const auto& diffs = differ_.getDiffs();
  ASSERT_EQ(diffs.size(), 2);
  EXPECT_EQ(diffs[0].type, DiffType::ADDED);
  EXPECT_EQ(diffs[0].sensorName, "SENSOR_2");
  EXPECT_EQ(diffs[0].pmUnitName, "UNIT_1");
  EXPECT_EQ(diffs[1].type, DiffType::ADDED);
  EXPECT_EQ(diffs[1].sensorName, "SENSOR_4");
  EXPECT_EQ(diffs[1].pmUnitName, "UNIT_2");
}

TEST_F(ConfigDifferTest, PrintDifferences_NoDiffs) {
  auto output = CaptureOutput([this]() { differ_.printDifferences(); });
  EXPECT_NE(output.find("No differences found."), std::string::npos);
}

TEST_F(ConfigDifferTest, PrintDifferences_WithDiffs) {
  auto sensor1 = CreateTestSensor("SENSOR_1");
  auto sensor2 = CreateTestSensor("SENSOR_2", "temp2_input");

  std::vector<sensor_config::PmSensor> sensors1 = {};
  std::vector<sensor_config::PmSensor> sensors2 = {sensor1, sensor2};

  differ_.comparePmSensors(sensors1, sensors2, kTestUnit, "default → v1.2.3");

  auto output = CaptureOutput([this]() { differ_.printDifferences(); });

  EXPECT_NE(output.find(kTestUnit), std::string::npos);
  EXPECT_NE(output.find("default → v1.2.3"), std::string::npos);
  EXPECT_NE(output.find("SENSOR_1"), std::string::npos);
  EXPECT_NE(output.find("SENSOR_2"), std::string::npos);
  EXPECT_NE(output.find("[ADDED]"), std::string::npos);
}
