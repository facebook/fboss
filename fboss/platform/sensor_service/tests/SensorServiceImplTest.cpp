// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <ranges>

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/logging/LoggerDB.h>
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

  // Rebuilds impl_ from config_ with loggedSensorNames set, captures stderr
  // (where folly XLOG writes the service log) across one poll cycle, and
  // returns the captured text. flushAllHandlers() forces the async log writer
  // to drain into the captured fd before we read it.
  std::string fetchAndCaptureLog(const std::vector<std::string>& loggedNames) {
    config_.loggedSensorNames() = loggedNames;
    impl_ = std::make_shared<SensorServiceImpl>(config_);
    folly::test::CaptureFD stderrCapture(STDERR_FILENO);
    impl_->fetchSensorData();
    folly::LoggerDB::get().flushAllHandlers();
    return stderrCapture.readIncremental();
  }

  std::string sensorPath(const std::string& name) {
    for (const auto& pmUnitSensors : *config_.pmUnitSensorsList()) {
      for (const auto& sensor : *pmUnitSensors.sensors()) {
        if (sensor.name() == name) {
          return *sensor.sysfsPath();
        }
      }
    }
    return {};
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

TEST_F(SensorServiceImplTest, fetchParsesHexSensorValue) {
  // CPLD regbit sysfs attrs can emit hex (e.g. "0xff"); these must parse
  // rather than be rejected. MOCK_FRU_SENSOR2 has no compute, so 0xff -> 255.
  const auto path = sensorPath("MOCK_FRU_SENSOR2");
  ASSERT_FALSE(path.empty());
  ASSERT_TRUE(folly::writeFile(std::string{"0xff"}, path.c_str()));

  impl_->fetchSensorData();
  auto sensorData = impl_->getAllSensorData();
  auto it = sensorData.find("MOCK_FRU_SENSOR2");
  ASSERT_TRUE(it != sensorData.end());
  ASSERT_TRUE(it->second.value().has_value());
  EXPECT_FLOAT_EQ(*it->second.value(), 255.0f);
}

TEST_F(SensorServiceImplTest, fetchHandlesUnparseableSensorValue) {
  // A single unparseable value must not throw out of the poll cycle: the bad
  // sensor degrades to a read failure while the others still publish.
  ASSERT_TRUE(
      folly::writeFile(
          std::string{"garbage"}, sensorPath("MOCK_FRU_SENSOR2").c_str()));

  ASSERT_NO_THROW(impl_->fetchSensorData());
  auto sensorData = impl_->getAllSensorData();
  EXPECT_FALSE(sensorData.at("MOCK_FRU_SENSOR2").value().has_value());
  EXPECT_TRUE(sensorData.at("MOCK_FRU_SENSOR1").value().has_value());
}

TEST_F(SensorServiceImplTest, logsConfiguredSensorValue) {
  // A configured logged sensor with a successful read emits its value to the
  // log, tagged so a single sensor's history can be grepped out (SEV S649086).
  auto log = fetchAndCaptureLog({"MOCK_FRU_SENSOR1"});
  EXPECT_NE(
      log.find(
          fmt::format(
              "{} MOCK_FRU_SENSOR1 = 0.025",
              SensorServiceImpl::kLoggedSensorPrefix)),
      std::string::npos)
      << log;
  // Sensors not in the list are not logged with the tag.
  EXPECT_EQ(
      log.find(
          fmt::format(
              "{} MOCK_FRU_SENSOR2", SensorServiceImpl::kLoggedSensorPrefix)),
      std::string::npos);
}

TEST_F(SensorServiceImplTest, logsReadFailureForConfiguredSensor) {
  // When a logged sensor cannot be read, the failure is surfaced explicitly
  // rather than dropped — this is the case the SEV needs to never lose.
  ASSERT_TRUE(std::filesystem::remove(sensorPath("MOCK_FRU_SENSOR1")));
  auto log = fetchAndCaptureLog({"MOCK_FRU_SENSOR1"});
  EXPECT_NE(
      log.find(
          fmt::format(
              "{} MOCK_FRU_SENSOR1 = READ_FAILED",
              SensorServiceImpl::kLoggedSensorPrefix)),
      std::string::npos)
      << log;
}

TEST_F(SensorServiceImplTest, logsNotCollectedForUnknownSensor) {
  // A configured name that is not collected this cycle is reported, not
  // silently skipped. (ConfigValidator rejects undefined names in production;
  // this exercises the runtime fail-safe directly.)
  auto log = fetchAndCaptureLog({"SENSOR_NOT_IN_THIS_CYCLE"});
  EXPECT_NE(
      log.find(
          fmt::format(
              "{} SENSOR_NOT_IN_THIS_CYCLE: sensor data missing",
              SensorServiceImpl::kLoggedSensorPrefix)),
      std::string::npos)
      << log;
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

  // SENSOR2 has critical violation, SENSOR3 has alarm violation.
  EXPECT_EQ(
      fb303::fbData->getCounter(
          SensorServiceImpl::kHasCriticalThresholdViolation),
      1);
  EXPECT_EQ(
      fb303::fbData->getCounter(SensorServiceImpl::kHasAlarmThresholdViolation),
      1);
}

} // namespace facebook::fboss
