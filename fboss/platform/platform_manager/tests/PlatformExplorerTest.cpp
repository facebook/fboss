// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/PlatformExplorer.h"
#include "fboss/platform/platform_manager/Utils.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
void writeVersions(
    std::string path,
    std::string deviceType,
    const char* version,
    const char* subversion) {
  Utils().createDirectories(path);
  EXPECT_TRUE(folly::writeFile(
      std::string(version),
      fmt::format("{}/{}_ver", path, deviceType).c_str()));
  EXPECT_TRUE(folly::writeFile(
      std::string(subversion),
      fmt::format("{}/{}_sub_ver", path, deviceType).c_str()));
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
  std::string fpgaPath =
      tmpDir.path().string() + "/run/devmap/fpgas/TEST_IOB_FPGA";
  writeVersions(fpgaPath, "fpga", "1", "0");
  std::string cpldPath =
      tmpDir.path().string() + "/run/devmap/cplds/TEST_MCB_CPLD";
  writeVersions(cpldPath, "cpld", "0x4", "0xf");
  std::string cpldPath2 =
      tmpDir.path().string() + "/run/devmap/cplds/TEST_CPLD_MIXED";
  writeVersions(cpldPath2, "cpld", "0xf", "9");
  std::string fpgaPathBadInt =
      tmpDir.path().string() + "/run/devmap/fpgas/TEST_FPGA_BAD_INT";
  writeVersions(fpgaPathBadInt, "fpga", "a", " ");

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath2] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaPathBadInt] = "";

  PlatformExplorer explorer(platformConfig);
  explorer.publishFirmwareVersions(tmpDir.path().string());

  expectVersions("TEST_IOB_FPGA", "1.0", 1000);
  expectVersions("TEST_MCB_CPLD", "4.15", 4015);
  expectVersions("TEST_CPLD_MIXED", "15.9", 15009);
  expectVersions("TEST_FPGA_BAD_INT", "0.0", 0);
}

} // namespace facebook::fboss::platform::platform_manager
