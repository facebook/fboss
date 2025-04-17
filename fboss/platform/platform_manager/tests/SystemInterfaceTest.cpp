// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/args.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/PkgManager.h"

using namespace ::testing;

namespace facebook::fboss::platform::platform_manager {
class MockPlatformUtils : public PlatformUtils {
 public:
  MOCK_METHOD(
      (std::pair<int, std::string>),
      execCommand,
      (const std::string&),
      (const));
};

TEST(SystemInterfaceTest, lsmod) {
  auto lsmod =
      "Module                  Size  Used by\n"
      "mp3_smbcpld            12288  0\n"
      "mp3_scmcpld            28672  0\n"
      "fboss_iob_xcvr         12288  0\n"
      "fboss_iob_led          12288  0\n"
      "led_class              20480  3 fboss_iob_led,mp3_scmcpld,mp3_fancpld\n"
      "autofs4                49152  2\n";
  auto expectedKmods = std::array<std::string, 6>{
      "mp3_smbcpld",
      "mp3_scmcpld",
      "fboss_iob_xcvr",
      "fboss_iob_led",
      "led_class",
      "autofs4"};
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  EXPECT_CALL(*platformUtils, execCommand(_))
      .WillOnce(Return(std::pair{0, lsmod}));
  package_manager::SystemInterface interface(platformUtils);
  auto kmods = interface.lsmod();
  EXPECT_EQ(kmods.size(), expectedKmods.size());
  for (const auto& expectedKmod : expectedKmods) {
    EXPECT_TRUE(kmods.contains(expectedKmod));
  }
}

}; // namespace facebook::fboss::platform::platform_manager
