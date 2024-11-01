// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <ranges>

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/SensorServiceThriftHandler.h"
#include "fboss/platform/sensor_service/test/TestUtils.h"

namespace facebook::fboss::platform::sensor_service {

class SensorServiceThriftHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    auto tmpDir = folly::test::TemporaryDirectory();
    auto sensorServiceImpl =
        createSensorServiceImplForTest(tmpDir.path().string());
    sensorServiceImpl->fetchSensorData();
    sensorServiceHandler_ = std::make_unique<SensorServiceThriftHandler>(
        std::move(sensorServiceImpl));
  }
  std::unique_ptr<SensorServiceThriftHandler> sensorServiceHandler_;
  std::map<std::string, float> sensorMockData_{getDefaultMockSensorData()};
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
  }
}

TEST_F(
    SensorServiceThriftHandlerTest,
    getSensorValuesByNameWithNonEmptySensorName) {
  auto mockSensorData = getDefaultMockSensorData();
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
