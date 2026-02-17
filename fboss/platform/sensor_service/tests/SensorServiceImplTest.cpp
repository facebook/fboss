// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <ranges>

#include <fb303/ServiceData.h>
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
      EXPECT_EQ(*sensorData.slotPath(), "/");
      if (sensorName == "MOCK_FRU_SENSOR1") {
        EXPECT_TRUE(
            sensorData.sysfsPath()->ends_with("mock_fru_sensor_1_path:temp1"));
      } else if (sensorName == "MOCK_FRU_SENSOR2") {
        EXPECT_TRUE(
            sensorData.sysfsPath()->ends_with("mock_fru_sensor_2_path:fan1"));
      } else if (sensorName == "MOCK_FRU_SENSOR3") {
        EXPECT_TRUE(
            sensorData.sysfsPath()->ends_with("mock_fru_sensor_3_path:vin"));
      }

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

TEST_F(SensorServiceImplTest, publishPerSensorStats) {
  // Success case
  impl_->fetchSensorData();
  auto expectedSensors = getMockSensorData();
  for (const auto& [sensorName, sensorValue] : expectedSensors) {
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadValue, sensorName)),
        static_cast<int64_t>(sensorValue));
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadFailure, sensorName)),
        0);
  }

  // Verify threshold counters are set correctly
  // MOCK_FRU_SENSOR1: value=0.025, critical=[−40,100], alarm=[−20,85] → no
  // violation
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kCriticalThresholdViolation,
              "MOCK_FRU_SENSOR1")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAlarmThresholdViolation, "MOCK_FRU_SENSOR1")),
      0);

  // MOCK_FRU_SENSOR2: value=11152, critical=[1000,10000] → critical violation
  // (>10000)
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kCriticalThresholdViolation,
              "MOCK_FRU_SENSOR2")),
      1);

  // MOCK_FRU_SENSOR3: value=16.875, alarm=[10,14] → alarm violation (>14)
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAlarmThresholdViolation, "MOCK_FRU_SENSOR3")),
      1);

  // Failure case: remove sysfs files and re-fetch
  for (const auto& pmUnitSensors : *config_.pmUnitSensorsList()) {
    for (const auto& pmSensors : *pmUnitSensors.sensors()) {
      std::filesystem::remove(*pmSensors.sysfsPath());
    }
  }
  impl_->fetchSensorData();
  for (const auto& [sensorName, _] : expectedSensors) {
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadValue, sensorName)),
        0);
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format(SensorServiceImpl::kReadFailure, sensorName)),
        1);
  }
}

TEST_F(SensorServiceImplTest, publishAggStats) {
  impl_->fetchSensorData();

  // Mock sensors:
  // MOCK_FRU_SENSOR1: TEMPERATURE, value=0.025, critical=[-40,100],
  // alarm=[-20,85] → no violations
  // MOCK_FRU_SENSOR2: FAN, value=11152, critical=[1000,10000] → critical
  // violation
  // MOCK_FRU_SENSOR3: VOLTAGE, value=16.875, alarm=[10,14] → alarm violation

  // TEMPERATURE: no critical, no alarm
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAggHasCriticalThresholdViolation,
              "temperature")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAggHasAlarmThresholdViolation,
              "temperature")),
      0);

  // FAN: has critical, no alarm
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAggHasCriticalThresholdViolation, "fan")),
      1);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAggHasAlarmThresholdViolation, "fan")),
      0);

  // VOLTAGE: no critical, has alarm
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAggHasCriticalThresholdViolation, "voltage")),
      0);
  EXPECT_EQ(
      fb303::fbData->getCounter(
          fmt::format(
              SensorServiceImpl::kAggHasAlarmThresholdViolation, "voltage")),
      1);
}

} // namespace facebook::fboss
