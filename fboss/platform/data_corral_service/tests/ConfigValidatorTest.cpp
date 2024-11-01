#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/data_corral_service/ConfigValidator.h"

using namespace ::testing;
// using namespace facebook::fboss::platform;

namespace {
constexpr auto kDarwin = "darwin";
constexpr auto kMontblanc = "montblanc";
constexpr auto kMeru800bia = "meru800bia";
constexpr auto kMeru800bfa = "meru800bfa";
constexpr auto kMorgan800cc = "morgan800cc";
constexpr auto kJanga800bic = "janga800bic";
constexpr auto kTahan800bc = "tahan800bc";
} // namespace

namespace facebook::fboss::platform::data_corral_service {

LedConfig getValidLedConfig() {
  auto ledConfig = LedConfig();
  ledConfig.absentLedColor() = LedColor::BLUE;
  ledConfig.presentLedColor() = LedColor::GREEN;
  ledConfig.presentLedSysfsPath() = "/sys/class/leds/1";
  ledConfig.absentLedSysfsPath() = "/sys/class/leds/2";
  return ledConfig;
}

PresenceDetection getValidSysfsPresenceDetection() {
  auto presenceDetection = PresenceDetection();
  presenceDetection.sysfsFileHandle() = SysfsFileHandle();
  presenceDetection.sysfsFileHandle()->presenceFilePath() =
      "/sys/class/present/0";
  return presenceDetection;
}

PresenceDetection getValidGpioPresenceDetection() {
  auto presenceDetection = PresenceDetection();
  presenceDetection.gpioLineHandle() = GpioLineHandle();
  presenceDetection.gpioLineHandle()->charDevPath() = "/dev/gpiochip0";
  presenceDetection.gpioLineHandle()->lineIndex() = 0;
  presenceDetection.gpioLineHandle()->desiredValue() = 1;
  return presenceDetection;
}

FruConfig getValidFruConfig() {
  auto fruConfig = FruConfig();
  fruConfig.fruName() = "fru1";
  fruConfig.fruType() = "type1";
  fruConfig.presenceDetection() = getValidSysfsPresenceDetection();
  return fruConfig;
}

LedManagerConfig getValidLedManagerConfig() {
  auto config = LedManagerConfig();
  config.systemLedConfig() = getValidLedConfig();
  config.fruTypeLedConfigs() = {{"type1", getValidLedConfig()}};
  config.fruConfigs() = {};
  config.fruConfigs()->push_back(getValidFruConfig());
  return config;
}

TEST(ConfigValidatorTest, ValidConfig) {
  auto config = getValidLedManagerConfig();
  EXPECT_TRUE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidLedConfig) {
  auto config = getValidLedManagerConfig();
  config.systemLedConfig()->presentLedSysfsPath() = "";
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config = LedManagerConfig();
  config.systemLedConfig()->absentLedSysfsPath() = "";
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidFruConfig) {
  auto config = getValidLedManagerConfig();
  config.fruConfigs()->at(0).fruName() = "";
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config = getValidLedManagerConfig();
  config.fruConfigs()->at(0).fruType() = "";
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config = getValidLedManagerConfig();
  config.fruConfigs()->at(0).presenceDetection() =
      getValidGpioPresenceDetection();
  config.fruConfigs()
      ->at(0)
      .presenceDetection()
      ->gpioLineHandle()
      ->lineIndex() = -1;
  EXPECT_FALSE(ConfigValidator().isValid(config));

  config = getValidLedManagerConfig();
  config.fruConfigs()->at(0).fruType() = "not_a_fru_type";
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidFruTypeLedConfig) {
  auto config = getValidLedManagerConfig();
  config.fruTypeLedConfigs()->at("type1") = getValidLedConfig();
  config.fruTypeLedConfigs()->at("type1").presentLedSysfsPath() = "";
  EXPECT_FALSE(ConfigValidator().isValid(config));
}

TEST(ConfigValidatorTest, InvalidPresenceDetection) {
  auto config = getValidSysfsPresenceDetection();
  config.sysfsFileHandle()->presenceFilePath() = "";
  EXPECT_FALSE(ConfigValidator().isValidPresenceConfig(config));

  config = getValidGpioPresenceDetection();
  config.gpioLineHandle()->charDevPath() = "";
  EXPECT_FALSE(ConfigValidator().isValidPresenceConfig(config));

  config = getValidGpioPresenceDetection();
  config.gpioLineHandle()->desiredValue() = -1;
  EXPECT_FALSE(ConfigValidator().isValidPresenceConfig(config));

  config = getValidGpioPresenceDetection();
  config.gpioLineHandle()->lineIndex() = -1;
  EXPECT_FALSE(ConfigValidator().isValidPresenceConfig(config));
}

TEST(ConfigValidatorTest, RealConfigsValid) {
  for (const auto& platform :
       {kDarwin,
        kJanga800bic,
        kMeru800bfa,
        kMeru800bia,
        kMontblanc,
        kMorgan800cc,
        kTahan800bc}) {
    XLOG(INFO) << "Validating config for " << platform;
    LedManagerConfig config;
    auto configJson =
        facebook::fboss::platform::ConfigLib().getLedManagerConfig(platform);
    apache::thrift::SimpleJSONSerializer::deserialize<LedManagerConfig>(
        configJson, config);
    EXPECT_TRUE(ConfigValidator().isValid(config));
  }
}
} // namespace facebook::fboss::platform::data_corral_service
