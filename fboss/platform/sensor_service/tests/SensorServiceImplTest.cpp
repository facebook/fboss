// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <ranges>

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/tests/TestUtils.h"

using namespace facebook::fboss::platform::sensor_service;

namespace facebook::fboss {

class SensorServiceImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    config_ = getMockSensorConfig(tmpDir_.path().string());
    impl_ = std::make_shared<SensorServiceImpl>(config_);
  }

  void testFetchAndCheckSensorData(bool expectedSensorReadSuccess) {
    auto now = Utils::nowInSecs();
    impl_->fetchSensorData();
    auto allSensorData = impl_->getAllSensorData();
    auto expectedSensors = getMockSensorData();
    EXPECT_EQ(allSensorData.size(), expectedSensors.size());
    for (const auto& [sensorName, sensorValue] : expectedSensors) {
      auto it = allSensorData.find(sensorName);
      ASSERT_TRUE(it != allSensorData.end())
          << fmt::format("Sensor {} not found in results", sensorName);
      auto sensorData = it->second;
      EXPECT_EQ(sensorData.name(), sensorName);
      if (expectedSensorReadSuccess) {
        EXPECT_EQ(sensorData.value(), sensorValue);
        EXPECT_GE(sensorData.timeStamp(), now);
      } else {
        EXPECT_FALSE(sensorData.value().has_value());
        EXPECT_FALSE(sensorData.timeStamp().has_value());
      }
    }
  }

 protected:
  SensorConfig config_;
  // the tmpDir should remain present throughout the lifetime of the test
  // for SensorServiceImpl to read values from the sysfs entries here.
  folly::test::TemporaryDirectory tmpDir_;
  std::shared_ptr<SensorServiceImpl> impl_;
};

TEST_F(SensorServiceImplTest, fetchAndCheckSensorDataSuccess) {
  testFetchAndCheckSensorData(true);
}

TEST_F(SensorServiceImplTest, fetchAndCheckSensorDataFailure) {
  // Remove sensor sysfs files
  for (const auto& pmUnitSensors : *config_.pmUnitSensorsList()) {
    for (const auto& pmSensors : *pmUnitSensors.sensors()) {
      ASSERT_TRUE(std::filesystem::remove(*pmSensors.sysfsPath()));
    }
  }

  // Check that sensor value/timestamp are now empty which implies read failed
  testFetchAndCheckSensorData(false);
}

TEST_F(SensorServiceImplTest, fetchAndCheckSensorDataSuccesAndThenFailure) {
  // CASE 1: Success
  testFetchAndCheckSensorData(true);

  // CASE 2: Failure (with sysfs files disappearing)
  // Remove sensors' sensor files.
  for (const auto& pmUnitSensors : *config_.pmUnitSensorsList()) {
    for (const auto& pmSensors : *pmUnitSensors.sensors()) {
      ASSERT_TRUE(std::filesystem::remove(*pmSensors.sysfsPath()));
    }
  }
  // Check that sensor value/timestamp are now empty which implies read failed
  testFetchAndCheckSensorData(false);
}

TEST_F(SensorServiceImplTest, getSomeSensors) {
  auto mockSensorData = getMockSensorData();
  auto mockSensorName = *std::views::keys(mockSensorData).begin();
  auto now = Utils::nowInSecs();
  impl_->fetchSensorData();
  auto sensorData = impl_->getSensorsData({mockSensorName, "BOGUS_SENSOR"});
  EXPECT_EQ(sensorData.size(), 1);
  EXPECT_EQ(*sensorData[0].name(), mockSensorName);
  EXPECT_FLOAT_EQ(*sensorData[0].value(), mockSensorData[mockSensorName]);
  EXPECT_GE(*sensorData[0].timeStamp(), now);
}

} // namespace facebook::fboss
