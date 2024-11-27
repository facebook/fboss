// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_manager/PlatformExplorer.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
void expectVersions(const char* deviceName, const char* versionString) {
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(fmt::format(
          PlatformExplorer::kGroupedFirmwareVersion,
          deviceName,
          versionString)),
      1);
}
} // namespace

namespace facebook::fboss::platform::platform_manager {

TEST(PlatformExplorerTest, PublishFirmwareVersions) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  std::string fpgaFwVerPath = "/run/devmap/fpgas/TEST_FPGA_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2", fmt::format("{}/fw_ver", fpgaFwVerPath)));
  std::string cpldFwVerPath = "/run/devmap/cplds/TEST_CPLD_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789", fmt::format("{}/fw_ver", cpldFwVerPath)));
  std::string cpldBadFwVerPath = "/run/devmap/cplds/TEST_CPLD_BADFWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789 // comment", fmt::format("{}/fw_ver", cpldBadFwVerPath)));
  // Case with fw_ver under hwmon
  std::string cpldHwmonFwVerPath = "/run/devmap/cplds/FAN0_CPLD_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2.3", fmt::format("{}/hwmon/hwmon20/fw_ver", cpldHwmonFwVerPath)));
  // Case with fw_ver NOT under hwmon, even when hwmon directory exists
  std::string cpldHwmonTrapPath = "/run/devmap/cplds/TAHAN_SMB_CPLD_TRAP";
  EXPECT_TRUE(platformFsUtils->createDirectories(
      fmt::format("{}/hwmon/hwmon20", cpldHwmonTrapPath)));
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "7.8.9", fmt::format("{}/fw_ver", cpldHwmonTrapPath)));

  // Non-existent versions
  std::string fpgaNonePath = "/run/devmap/fpgas/NONE";
  EXPECT_TRUE(platformFsUtils->createDirectories(fpgaNonePath));

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldBadFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonTrapPath] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaNonePath] = "";

  PlatformExplorer explorer(platformConfig, platformFsUtils);
  explorer.publishFirmwareVersions();

  expectVersions("TEST_FPGA_FWVER", "1.2");
  expectVersions("TEST_CPLD_FWVER", "123.456.789");
  expectVersions("TEST_CPLD_BADFWVER", "ERROR_INVALID_STRING");
  expectVersions("FAN0_CPLD_FWVER", "1.2.3");
  expectVersions("TAHAN_SMB_CPLD_TRAP", "7.8.9");
  expectVersions("NONE", PlatformExplorer::kFwVerErrorFileNotFound);
}

} // namespace facebook::fboss::platform::platform_manager
