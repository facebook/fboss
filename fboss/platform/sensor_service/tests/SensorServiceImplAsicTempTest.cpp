// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <memory>

#include <fb303/ServiceData.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/MockPlatformUtils.h"
#include "fboss/platform/sensor_service/SensorServiceImpl.h"
#include "fboss/platform/sensor_service/Utils.h"

using namespace facebook::fboss::platform::sensor_service;
using namespace testing;

namespace facebook::fboss::platform::sensor_service {

class SensorServiceImplAsicTempTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mockPlatformUtils_ = std::make_shared<MockPlatformUtils>();
    impl_ = std::make_unique<SensorServiceImpl>(
        config_, std::make_shared<Utils>(), mockPlatformUtils_);
  }

  SensorConfig config_;
  std::shared_ptr<MockPlatformUtils> mockPlatformUtils_;
  std::unique_ptr<SensorServiceImpl> impl_;
};

// Test successful ASIC command execution
TEST_F(SensorServiceImplAsicTempTest, SuccessfulReading) {
  EXPECT_CALL(*mockPlatformUtils_, execCommand("echo 42"))
      .WillOnce(Return(std::make_pair(0, "42")));

  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_MGET_TEMP";
  asicCommand.cmd() = "echo 42";
  asicCommand.sensorType() = SensorType::TEMPERTURE;

  auto sensorData = impl_->processAsicCmd(asicCommand);

  EXPECT_EQ(*sensorData.name(), "ASIC_TEMP_MGET_TEMP");
  EXPECT_EQ(*sensorData.sensorType(), SensorType::TEMPERTURE);
  ASSERT_TRUE(sensorData.value().has_value());
  EXPECT_EQ(*sensorData.value(), 42);
}

// Test when command fails
TEST_F(SensorServiceImplAsicTempTest, CommandFailure) {
  EXPECT_CALL(*mockPlatformUtils_, execCommand("failing_command"))
      .WillOnce(Return(std::make_pair(1, "Error reading temperature")));

  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_MGET_TEMP";
  asicCommand.cmd() = "failing_command";
  asicCommand.sensorType() = SensorType::TEMPERTURE;

  auto sensorData = impl_->processAsicCmd(asicCommand);

  EXPECT_EQ(*sensorData.name(), "ASIC_TEMP_MGET_TEMP");
  EXPECT_EQ(*sensorData.sensorType(), SensorType::TEMPERTURE);
  EXPECT_FALSE(sensorData.value().has_value());
}

// Test with missing command
TEST_F(SensorServiceImplAsicTempTest, MissingCommand) {
  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_MGET_TEMP";
  asicCommand.sensorType() = SensorType::TEMPERTURE;
  // Don't set cmd() - leave it unset

  auto sensorData = impl_->processAsicCmd(asicCommand);

  EXPECT_EQ(*sensorData.name(), "ASIC_TEMP_MGET_TEMP");
  EXPECT_EQ(*sensorData.sensorType(), SensorType::TEMPERTURE);
  EXPECT_FALSE(sensorData.value().has_value());
}

// Test when command returns non-numeric value causing parse exception
TEST_F(SensorServiceImplAsicTempTest, NonNumericOutput) {
  EXPECT_CALL(*mockPlatformUtils_, execCommand("echo invalid_data"))
      .WillOnce(Return(std::make_pair(0, "invalid_data")));

  AsicCommand asicCommand;
  asicCommand.sensorName() = "ASIC_TEMP_MGET_TEMP";
  asicCommand.cmd() = "echo invalid_data";
  asicCommand.sensorType() = SensorType::TEMPERTURE;

  auto sensorData = impl_->processAsicCmd(asicCommand);

  EXPECT_EQ(*sensorData.name(), "ASIC_TEMP_MGET_TEMP");
  EXPECT_EQ(*sensorData.sensorType(), SensorType::TEMPERTURE);
  // Should not have value because parsing failed
  EXPECT_FALSE(sensorData.value().has_value());
}

} // namespace facebook::fboss::platform::sensor_service
