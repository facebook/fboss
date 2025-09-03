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

// Simple mock for Utils
class MockUtils : public Utils {
 public:
  MOCK_METHOD2(
      getPciAddress,
      std::optional<std::string>(
          const std::string& vendorId,
          const std::string& deviceId));
};

class SensorServiceImplAsicTempTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mockUtils_ = std::make_shared<MockUtils>();
    mockPlatformUtils_ = std::make_shared<MockPlatformUtils>();
    config_.switchAsicTemp() = SwitchAsicTemp();
    config_.switchAsicTemp()->vendorId() = "0x1234";
    config_.switchAsicTemp()->deviceId() = "0x5678";
    impl_ = std::make_unique<SensorServiceImpl>(
        config_, mockUtils_, mockPlatformUtils_);
  }

  SensorConfig config_;
  std::shared_ptr<MockUtils> mockUtils_;
  std::shared_ptr<MockPlatformUtils> mockPlatformUtils_;
  std::unique_ptr<SensorServiceImpl> impl_;
};

// Test successful ASIC temperature reading
TEST_F(SensorServiceImplAsicTempTest, SuccessfulReading) {
  ON_CALL(*mockUtils_, getPciAddress("0x1234", "0x5678"))
      .WillByDefault(Return("0000:01:00.0"));
  ON_CALL(
      *mockPlatformUtils_,
      runCommand(ElementsAre("/usr/bin/mget_temp", "-d", "0000:01:00.0")))
      .WillByDefault(Return(std::make_pair(0, "42\n")));

  impl_->fetchSensorData();
  auto sensorData = impl_->getAllSensorData();

  ASSERT_TRUE(
      sensorData.find(SensorServiceImpl::kAsicTemp) != sensorData.end());
  auto& asicTemp = sensorData[SensorServiceImpl::kAsicTemp];
  EXPECT_EQ(*asicTemp.name(), SensorServiceImpl::kAsicTemp);
  EXPECT_EQ(*asicTemp.sensorType(), SensorType::TEMPERTURE);
  ASSERT_TRUE(asicTemp.value().has_value());
  EXPECT_EQ(*asicTemp.value(), 42);
  EXPECT_EQ(*asicTemp.sysfsPath(), "/usr/bin/mget_temp -d 0000:01:00.0");
  EXPECT_EQ(*asicTemp.slotPath(), "");

  EXPECT_EQ(
      fb303::fbData->getCounter(fmt::format(
          SensorServiceImpl::kReadValue, SensorServiceImpl::kAsicTemp)),
      42);
  EXPECT_EQ(
      fb303::fbData->getCounter(fmt::format(
          SensorServiceImpl::kReadFailure, SensorServiceImpl::kAsicTemp)),
      0);
}

// Test when PCI device is not found
TEST_F(SensorServiceImplAsicTempTest, DeviceNotFound) {
  ON_CALL(*mockUtils_, getPciAddress("0x1234", "0x5678"))
      .WillByDefault(Return(std::nullopt));

  impl_->fetchSensorData();
  auto sensorData = impl_->getAllSensorData();

  ASSERT_TRUE(
      sensorData.find(SensorServiceImpl::kAsicTemp) != sensorData.end());
  auto& asicTemp = sensorData[SensorServiceImpl::kAsicTemp];
  EXPECT_EQ(*asicTemp.name(), SensorServiceImpl::kAsicTemp);
  EXPECT_EQ(*asicTemp.sensorType(), SensorType::TEMPERTURE);
  EXPECT_FALSE(asicTemp.value().has_value());

  EXPECT_EQ(
      fb303::fbData->getCounter(fmt::format(
          SensorServiceImpl::kReadFailure, SensorServiceImpl::kAsicTemp)),
      1);
}

// Test when mget_temp command fails
TEST_F(SensorServiceImplAsicTempTest, CommandFailure) {
  EXPECT_CALL(*mockUtils_, getPciAddress("0x1234", "0x5678"))
      .WillOnce(Return("0000:01:00.0"));
  EXPECT_CALL(
      *mockPlatformUtils_,
      runCommand(ElementsAre("/usr/bin/mget_temp", "-d", "0000:01:00.0")))
      .WillOnce(Return(std::make_pair(1, "Error reading temperature")));

  impl_->fetchSensorData();
  auto sensorData = impl_->getAllSensorData();

  ASSERT_TRUE(
      sensorData.find(SensorServiceImpl::kAsicTemp) != sensorData.end());
  auto& asicTemp = sensorData[SensorServiceImpl::kAsicTemp];
  EXPECT_EQ(*asicTemp.name(), SensorServiceImpl::kAsicTemp);
  EXPECT_EQ(*asicTemp.sensorType(), SensorType::TEMPERTURE);
  EXPECT_FALSE(asicTemp.value().has_value());

  EXPECT_EQ(
      fb303::fbData->getCounter(fmt::format(
          SensorServiceImpl::kReadFailure, SensorServiceImpl::kAsicTemp)),
      1);
}

// Test with missing vendor ID
TEST_F(SensorServiceImplAsicTempTest, MissingVendorId) {
  config_.switchAsicTemp()->vendorId().reset();
  impl_ = std::make_unique<SensorServiceImpl>(
      config_, mockUtils_, mockPlatformUtils_);

  impl_->fetchSensorData();

  auto sensorData = impl_->getAllSensorData();
  ASSERT_TRUE(
      sensorData.find(SensorServiceImpl::kAsicTemp) != sensorData.end());

  auto& asicTemp = sensorData[SensorServiceImpl::kAsicTemp];
  EXPECT_EQ(*asicTemp.name(), SensorServiceImpl::kAsicTemp);
  EXPECT_EQ(*asicTemp.sensorType(), SensorType::TEMPERTURE);
  EXPECT_FALSE(asicTemp.value().has_value());

  // Verify failure counter was set
  EXPECT_EQ(
      fb303::fbData->getCounter(fmt::format(
          SensorServiceImpl::kReadFailure, SensorServiceImpl::kAsicTemp)),
      1);
}

} // namespace facebook::fboss::platform::sensor_service
