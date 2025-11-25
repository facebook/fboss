// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fan_service/SensorData.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform::fan_service;

class SensorDataTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sensorData_ = std::make_unique<SensorData>();
  }

  std::unique_ptr<SensorData> sensorData_;
};

// Test getSensorEntry with non-existent sensor
TEST_F(SensorDataTest, GetSensorEntryNotFound) {
  auto entry = sensorData_->getSensorEntry("non_existent");
  EXPECT_FALSE(entry.has_value());
}

// Test updateSensorEntry and getSensorEntry
TEST_F(SensorDataTest, UpdateAndGetSensorEntry) {
  const std::string sensorName = "temp_sensor_1";
  const float value = 45.5;
  const uint64_t timestamp = 1234567890;

  sensorData_->updateSensorEntry(sensorName, value, timestamp);

  auto entry = sensorData_->getSensorEntry(sensorName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->name, sensorName);
  EXPECT_FLOAT_EQ(entry->value, value);
  EXPECT_EQ(entry->lastUpdated, timestamp);
}

// Test updating an existing sensor entry
TEST_F(SensorDataTest, UpdateExistingSensorEntry) {
  const std::string sensorName = "temp_sensor_1";

  // First update
  sensorData_->updateSensorEntry(sensorName, 30.0, 1000);

  // Second update with different values
  const float newValue = 50.0;
  const uint64_t newTimestamp = 2000;
  sensorData_->updateSensorEntry(sensorName, newValue, newTimestamp);

  auto entry = sensorData_->getSensorEntry(sensorName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_FLOAT_EQ(entry->value, newValue);
  EXPECT_EQ(entry->lastUpdated, newTimestamp);
}

// Test delSensorEntry
TEST_F(SensorDataTest, DeleteSensorEntry) {
  const std::string sensorName = "temp_sensor_1";
  sensorData_->updateSensorEntry(sensorName, 45.5, 1234567890);

  // Verify entry exists
  EXPECT_TRUE(sensorData_->getSensorEntry(sensorName).has_value());

  // Delete entry
  sensorData_->delSensorEntry(sensorName);

  // Verify entry no longer exists
  EXPECT_FALSE(sensorData_->getSensorEntry(sensorName).has_value());
}

// Test delSensorEntry with non-existent sensor (should not crash)
TEST_F(SensorDataTest, DeleteNonExistentSensorEntry) {
  EXPECT_NO_THROW(sensorData_->delSensorEntry("non_existent"));
}

// Test getSensorEntries
TEST_F(SensorDataTest, GetSensorEntries) {
  sensorData_->updateSensorEntry("sensor1", 30.0, 1000);
  sensorData_->updateSensorEntry("sensor2", 40.0, 2000);
  sensorData_->updateSensorEntry("sensor3", 50.0, 3000);

  auto entries = sensorData_->getSensorEntries();
  EXPECT_EQ(entries.size(), 3);
  EXPECT_TRUE(entries.find("sensor1") != entries.end());
  EXPECT_TRUE(entries.find("sensor2") != entries.end());
  EXPECT_TRUE(entries.find("sensor3") != entries.end());
}

// Test multiple sensor operations
TEST_F(SensorDataTest, MultipleSensorOperations) {
  // Add multiple sensors
  sensorData_->updateSensorEntry("sensor1", 30.0, 1000);
  sensorData_->updateSensorEntry("sensor2", 40.0, 2000);
  sensorData_->updateSensorEntry("sensor3", 50.0, 3000);

  // Verify count
  EXPECT_EQ(sensorData_->getSensorEntries().size(), 3);

  // Delete one
  sensorData_->delSensorEntry("sensor2");
  EXPECT_EQ(sensorData_->getSensorEntries().size(), 2);

  // Update one
  sensorData_->updateSensorEntry("sensor1", 35.0, 4000);
  auto entry = sensorData_->getSensorEntry("sensor1");
  ASSERT_TRUE(entry.has_value());
  EXPECT_FLOAT_EQ(entry->value, 35.0);
}
