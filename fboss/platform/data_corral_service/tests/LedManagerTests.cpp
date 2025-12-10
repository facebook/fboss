// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/data_corral_service/LedManager.h"

using namespace testing;
using namespace folly::test;

namespace facebook::fboss::platform::data_corral_service {
class LedManagerTests : public Test {
 public:
  void SetUp() override {
    systemLedConfig_.presentLedSysfsPath() = TemporaryFile().path().string();
    systemLedConfig_.absentLedSysfsPath() = TemporaryFile().path().string();

    auto fanLedConfig = LedConfig();
    fanLedConfig.presentLedSysfsPath() = TemporaryFile().path().string();
    fanLedConfig.absentLedSysfsPath() = TemporaryFile().path().string();

    auto psuLedConfig = LedConfig();
    psuLedConfig.presentLedSysfsPath() = TemporaryFile().path().string();
    psuLedConfig.absentLedSysfsPath() = TemporaryFile().path().string();

    fruTypeLedConfigs_ = {{"FAN", fanLedConfig}, {"PSU", psuLedConfig}};

    ledManager_ =
        std::make_shared<LedManager>(systemLedConfig_, fruTypeLedConfigs_);
  }

  LedConfig systemLedConfig_ = LedConfig();
  std::map<std::string, LedConfig> fruTypeLedConfigs_;
  std::shared_ptr<LedManager> ledManager_;
};

TEST_F(LedManagerTests, UnknownFruType) {
  EXPECT_FALSE(ledManager_->programFruLed("UNKNOWN_TYPE", true));
}

TEST_F(LedManagerTests, FruLedPresent) {
  EXPECT_TRUE(ledManager_->programFruLed("FAN", true));
  std::string presentVal, absentVal;
  EXPECT_TRUE(
      folly::readFile(
          fruTypeLedConfigs_["FAN"].presentLedSysfsPath()->c_str(),
          presentVal));
  EXPECT_EQ(presentVal, "1");
  folly::readFile(
      fruTypeLedConfigs_["FAN"].absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "0");
}

TEST_F(LedManagerTests, FruLedAbsent) {
  EXPECT_TRUE(ledManager_->programFruLed("FAN", false));
  std::string presentVal, absentVal;
  folly::readFile(
      fruTypeLedConfigs_["FAN"].presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "0");
  folly::readFile(
      fruTypeLedConfigs_["FAN"].absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "1");
}

TEST_F(LedManagerTests, FruLedAbsentToPresent) {
  EXPECT_TRUE(ledManager_->programFruLed("PSU", false));
  EXPECT_TRUE(ledManager_->programFruLed("PSU", true));
  std::string presentVal, absentVal;
  folly::readFile(
      fruTypeLedConfigs_["PSU"].presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "1");
  folly::readFile(
      fruTypeLedConfigs_["PSU"].absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "0");
}

TEST_F(LedManagerTests, FruLedPresentToAbsent) {
  EXPECT_TRUE(ledManager_->programFruLed("PSU", true));
  EXPECT_TRUE(ledManager_->programFruLed("PSU", false));
  std::string presentVal, absentVal;
  folly::readFile(
      fruTypeLedConfigs_["PSU"].presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "0");
  folly::readFile(
      fruTypeLedConfigs_["PSU"].absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "1");
}

TEST_F(LedManagerTests, SystemLedPresent) {
  EXPECT_TRUE(ledManager_->programSystemLed(true));
  std::string presentVal, absentVal;
  folly::readFile(systemLedConfig_.presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "1");
  folly::readFile(systemLedConfig_.absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "0");
}

TEST_F(LedManagerTests, SystemLedAbsent) {
  EXPECT_TRUE(ledManager_->programSystemLed(false));
  std::string presentVal, absentVal;
  folly::readFile(systemLedConfig_.presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "0");
  folly::readFile(systemLedConfig_.absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "1");
}

TEST_F(LedManagerTests, SystemLedAbsentToPresent) {
  EXPECT_TRUE(ledManager_->programSystemLed(false));
  EXPECT_TRUE(ledManager_->programSystemLed(true));
  std::string presentVal, absentVal;
  folly::readFile(systemLedConfig_.presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "1");
  folly::readFile(systemLedConfig_.absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "0");
}

TEST_F(LedManagerTests, SystemLedPresentToAbsent) {
  EXPECT_TRUE(ledManager_->programSystemLed(true));
  EXPECT_TRUE(ledManager_->programSystemLed(false));
  std::string presentVal, absentVal;
  folly::readFile(systemLedConfig_.presentLedSysfsPath()->c_str(), presentVal);
  EXPECT_EQ(presentVal, "0");
  folly::readFile(systemLedConfig_.absentLedSysfsPath()->c_str(), absentVal);
  EXPECT_EQ(absentVal, "1");
}

TEST_F(LedManagerTests, ReadSysfsFail) {
  systemLedConfig_.presentLedSysfsPath() = "";
  ledManager_ =
      std::make_shared<LedManager>(systemLedConfig_, fruTypeLedConfigs_);
  EXPECT_FALSE(ledManager_->programSystemLed(true));
}
} // namespace facebook::fboss::platform::data_corral_service
