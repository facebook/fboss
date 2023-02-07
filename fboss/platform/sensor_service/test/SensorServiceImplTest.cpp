// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/experimental/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/test/TestUtils.h"

using namespace facebook::fboss::platform::sensor_service;

namespace facebook::fboss {

class SensorServiceImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    impl_ = createSensorServiceImplForTest(tmpDir.path().string());
    FLAGS_mock_lmsensor_json_data =
        createMockSensorDataFile(tmpDir.path().string());
  }

  std::unique_ptr<SensorServiceImpl> impl_{nullptr};
  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();
};

TEST_F(SensorServiceImplTest, fetchAndCheckSensorData) {
  auto now = platform::helpers::nowInSecs();
  impl_->fetchSensorData();
  auto sensorData = impl_->getAllSensorData();
  std::map<std::string, float> expectedSensors = {
      {"MOCK_FRU_1_SENSOR_1", 25},
      {"MOCK_FRU_1_SENSOR_2", 11152},
      {"MOCK_FRU_2_SENSOR_1", 11.875},
  };
  EXPECT_EQ(sensorData.size(), expectedSensors.size());
  for (const auto& it : expectedSensors) {
    EXPECT_TRUE(sensorData.find(it.first) != sensorData.end());
    EXPECT_EQ(sensorData[it.first].value(), it.second);
    EXPECT_EQ(sensorData[it.first].name(), it.first);
    EXPECT_GE(sensorData[it.first].timeStamp(), now);
  }
}

} // namespace facebook::fboss
