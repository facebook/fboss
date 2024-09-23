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
void writeVersions(
    std::string path,
    std::string deviceType,
    const char* version,
    const char* subversion,
    const std::shared_ptr<PlatformFsUtils> platformFsUtils) {
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      std::string(version), fmt::format("{}/{}_ver", path, deviceType)));
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      std::string(subversion), fmt::format("{}/{}_sub_ver", path, deviceType)));
}

void expectVersions(
    const char* deviceName,
    const char* versionString,
    int versionOdsValue) {
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          fmt::format(PlatformExplorer::kFirmwareVersion, deviceName)),
      versionOdsValue);
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
  std::string fpgaPath = "/run/devmap/fpgas/TEST_IOB_FPGA";
  writeVersions(fpgaPath, "fpga", "1", "0", platformFsUtils);
  std::string cpldPath = "/run/devmap/cplds/TEST_MCB_CPLD";
  writeVersions(cpldPath, "cpld", "0x4", "0xf", platformFsUtils);
  std::string cpldPath2 = "/run/devmap/cplds/TEST_CPLD_MIXED";
  writeVersions(cpldPath2, "cpld", "0xf", "9", platformFsUtils);
  std::string fpgaPathBadInt = "/run/devmap/fpgas/TEST_FPGA_BAD_INT";
  writeVersions(fpgaPathBadInt, "fpga", "a", " ", platformFsUtils);
  std::string cpldHwmonPath = "/run/devmap/cplds/FAN0_CPLD";
  writeVersions(
      cpldHwmonPath + "/hwmon/hwmon20", "cpld", "99", "99", platformFsUtils);

  // New-style fw_ver file
  std::string fpgaFwVerPath = "/run/devmap/fpgas/TEST_FPGA_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2", fmt::format("{}/fw_ver", fpgaFwVerPath)));
  // fw_ver should be prioritized over old-style
  std::string cpldFwVerPath = "/run/devmap/cplds/TEST_CPLD_FWVER";
  writeVersions(cpldFwVerPath, "cpld", "999", "999", platformFsUtils);
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789", fmt::format("{}/fw_ver", cpldFwVerPath)));
  std::string cpldBadFwVerPath = "/run/devmap/cplds/TEST_CPLD_BADFWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789 // comment", fmt::format("{}/fw_ver", cpldBadFwVerPath)));
  // Case with fw_ver under hwmon
  std::string cpldHwmonFwVerPath = "/run/devmap/cplds/FAN0_CPLD_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2.3", fmt::format("{}/hwmon/hwmon20/fw_ver", cpldHwmonFwVerPath)));

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath2] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaPathBadInt] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonPath] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldBadFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonFwVerPath] = "";

  PlatformExplorer explorer(platformConfig, platformFsUtils);
  explorer.publishFirmwareVersions();

  expectVersions("TEST_IOB_FPGA", "1.0", 1000);
  expectVersions("TEST_MCB_CPLD", "4.15", 4015);
  expectVersions("TEST_CPLD_MIXED", "15.9", 15009);
  expectVersions("TEST_FPGA_BAD_INT", "0.0", 0);
  expectVersions("FAN0_CPLD", "99.99", 99099);

  expectVersions("TEST_FPGA_FWVER", "1.2", 1'002'000);
  expectVersions("TEST_CPLD_FWVER", "123.456.789", 123456789);
  expectVersions("TEST_CPLD_BADFWVER", "ERROR_INVALID_STRING", 0);
  expectVersions("FAN0_CPLD_FWVER", "1.2.3", 1'002'003);
}

} // namespace facebook::fboss::platform::platform_manager
