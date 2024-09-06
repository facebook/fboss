// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/ControlLogic.h"
#include "fboss/platform/fan_service/SensorData.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_constants.h"

using namespace ::testing;
using namespace facebook::fboss::platform::fan_service;
namespace constants =
    facebook::fboss::platform::fan_service::fan_service_config_constants;

namespace {

float kDefaultRpm = 31;

std::array<int, 5> kExpectedPwms = std::array<int, 5>{49, 49, 48, 48, 47};
std::array<int, 5> kExpectedBoostModePwms =
    std::array<int, 5>{51, 51, 52, 52, 53};

}; // namespace

namespace facebook::fboss::platform {
class MockBsp : public Bsp {
 public:
  explicit MockBsp(const FanServiceConfig& config) : Bsp(config) {}
  MOCK_METHOD(bool, setFanPwmSysfs, (const std::string&, int));
  MOCK_METHOD(bool, setFanLedSysfs, (const std::string&, int));
  MOCK_METHOD(bool, checkIfInitialSensorDataRead, (), (const));
  MOCK_METHOD(float, readSysfs, (const std::string&), (const));
};

class ControlLogicTests : public testing::Test {
 public:
  void SetUp() override {
    std::string confName = "fboss/platform/configs/sample/fan_service.json";
    std::string fanServiceConfJson;
    if (!folly::readFile(confName.c_str(), fanServiceConfJson)) {
      throw std::runtime_error("Fail to read config file: " + confName);
    }

    apache::thrift::SimpleJSONSerializer::deserialize<FanServiceConfig>(
        fanServiceConfJson, fanServiceConfig_);
    mockBsp_ = std::make_shared<MockBsp>(fanServiceConfig_);
    controlLogic_ = std::make_shared<ControlLogic>(fanServiceConfig_, mockBsp_);
    sensorData_ = std::make_shared<SensorData>();

    float value = 30.0;
    for (const auto& sensor : *fanServiceConfig_.sensors()) {
      sensorData_->updateSensorEntry(
          *sensor.sensorName(), value, mockBsp_->getCurrentTime());
      value += 10.5;
    }

    for (const auto& optic : *fanServiceConfig_.optics()) {
      value = 30.0;
      auto opticData = std::vector<std::pair<std::string, float>>();
      for (const auto& [opticType, tempToPwm] : *optic.tempToPwmMaps()) {
        opticData.emplace_back(opticType, value);
        value += 12.0;
      }
      sensorData_->updateOpticEntry(
          *optic.opticName(), std::move(opticData), mockBsp_->getCurrentTime());
    }

    ASSERT_TRUE(kExpectedPwms.size() == fanServiceConfig_.fans()->size());
  }

  FanServiceConfig fanServiceConfig_;
  std::shared_ptr<SensorData> sensorData_;
  std::shared_ptr<MockBsp> mockBsp_;
  std::shared_ptr<ControlLogic> controlLogic_;
};

TEST_F(ControlLogicTests, SetTransitionValueSuccess) {
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _)).Times(0);
  }

  controlLogic_->setTransitionValue();

  for (const auto& [fanName, fanStatus] : controlLogic_->getFanStatuses()) {
    EXPECT_EQ(*fanStatus.fanFailed(), false);
    EXPECT_EQ(
        *fanStatus.pwmToProgram(), *fanServiceConfig_.pwmTransitionValue());
  }
}

TEST_F(ControlLogicTests, SetTransitionValueFailure) {
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .WillOnce(Return(false));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _))
        .Times(1)
        .WillOnce(Return(true));
  }

  controlLogic_->setTransitionValue();

  for (const auto& [fanName, fanStatus] : controlLogic_->getFanStatuses()) {
    EXPECT_EQ(*fanStatus.fanFailed(), true);
    EXPECT_EQ(
        *fanStatus.pwmToProgram(), *fanServiceConfig_.pwmTransitionValue());
  }
}

TEST_F(ControlLogicTests, UpdateControlSuccess) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead()).WillOnce(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmSysfsPath()))
        .WillOnce(Return(kDefaultRpm));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceSysfsPath()))
        .WillOnce(Return(1 /* fan exists */));
  }

  auto startTime = mockBsp_->getCurrentTime();

  controlLogic_->setTransitionValue();

  controlLogic_->updateControl(sensorData_);
  const auto fanStatuses = controlLogic_->getFanStatuses();
  EXPECT_EQ(fanStatuses.size(), fanServiceConfig_.fans()->size());
  int i = 0;
  for (const auto& [fanName, fanStatus] : fanStatuses) {
    EXPECT_EQ(*fanStatus.fanFailed(), false);
    EXPECT_EQ(*fanStatus.rpm(), kDefaultRpm);
    EXPECT_GE(*fanStatus.lastSuccessfulAccessTime(), startTime);
    EXPECT_EQ(*fanStatus.pwmToProgram(), kExpectedPwms[i++]);
    EXPECT_EQ(fb303::fbData->getCounter(fmt::format("{}.absent", fanName)), 0);
    EXPECT_EQ(
        fb303::fbData->getCounter(fmt::format("{}.rpm_read.failure", fanName)),
        0);
  }
  const auto& sensorCaches = controlLogic_->getSensorCaches();
  for (const auto& [sensorName, sensorCache] : sensorCaches) {
    EXPECT_EQ(sensorCache.sensorFailed, false);
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format("{}.sensor_read.failure", sensorName)),
        0);
  }
}

TEST_F(ControlLogicTests, UpdateControlFailureDueToMissingFans) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead()).WillOnce(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmSysfsPath())).Times(0);
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceSysfsPath()))
        .WillOnce(Return(0 /* fan missing */));
  }

  controlLogic_->setTransitionValue();

  controlLogic_->updateControl(sensorData_);
  const auto fanStatuses = controlLogic_->getFanStatuses();
  int i = 0;
  EXPECT_EQ(fanStatuses.size(), fanServiceConfig_.fans()->size());
  for (const auto& [fanName, fanStatus] : fanStatuses) {
    EXPECT_EQ(*fanStatus.fanFailed(), true);
    EXPECT_EQ(fanStatus.rpm().has_value(), false);
    EXPECT_EQ(*fanStatus.lastSuccessfulAccessTime(), 0);
    EXPECT_EQ(*fanStatus.pwmToProgram(), kExpectedBoostModePwms[i++]);
    EXPECT_EQ(fb303::fbData->getCounter(fmt::format("{}.absent", fanName)), 1);
    EXPECT_EQ(
        fb303::fbData->getCounter(fmt::format("{}.rpm_read.failure", fanName)),
        1);
  }
}

TEST_F(ControlLogicTests, UpdateControlFailureDueToFanInaccessible) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead()).WillOnce(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _))
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmSysfsPath()))
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceSysfsPath()))
        .WillOnce(Return(1 /* fan exists */));
  }

  controlLogic_->setTransitionValue();

  controlLogic_->updateControl(sensorData_);
  const auto fanStatuses = controlLogic_->getFanStatuses();
  EXPECT_EQ(fanStatuses.size(), fanServiceConfig_.fans()->size());
  int i = 0;
  for (const auto& [fanName, fanStatus] : fanStatuses) {
    EXPECT_EQ(*fanStatus.fanFailed(), true);
    EXPECT_EQ(fanStatus.rpm().has_value(), false);
    EXPECT_EQ(*fanStatus.lastSuccessfulAccessTime(), 0);
    EXPECT_EQ(*fanStatus.pwmToProgram(), kExpectedBoostModePwms[i++]);
    EXPECT_EQ(fb303::fbData->getCounter(fmt::format("{}.absent", fanName)), 0);
    EXPECT_EQ(
        fb303::fbData->getCounter(fmt::format("{}.rpm_read.failure", fanName)),
        1);
  }
}

TEST_F(
    ControlLogicTests,
    UpdateControlFailureDueToFanInaccessibleAfterSuccessLTThreshold) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead())
      .Times(2)
      .WillRepeatedly(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmSysfsPath()))
        .WillOnce(Return(kDefaultRpm))
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceSysfsPath()))
        .Times(2)
        .WillRepeatedly(Return(1 /* fan exists */));
  }

  auto startTime = mockBsp_->getCurrentTime();

  controlLogic_->setTransitionValue();

  // Simulate two cycles of programing fan.
  controlLogic_->updateControl(sensorData_);
  controlLogic_->updateControl(sensorData_);
  const auto fanStatuses = controlLogic_->getFanStatuses();
  EXPECT_EQ(fanStatuses.size(), fanServiceConfig_.fans()->size());
  for (const auto& [fanName, fanStatus] : fanStatuses) {
    EXPECT_EQ(*fanStatus.fanFailed(), false);
    EXPECT_EQ(fanStatus.rpm().has_value(), false);
    EXPECT_GE(*fanStatus.lastSuccessfulAccessTime(), startTime);
    EXPECT_EQ(fb303::fbData->getCounter(fmt::format("{}.absent", fanName)), 0);
    EXPECT_EQ(
        fb303::fbData->getCounter(fmt::format("{}.rpm_read.failure", fanName)),
        1);
  }
}
TEST_F(ControlLogicTests, UpdateControlSensorReadFailure) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead()).WillOnce(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmSysfsPath(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledSysfsPath(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmSysfsPath()))
        .WillOnce(Return(kDefaultRpm));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceSysfsPath()))
        .WillOnce(Return(1 /* fan exists */));
  }

  auto startTime = mockBsp_->getCurrentTime();

  controlLogic_->setTransitionValue();

  for (const auto& sensor : *fanServiceConfig_.sensors()) {
    sensorData_->delSensorEntry(*sensor.sensorName());
  }
  controlLogic_->updateControl(sensorData_);
  const auto fanStatuses = controlLogic_->getFanStatuses();
  EXPECT_EQ(fanStatuses.size(), fanServiceConfig_.fans()->size());
  int i = 0;
  for (const auto& [fanName, fanStatus] : fanStatuses) {
    EXPECT_EQ(*fanStatus.fanFailed(), false);
    EXPECT_EQ(*fanStatus.rpm(), kDefaultRpm);
    EXPECT_GE(*fanStatus.lastSuccessfulAccessTime(), startTime);
    EXPECT_EQ(*fanStatus.pwmToProgram(), kExpectedPwms[i++]);
    EXPECT_EQ(fb303::fbData->getCounter(fmt::format("{}.absent", fanName)), 0);
    EXPECT_EQ(
        fb303::fbData->getCounter(fmt::format("{}.rpm_read.failure", fanName)),
        0);
  }
  const auto& sensorCaches = controlLogic_->getSensorCaches();
  for (const auto& [sensorName, sensorCache] : sensorCaches) {
    EXPECT_EQ(sensorCache.sensorFailed, true);
    EXPECT_EQ(
        fb303::fbData->getCounter(
            fmt::format("{}.sensor_read.failure", sensorName)),
        1);
  }
}

} // namespace facebook::fboss::platform
