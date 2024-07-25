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
    const char* version,
    const char* subversion) {
  Utils().createDirectories(path);
  EXPECT_TRUE(
      folly::writeFile(std::string(version), (path + "/fpga_ver").c_str()));
  EXPECT_TRUE(folly::writeFile(
      std::string(subversion), (path + "/fpga_sub_ver").c_str()));
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
  writeVersions(fpgaPath, "1", "0");
  std::string cpldPath =
      tmpDir.path().string() + "/run/devmap/cplds/TEST_MCB_CPLD";
  writeVersions(cpldPath, "0x4", "0xf");
  std::string cpldPath2 =
      tmpDir.path().string() + "/run/devmap/cplds/TEST_CPLD_MIXED";
  writeVersions(cpldPath2, "0xf", "9");

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath2] = "";

  PlatformExplorer explorer(platformConfig);
  explorer.publishFirmwareVersions(tmpDir.path().string());

  expectVersions("TEST_IOB_FPGA", "1.0", 1000);
  expectVersions("TEST_MCB_CPLD", "4.15", 4015);
  expectVersions("TEST_CPLD_MIXED", "15.9", 15009);
}

} // namespace facebook::fboss::platform::platform_manager
