// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/utilities/PowerConfigUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss::platform::sensor_service {
namespace sensor_config = ::facebook::fboss::platform::sensor_config;

namespace {

sensor_config::PerSlotPowerConfig makeSlot(const std::string& name) {
  sensor_config::PerSlotPowerConfig slot;
  slot.name() = name;
  return slot;
}

} // namespace

TEST(PowerConfigUtilsTest, IsPsuOrPemAcceptsPsuAndPemWithDigits) {
  EXPECT_TRUE(isPsuOrPem(makeSlot("PSU0")));
  EXPECT_TRUE(isPsuOrPem(makeSlot("PSU1")));
  EXPECT_TRUE(isPsuOrPem(makeSlot("PSU12")));
  EXPECT_TRUE(isPsuOrPem(makeSlot("PEM0")));
  EXPECT_TRUE(isPsuOrPem(makeSlot("PEM5")));
}

TEST(PowerConfigUtilsTest, IsPsuOrPemRejectsDecoratedPsuPemNames) {
  // FullMatch — anything past the digits disqualifies. Guards against
  // accidental matches like PSU_BRICK / PSU_PWRBRK / PEM_OUT.
  EXPECT_FALSE(isPsuOrPem(makeSlot("PSU")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("PSU_BRICK")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("PSU_PWRBRK")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("PSU1_BRICK")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("PEM_OUT")));
}

TEST(PowerConfigUtilsTest, IsPsuOrPemRejectsUnrelatedSlots) {
  EXPECT_FALSE(isPsuOrPem(makeSlot("HSC")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("HSC1")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("PWRBRK")));
  EXPECT_FALSE(isPsuOrPem(makeSlot("")));
}

TEST(PowerConfigUtilsTest, HasPsuOrPemEmptyConfig) {
  sensor_config::PowerConfig powerConfig;
  powerConfig.perSlotPowerConfigs() = {};
  EXPECT_FALSE(hasPsuOrPem(powerConfig));
}

TEST(PowerConfigUtilsTest, HasPsuOrPemHscOnly) {
  sensor_config::PowerConfig powerConfig;
  powerConfig.perSlotPowerConfigs() = {makeSlot("HSC1"), makeSlot("PWRBRK")};
  EXPECT_FALSE(hasPsuOrPem(powerConfig));
}

TEST(PowerConfigUtilsTest, HasPsuOrPemTrueWhenPsuPresentAmongHsc) {
  sensor_config::PowerConfig powerConfig;
  powerConfig.perSlotPowerConfigs() = {
      makeSlot("HSC1"), makeSlot("PSU1"), makeSlot("PWRBRK")};
  EXPECT_TRUE(hasPsuOrPem(powerConfig));
}

TEST(PowerConfigUtilsTest, HasPsuOrPemTrueForPemOnly) {
  sensor_config::PowerConfig powerConfig;
  powerConfig.perSlotPowerConfigs() = {makeSlot("PEM0"), makeSlot("PEM1")};
  EXPECT_TRUE(hasPsuOrPem(powerConfig));
}

TEST(PowerConfigUtilsTest, HasPsuOrPemRejectsDecoratedPrefixOnly) {
  // Defensive: if a config only has PSU_BRICK / PSU_PWRBRK style names
  // (which validation should reject elsewhere), hasPsuOrPem must NOT
  // treat them as field-replaceable.
  sensor_config::PowerConfig powerConfig;
  powerConfig.perSlotPowerConfigs() = {
      makeSlot("PSU_BRICK"), makeSlot("PSU_PWRBRK")};
  EXPECT_FALSE(hasPsuOrPem(powerConfig));
}

} // namespace facebook::fboss::platform::sensor_service
