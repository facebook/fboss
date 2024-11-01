// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/testing/TestUtil.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/platform/data_corral_service/FruPresenceExplorer.h"
#include "fboss/platform/data_corral_service/LedManager.h"

using namespace testing;
using namespace folly::test;

namespace facebook::fboss::platform::data_corral_service {

class MockLedManager : public LedManager {
 public:
  MockLedManager(
      const LedConfig& systemLedConfig,
      const std::map<std::string, LedConfig>& fruTypeLedConfigs)
      : LedManager(systemLedConfig, fruTypeLedConfigs) {}
  MOCK_METHOD(bool, programSystemLed, (bool), (const));
  MOCK_METHOD(bool, programFruLed, (const std::string&, bool), (const));
};

class FruPresenceExplorerTests : public Test {
 public:
  void SetUp() override {
    auto fan1 = FruConfig();
    fan1.fruName() = "FAN1";
    fan1.fruType() = "FAN";
    fan1.presenceDetection() = PresenceDetection();
    fan1.presenceDetection()->sysfsFileHandle() = SysfsFileHandle();
    fan1.presenceDetection()->sysfsFileHandle()->presenceFilePath() =
        TemporaryFile().path().string();
    fan1.presenceDetection()->sysfsFileHandle()->desiredValue() = 1;

    auto fan2 = FruConfig();
    fan2.fruName() = "FAN2";
    fan2.fruType() = "FAN";
    fan2.presenceDetection() = PresenceDetection();
    fan2.presenceDetection()->sysfsFileHandle() = SysfsFileHandle();
    fan2.presenceDetection()->sysfsFileHandle()->presenceFilePath() =
        TemporaryFile().path().string();
    fan2.presenceDetection()->sysfsFileHandle()->desiredValue() = 1;

    auto pem1 = FruConfig();
    pem1.fruName() = "PEM1";
    pem1.fruType() = "PEM";
    pem1.presenceDetection() = PresenceDetection();
    pem1.presenceDetection()->sysfsFileHandle() = SysfsFileHandle();
    pem1.presenceDetection()->sysfsFileHandle()->presenceFilePath() =
        TemporaryFile().path().string();
    pem1.presenceDetection()->sysfsFileHandle()->desiredValue() = 1;

    fruConfigs_ = std::vector<FruConfig>{fan1, fan2, pem1};
  }

  std::vector<FruConfig> fruConfigs_;
  std::shared_ptr<FruPresenceExplorer> fruPresenceExplorer_;
  std::shared_ptr<MockLedManager> mockLedManager_ =
      std::make_shared<MockLedManager>(
          LedConfig(),
          std::map<std::string, LedConfig>{});
};

TEST_F(FruPresenceExplorerTests, FrusAllPresent) {
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[0]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[1]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[2]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));

  EXPECT_CALL(*mockLedManager_, programFruLed("FAN", true))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programFruLed("PEM", true))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programSystemLed(true)).WillOnce(Return(true));

  fruPresenceExplorer_ =
      std::make_shared<FruPresenceExplorer>(fruConfigs_, mockLedManager_);
  fruPresenceExplorer_->detectFruPresence();
}

TEST_F(FruPresenceExplorerTests, FrusNotAllPresent) {
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[0]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[1]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[2]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));

  EXPECT_CALL(*mockLedManager_, programFruLed("FAN", false))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programFruLed("PEM", true))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programSystemLed(false)).WillOnce(Return(true));

  fruPresenceExplorer_ =
      std::make_shared<FruPresenceExplorer>(fruConfigs_, mockLedManager_);
  fruPresenceExplorer_->detectFruPresence();
}

TEST_F(FruPresenceExplorerTests, PresenceDetectionFail) {
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[0]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));
  fruConfigs_[1].presenceDetection()->sysfsFileHandle()->presenceFilePath() =
      "INVALID_PATH";
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[2]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "1"));

  EXPECT_CALL(*mockLedManager_, programFruLed("FAN", false))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programFruLed("PEM", true))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programSystemLed(false)).WillOnce(Return(true));

  fruPresenceExplorer_ =
      std::make_shared<FruPresenceExplorer>(fruConfigs_, mockLedManager_);
  fruPresenceExplorer_->detectFruPresence();
}

TEST_F(FruPresenceExplorerTests, PresenceDetectionHexadecimal) {
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[0]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0x1"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[1]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0x01"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[2]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0x001"));

  EXPECT_CALL(*mockLedManager_, programFruLed("FAN", true))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programFruLed("PEM", true))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programSystemLed(true)).WillOnce(Return(true));

  fruPresenceExplorer_ =
      std::make_shared<FruPresenceExplorer>(fruConfigs_, mockLedManager_);
  fruPresenceExplorer_->detectFruPresence();
}

TEST_F(FruPresenceExplorerTests, AbsenceDetectionHexadecimal) {
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[0]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0x0"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[1]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0x00"));
  EXPECT_TRUE(writeSysfs(
      *fruConfigs_[2]
           .presenceDetection()
           ->sysfsFileHandle()
           ->presenceFilePath(),
      "0x000"));

  EXPECT_CALL(*mockLedManager_, programFruLed("FAN", false))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programFruLed("PEM", false))
      .WillOnce(Return(true));
  EXPECT_CALL(*mockLedManager_, programSystemLed(false)).WillOnce(Return(true));

  fruPresenceExplorer_ =
      std::make_shared<FruPresenceExplorer>(fruConfigs_, mockLedManager_);
  fruPresenceExplorer_->detectFruPresence();
}
} // namespace facebook::fboss::platform::data_corral_service
