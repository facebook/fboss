#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/weutil/ConfigValidator.h"

using namespace ::testing;
using namespace apache::thrift;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::weutil;

namespace {
constexpr auto kChassisName = "CHASSIS";

weutil_config::WeutilConfig createValidWeutilConfig() {
  weutil_config::WeutilConfig weutilConfig;
  weutilConfig.chassisEepromName() = kChassisName;
  weutilConfig.fruEepromList()[kChassisName] = {};
  weutilConfig.fruEepromList()[kChassisName].path() =
      "/run/devmap/eeproms/CHASSIS_EEPROM";
  weutilConfig.fruEepromList()["MCB"] = {};
  weutilConfig.fruEepromList()["MCB"].path() = "/run/devmap/eeproms/MCB_EEPROM";
  return weutilConfig;
}

bool isWeutilConfigValid(
    const weutil_config::WeutilConfig& weutilConfig,
    const std::string& platformName = "") {
  return ConfigValidator().isValid(weutilConfig, platformName);
}

} // namespace

TEST(ConfigValidatorTest, validConfig) {
  auto config = createValidWeutilConfig();
  EXPECT_TRUE(isWeutilConfigValid(config));
}

TEST(ConfigValidatorTest, darwinValidConfigEmptyChassisPath) {
  auto config = createValidWeutilConfig();
  config.fruEepromList()[kChassisName].path() = "";
  EXPECT_TRUE(isWeutilConfigValid(config, "darwin"));
}

TEST(ConfigValidatorTest, darwinInvalidConfigEmptyEepromPath) {
  auto config = createValidWeutilConfig();
  config.fruEepromList()["MCB"].path() = "";
  EXPECT_FALSE(isWeutilConfigValid(config, "darwin"));
}

TEST(ConfigValidatorTest, EmptyChassisPath) {
  auto config = createValidWeutilConfig();
  config.fruEepromList()[kChassisName].path() = "";
  EXPECT_FALSE(isWeutilConfigValid(config));
}

TEST(ConfigValidatorTest, EmptyEepromName) {
  auto config = createValidWeutilConfig();
  config.fruEepromList()[""].path() = "/run/devmap/eeproms/MCB_EEPROM";
  EXPECT_FALSE(isWeutilConfigValid(config));
}

TEST(ConfigValidatorTest, InvalidEepromPathPrefix) {
  auto config = createValidWeutilConfig();
  config.fruEepromList()[kChassisName].path() = "/invalid/prefix/CHASSIS";
  EXPECT_FALSE(isWeutilConfigValid(config));
}
