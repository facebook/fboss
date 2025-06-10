// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include "fboss/platform/config_lib/ConfigLib.h"

using namespace ::testing;
using namespace facebook::fboss::platform;

namespace {
const std::string kDarwin = "darwin";
const std::string kMontblanc = "montblanc";
const std::string kMeru800bia = "meru800bia";
const std::string kMeru800bfa = "meru800bfa";
const std::string kMorgan800cc = "morgan800cc";
const std::string kJanga800bic = "janga800bic";
const std::string kTahan800bc = "tahan800bc";
const std::string kGlath05a_64o = "glath05a-64o";
const std::string kSample = "sample";
const std::string kNonExistentPlatform = "nonExistentPlatform";
} // namespace

TEST(ConfigLibTest, Basic) {
  // SensorService Configs
  EXPECT_NO_THROW(ConfigLib().getSensorServiceConfig(kDarwin));
  EXPECT_NO_THROW(ConfigLib().getSensorServiceConfig(kMeru800bia));
  EXPECT_NO_THROW(ConfigLib().getSensorServiceConfig(kMeru800bfa));
  EXPECT_NO_THROW(ConfigLib().getSensorServiceConfig(kMorgan800cc));
  EXPECT_NO_THROW(ConfigLib().getSensorServiceConfig(kJanga800bic));
  EXPECT_NO_THROW(ConfigLib().getSensorServiceConfig(kTahan800bc));
  EXPECT_THROW(
      ConfigLib().getSensorServiceConfig(kNonExistentPlatform),
      std::runtime_error);

  // FanService Configs
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kDarwin));
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kMeru800bia));
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kMeru800bfa));
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kMorgan800cc));
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kJanga800bic));
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kTahan800bc));
  EXPECT_NO_THROW(ConfigLib().getFanServiceConfig(kSample));
  EXPECT_THROW(
      ConfigLib().getFanServiceConfig(kNonExistentPlatform),
      std::runtime_error);

  // PlatformManager Configs
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kSample));
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kMeru800bia));
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kMeru800bfa));
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kMontblanc));
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kMorgan800cc));
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kJanga800bic));
  EXPECT_NO_THROW(ConfigLib().getPlatformManagerConfig(kTahan800bc));
  EXPECT_THROW(
      ConfigLib().getPlatformManagerConfig(kNonExistentPlatform),
      std::runtime_error);

  // weutil Configs
  EXPECT_NO_THROW(ConfigLib().getWeutilConfig(kDarwin));
  EXPECT_NO_THROW(ConfigLib().getWeutilConfig(kMeru800bfa));
  EXPECT_NO_THROW(ConfigLib().getWeutilConfig(kMeru800bia));
  EXPECT_NO_THROW(ConfigLib().getWeutilConfig(kJanga800bic));
  EXPECT_NO_THROW(ConfigLib().getWeutilConfig(kTahan800bc));
  EXPECT_NO_THROW(ConfigLib().getWeutilConfig(kMontblanc));
  EXPECT_THROW(
      ConfigLib().getWeutilConfig(kNonExistentPlatform), std::runtime_error);

  // fwutil Configs
  EXPECT_NO_THROW(ConfigLib().getFwUtilConfig(kDarwin));
  EXPECT_NO_THROW(ConfigLib().getFwUtilConfig(kMeru800bfa));
  EXPECT_NO_THROW(ConfigLib().getFwUtilConfig(kMeru800bia));
  EXPECT_NO_THROW(ConfigLib().getFwUtilConfig(kJanga800bic));
  EXPECT_NO_THROW(ConfigLib().getFwUtilConfig(kTahan800bc));
  EXPECT_THROW(
      ConfigLib().getFwUtilConfig(kNonExistentPlatform), std::runtime_error);

  // LedManager Configs
  EXPECT_NO_THROW(ConfigLib().getLedManagerConfig(kDarwin));
  EXPECT_NO_THROW(ConfigLib().getLedManagerConfig(kMorgan800cc));
  EXPECT_NO_THROW(ConfigLib().getLedManagerConfig(kGlath05a_64o));
  EXPECT_THROW(
      ConfigLib().getLedManagerConfig(kNonExistentPlatform),
      std::runtime_error);

  // BspTests Configs
  EXPECT_NO_THROW(ConfigLib().getBspTestConfig(kMeru800bfa));
  EXPECT_THROW(
      ConfigLib().getBspTestConfig(kNonExistentPlatform), std::out_of_range);
}
