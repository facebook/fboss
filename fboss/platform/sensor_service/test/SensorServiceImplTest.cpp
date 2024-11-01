// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <ranges>

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"
#include "fboss/platform/sensor_service/test/TestUtils.h"

using namespace facebook::fboss::platform::sensor_service;

namespace facebook::fboss {

class SensorServiceImplTest : public ::testing::Test {
 public:
  void testFetchAndCheckSensorData(
      std::shared_ptr<SensorServiceImpl> sensorServiceImpl,
      bool expectedSensorReadSuccess) {
    auto now = Utils::nowInSecs();
    sensorServiceImpl->fetchSensorData();
    auto sensorData = sensorServiceImpl->getAllSensorData();
    auto expectedSensors = getDefaultMockSensorData();
    EXPECT_EQ(sensorData.size(), expectedSensors.size());
    for (const auto& it : expectedSensors) {
      EXPECT_TRUE(sensorData.find(it.first) != sensorData.end());
      EXPECT_EQ(*sensorData[it.first].name(), it.first);
      if (expectedSensorReadSuccess) {
        EXPECT_EQ(*sensorData[it.first].value(), it.second);
        EXPECT_GE(*sensorData[it.first].timeStamp(), now);
      } else {
        EXPECT_FALSE(sensorData[it.first].value().has_value());
        EXPECT_FALSE(sensorData[it.first].timeStamp().has_value());
      }
    }
  }
};

TEST_F(SensorServiceImplTest, fetchAndCheckSensorDataSuccess) {
  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  auto sensorServiceImpl =
      createSensorServiceImplForTest(tmpDir.path().string());

  // Test that sensor reads work as expected
  testFetchAndCheckSensorData(sensorServiceImpl, true);
}

TEST_F(SensorServiceImplTest, fetchAndCheckSensorDataFailure) {
  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  auto sensorServiceImpl =
      createSensorServiceImplForTest(tmpDir.path().string());

  // Remove sensor sysfs files
  std::string sensorConfJson;
  ASSERT_TRUE(folly::readFile(FLAGS_config_file.c_str(), sensorConfJson));
  SensorConfig sensorConfig;
  apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
      sensorConfJson, sensorConfig);
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    for (const auto& pmSensors : *pmUnitSensors.sensors()) {
      ASSERT_TRUE(std::filesystem::remove(*pmSensors.sysfsPath()));
    }
  }

  // Check that sensor value/timestamp are now empty which implies read failed
  testFetchAndCheckSensorData(sensorServiceImpl, false);
}

TEST_F(SensorServiceImplTest, fetchAndCheckSensorDataSuccesAndThenFailure) {
  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  auto sensorServiceImpl =
      createSensorServiceImplForTest(tmpDir.path().string());

  // CASE 1: Success
  testFetchAndCheckSensorData(sensorServiceImpl, true);

  // CASE 2: Failure (with sysfs files disappearing)
  // Remove sensors' sensor files.
  std::string sensorConfJson;
  ASSERT_TRUE(folly::readFile(FLAGS_config_file.c_str(), sensorConfJson));
  SensorConfig sensorConfig;
  apache::thrift::SimpleJSONSerializer::deserialize<SensorConfig>(
      sensorConfJson, sensorConfig);
  for (const auto& pmUnitSensors : *sensorConfig.pmUnitSensorsList()) {
    for (const auto& pmSensors : *pmUnitSensors.sensors()) {
      ASSERT_TRUE(std::filesystem::remove(*pmSensors.sysfsPath()));
    }
  }
  // Check that sensor value/timestamp are now empty which implies read failed
  testFetchAndCheckSensorData(sensorServiceImpl, false);
}

TEST_F(SensorServiceImplTest, getSomeSensors) {
  auto mockSensorData = getDefaultMockSensorData();
  auto mockSensorName = *std::views::keys(mockSensorData).begin();

  auto now = Utils::nowInSecs();
  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
  auto sensorServiceImpl =
      createSensorServiceImplForTest(tmpDir.path().string());
  sensorServiceImpl->fetchSensorData();
  auto sensorData =
      sensorServiceImpl->getSensorsData({mockSensorName, "BOGUS_SENSOR"});
  EXPECT_EQ(sensorData.size(), 1);
  EXPECT_EQ(*sensorData[0].name(), mockSensorName);
  EXPECT_FLOAT_EQ(*sensorData[0].value(), mockSensorData[mockSensorName]);
  EXPECT_GE(*sensorData[0].timeStamp(), now);
}

} // namespace facebook::fboss
