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

// Test getOpticEntry with non-existent optic
TEST_F(SensorDataTest, GetOpticEntryNotFound) {
  auto entry = sensorData_->getOpticEntry("non_existent");
  EXPECT_FALSE(entry.has_value());
}

// Test updateOpticEntry and getOpticEntry
TEST_F(SensorDataTest, UpdateAndGetOpticEntry) {
  const std::string opticName = "optic_1";
  std::vector<std::pair<std::string, float>> data = {
      {"temp_type_1", 35.0}, {"temp_type_2", 40.0}};
  const uint64_t qsfpTimestamp = 9876543210;

  sensorData_->updateOpticEntry(opticName, data, qsfpTimestamp);

  auto entry = sensorData_->getOpticEntry(opticName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->data.size(), 2);
  EXPECT_EQ(entry->data[0].first, "temp_type_1");
  EXPECT_FLOAT_EQ(entry->data[0].second, 35.0);
  EXPECT_EQ(entry->data[1].first, "temp_type_2");
  EXPECT_FLOAT_EQ(entry->data[1].second, 40.0);
  EXPECT_EQ(entry->qsfpServiceTimeStamp, qsfpTimestamp);
  EXPECT_GT(entry->lastOpticsUpdateTimeInSec, 0);
}

// Test updating an existing optic entry
TEST_F(SensorDataTest, UpdateExistingOpticEntry) {
  const std::string opticName = "optic_1";

  // First update
  std::vector<std::pair<std::string, float>> data1 = {{"temp_type_1", 30.0}};
  sensorData_->updateOpticEntry(opticName, data1, 1000);

  // Second update with different data
  std::vector<std::pair<std::string, float>> data2 = {
      {"temp_type_1", 35.0}, {"temp_type_2", 40.0}};
  const uint64_t newTimestamp = 2000;
  sensorData_->updateOpticEntry(opticName, data2, newTimestamp);

  auto entry = sensorData_->getOpticEntry(opticName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->data.size(), 2);
  EXPECT_EQ(entry->qsfpServiceTimeStamp, newTimestamp);
}

// Test resetOpticData
TEST_F(SensorDataTest, ResetOpticData) {
  const std::string opticName = "optic_1";
  std::vector<std::pair<std::string, float>> data = {{"temp_type_1", 35.0}};
  sensorData_->updateOpticEntry(opticName, data, 1000);

  // Verify data exists
  auto entry = sensorData_->getOpticEntry(opticName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->data.size(), 1);

  // Reset optic data
  sensorData_->resetOpticData(opticName);

  // Verify data is empty but entry still exists
  entry = sensorData_->getOpticEntry(opticName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->data.size(), 0);
}

// Test resetOpticData on non-existent optic (should create empty entry)
TEST_F(SensorDataTest, ResetNonExistentOpticData) {
  const std::string opticName = "new_optic";

  // Reset non-existent optic
  sensorData_->resetOpticData(opticName);

  // Verify entry was created with empty data
  auto entry = sensorData_->getOpticEntry(opticName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->data.size(), 0);
}

// Test updateOpticDataProcessingTimestamp
TEST_F(SensorDataTest, UpdateOpticDataProcessingTimestamp) {
  const std::string opticName = "optic_1";
  const uint64_t timestamp = 5555555555;

  sensorData_->updateOpticDataProcessingTimestamp(opticName, timestamp);

  auto entry = sensorData_->getOpticEntry(opticName);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->dataProcessTimeStamp, timestamp);
}

// Test getOpticEntries
TEST_F(SensorDataTest, GetOpticEntries) {
  std::vector<std::pair<std::string, float>> data1 = {{"type1", 30.0}};
  std::vector<std::pair<std::string, float>> data2 = {{"type2", 40.0}};

  sensorData_->updateOpticEntry("optic1", data1, 1000);
  sensorData_->updateOpticEntry("optic2", data2, 2000);

  const auto& entries = sensorData_->getOpticEntries();
  EXPECT_EQ(entries.size(), 2);
  EXPECT_TRUE(entries.find("optic1") != entries.end());
  EXPECT_TRUE(entries.find("optic2") != entries.end());
}

// Test getLastQsfpSvcTime
TEST_F(SensorDataTest, GetLastQsfpSvcTime) {
  // Initially should be 0 or uninitialized
  EXPECT_EQ(sensorData_->getLastQsfpSvcTime(), 0);

  // Update optic entry which sets lastSuccessfulQsfpServiceContact_
  const uint64_t timestamp1 = 1111111111;
  std::vector<std::pair<std::string, float>> data = {{"type1", 30.0}};
  sensorData_->updateOpticEntry("optic1", data, timestamp1);

  EXPECT_EQ(sensorData_->getLastQsfpSvcTime(), timestamp1);

  // Update with newer timestamp
  const uint64_t timestamp2 = 2222222222;
  sensorData_->updateOpticEntry("optic2", data, timestamp2);

  EXPECT_EQ(sensorData_->getLastQsfpSvcTime(), timestamp2);
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

// Test multiple optic operations
TEST_F(SensorDataTest, MultipleOpticOperations) {
  std::vector<std::pair<std::string, float>> data1 = {{"type1", 30.0}};
  std::vector<std::pair<std::string, float>> data2 = {
      {"type2", 40.0}, {"type3", 45.0}};

  // Add multiple optics
  sensorData_->updateOpticEntry("optic1", data1, 1000);
  sensorData_->updateOpticEntry("optic2", data2, 2000);

  EXPECT_EQ(sensorData_->getOpticEntries().size(), 2);

  // Reset one
  sensorData_->resetOpticData("optic1");
  auto entry = sensorData_->getOpticEntry("optic1");
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->data.size(), 0);

  // Update processing timestamp
  sensorData_->updateOpticDataProcessingTimestamp("optic2", 5555);
  entry = sensorData_->getOpticEntry("optic2");
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->dataProcessTimeStamp, 5555);
}
