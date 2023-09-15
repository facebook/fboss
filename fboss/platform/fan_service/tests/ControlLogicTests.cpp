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

namespace {

float kDefaultRpm = 31;

// ControlLogicTests is added after fan service was deployed in production.
// So kExpectedPwms is production outputs of the given test inputs.
std::array<int, 5> kExpectedPwms = std::array<int, 5>{51, 51, 52, 52, 53};

}; // namespace

namespace facebook::fboss::platform {
class MockBsp : public Bsp {
 public:
  explicit MockBsp(const FanServiceConfig& config) : Bsp(config) {}
  MOCK_METHOD(bool, setFanPwmSysfs, (std::string, int));
  MOCK_METHOD(bool, setFanPwmShell, (std::string, std::string, int));
  MOCK_METHOD(bool, setFanLedSysfs, (std::string, int));
  MOCK_METHOD(bool, setFanLedShell, (std::string, std::string, int));

  MOCK_METHOD(bool, checkIfInitialSensorDataRead, (), (const));
  MOCK_METHOD(float, readSysfs, (std::string), (const));
};

class ControlLogicTests : public testing::Test {
 public:
  void SetUp() override {
    std::string confName =
        "fboss/platform/fan_service/tests/configs/MockFanServiceConfig.json";
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
      sensorData_->updateEntryFloat(
          *sensor.sensorName(), value, mockBsp_->getCurrentTime());
      value += 10.5;
    }

    for (const auto& optic : *fanServiceConfig_.optics()) {
      value = 30.0;
      auto opticData = std::vector<std::pair<std::string, float>>();
      for (const auto& [opticType, tempToPwm] : *optic.tempToPwmMaps()) {
        opticData.push_back(std::make_pair(opticType, value));
        value += 12.0;
      }
      sensorData_->setOpticEntry(
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
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmAccess()->path(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledAccess()->path(), _))
        .Times(0);
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
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmAccess()->path(), _))
        .WillOnce(Return(false));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledAccess()->path(), _))
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
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmAccess()->path(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledAccess()->path(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmAccess()->path()))
        .WillOnce(Return(kDefaultRpm));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceAccess()->path()))
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
  }
}

TEST_F(ControlLogicTests, UpdateControlFailureDueToMissingFans) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead()).WillOnce(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmAccess()->path(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledAccess()->path(), _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmAccess()->path())).Times(0);
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceAccess()->path()))
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
    EXPECT_EQ(*fanStatus.pwmToProgram(), kExpectedPwms[i++]);
  }
}

TEST_F(ControlLogicTests, UpdateControlFailureAfterFanUnaccessibleFirstTime) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead()).WillOnce(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmAccess()->path(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledAccess()->path(), _))
        .Times(1)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmAccess()->path()))
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceAccess()->path()))
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
    EXPECT_EQ(*fanStatus.pwmToProgram(), kExpectedPwms[i++]);
  }
}

TEST_F(ControlLogicTests, UpdateControlSuccessAfterFanUnaccessibleLTThreshold) {
  EXPECT_CALL(*mockBsp_, checkIfInitialSensorDataRead())
      .Times(2)
      .WillRepeatedly(Return(true));
  for (const auto& fan : *fanServiceConfig_.fans()) {
    EXPECT_CALL(*mockBsp_, setFanPwmSysfs(*fan.pwmAccess()->path(), _))
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, setFanLedSysfs(*fan.ledAccess()->path(), _))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.rpmAccess()->path()))
        .WillOnce(Return(kDefaultRpm))
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(*mockBsp_, readSysfs(*fan.presenceAccess()->path()))
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
  }
}
} // namespace facebook::fboss::platform
