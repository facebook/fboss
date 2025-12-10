// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <ranges>

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/tests/TestUtils.h"

namespace facebook::fboss::platform::sensor_service {

class SensorServiceThriftHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    folly::test::TemporaryDirectory tmpDir;
    auto config = getMockSensorConfig(tmpDir.path().string());
    auto impl = std::make_shared<SensorServiceImpl>(config);
    impl->fetchSensorData();
    sensorServiceHandler_ =
        std::make_unique<SensorServiceThriftHandler>(std::move(impl));
  }
  std::unique_ptr<SensorServiceThriftHandler> sensorServiceHandler_;
  std::map<std::string, float> sensorMockData_{getMockSensorData()};
};

TEST_F(
    SensorServiceThriftHandlerTest,
    getSensorValuesByNameWithEmptySensorName) {
  SensorReadResponse response;
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>());
  EXPECT_EQ(response.sensorData()->size(), sensorMockData_.size());
  for (auto& sensorDatum : *response.sensorData()) {
    EXPECT_EQ(sensorMockData_.at(*sensorDatum.name()), *sensorDatum.value());
    EXPECT_EQ(*sensorDatum.slotPath(), "/");
    if (*sensorDatum.name() == "MOCK_FRU_SENSOR1") {
      EXPECT_TRUE(
          sensorDatum.sysfsPath()->ends_with("mock_fru_sensor_1_path:temp1"));
    } else if (*sensorDatum.name() == "MOCK_FRU_SENSOR2") {
      EXPECT_TRUE(
          sensorDatum.sysfsPath()->ends_with("mock_fru_sensor_2_path:fan1"));
    } else if (*sensorDatum.name() == "MOCK_FRU_SENSOR3") {
      EXPECT_TRUE(
          sensorDatum.sysfsPath()->ends_with("mock_fru_sensor_3_path:vin"));
    }
  }
}

TEST_F(
    SensorServiceThriftHandlerTest,
    getSensorValuesByNameWithNonEmptySensorName) {
  auto mockSensorData = getMockSensorData();
  auto mockSensorNamesIt = std::views::keys(mockSensorData).begin();

  auto sensorNames = std::vector<std::string>{*mockSensorNamesIt};
  SensorReadResponse response;
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>(sensorNames));
  EXPECT_EQ(response.sensorData()->size(), sensorNames.size());
  EXPECT_EQ(
      sensorMockData_.at(*mockSensorNamesIt),
      *response.sensorData()[0].value());

  sensorNames.push_back(*(++mockSensorNamesIt));
  sensorServiceHandler_->getSensorValuesByNames(
      response, std::make_unique<std::vector<std::string>>(sensorNames));
  EXPECT_EQ(response.sensorData()->size(), sensorNames.size());
  for (auto& sensorDatum : *response.sensorData()) {
    EXPECT_EQ(sensorMockData_.at(*sensorDatum.name()), *sensorDatum.value());
  }
}

}; // namespace facebook::fboss::platform::sensor_service
