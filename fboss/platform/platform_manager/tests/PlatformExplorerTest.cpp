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

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldPath2] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaPathBadInt] = "";

  PlatformExplorer explorer(platformConfig, platformFsUtils);
  explorer.publishFirmwareVersions();

  expectVersions("TEST_IOB_FPGA", "1.0", 1000);
  expectVersions("TEST_MCB_CPLD", "4.15", 4015);
  expectVersions("TEST_CPLD_MIXED", "15.9", 15009);
  expectVersions("TEST_FPGA_BAD_INT", "0.0", 0);
}

} // namespace facebook::fboss::platform::platform_manager
