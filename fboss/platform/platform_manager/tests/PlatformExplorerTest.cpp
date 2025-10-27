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
      facebook::fb303::fbData->getCounter(
          fmt::format(
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

  std::string fpgaFwVerPath = "/run/devmap/inforoms/TEST_FPGA_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2", fmt::format("{}/fw_ver", fpgaFwVerPath)));
  std::string cpldFwVerPath = "/run/devmap/cplds/TEST_CPLD_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789", fmt::format("{}/fw_ver", cpldFwVerPath)));
  std::string cpldBadFwVerPath = "/run/devmap/cplds/TEST_CPLD_BADFWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789 // comment", fmt::format("{}/fw_ver", cpldBadFwVerPath)));
  std::string cpldBadFwVer2Path = "/run/devmap/cplds/TEST_CPLD_BADFWVER2";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.$", fmt::format("{}/fw_ver", cpldBadFwVer2Path)));
  std::string fpgaBadFwVerPath = "/run/devmap/inforoms/TEST_FPGA_BADFWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2.3 a", fmt::format("{}/fw_ver", fpgaBadFwVerPath)));
  std::string fpgaLongFwVerPath = "/run/devmap/inforoms/TEST_FPGA_LONGFWVER";
  std::string bigStr = "0123456789.0123456789.0123456789"; // 32 chars
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      bigStr + "." + bigStr, fmt::format("{}/fw_ver", fpgaLongFwVerPath)));
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
  std::string fpgaNonePath = "/run/devmap/inforoms/NONE";
  EXPECT_TRUE(platformFsUtils->createDirectories(fpgaNonePath));

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldBadFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldBadFwVer2Path] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaBadFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaLongFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonTrapPath] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaNonePath] = "";

  PlatformExplorer explorer(platformConfig, platformFsUtils);
  explorer.publishFirmwareVersions();

  expectVersions("TEST_FPGA_FWVER", "1.2");
  expectVersions("TEST_CPLD_FWVER", "123.456.789");
  expectVersions(
      "TEST_CPLD_BADFWVER", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions(
      "TEST_CPLD_BADFWVER2", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions(
      "TEST_FPGA_BADFWVER", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions(
      "TEST_FPGA_LONGFWVER", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions("FAN0_CPLD_FWVER", "1.2.3");
  expectVersions("TAHAN_SMB_CPLD_TRAP", "7.8.9");
  expectVersions("NONE", PlatformExplorer::kFwVerErrorFileNotFound);
}

// Test that exceptions in createDeviceSymLink are properly handled
TEST(PlatformExplorerTest, SymlinkExceptionHandling) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  // Create a platform config with minimal setup
  PlatformConfig platformConfig;

  // Set required fields for explore() to work
  platformConfig.rootPmUnitName() = "TEST_ROOT_PM";
  platformConfig.rootSlotType() = "TEST_SLOT";

  // Create the slot type config
  SlotTypeConfig slotTypeConfig;
  slotTypeConfig.pmUnitName() = "TEST_ROOT_PM";
  platformConfig.slotTypeConfigs()["TEST_SLOT"] = slotTypeConfig;

  // Create the PM unit config
  PmUnitConfig pmUnitConfig;
  platformConfig.pmUnitConfigs()["TEST_ROOT_PM"] = pmUnitConfig;

  // Add a symbolic link with an unsupported path that will trigger the
  // exception
  std::string unsupportedLinkPath = "/run/devmap/unsupported/device";
  std::string devicePath = "/[TEST_DEVICE]";
  platformConfig.symbolicLinkToDevicePath()[unsupportedLinkPath] = devicePath;

  // Create the explorer
  PlatformExplorer explorer(platformConfig, platformFsUtils);

  // Make sure the directory exists so we can check it later
  std::string unsupportedDir = "/run/devmap/unsupported";
  EXPECT_TRUE(platformFsUtils->createDirectories(unsupportedDir));

  // This should trigger the exception in createDeviceSymLink but it should be
  // caught and not crash the program
  EXPECT_NO_THROW(explorer.explore());

  // Verify that the exception was handled gracefully by checking that
  // the symlink was not created
  EXPECT_FALSE(platformFsUtils->exists(unsupportedLinkPath));
}

} // namespace facebook::fboss::platform::platform_manager
