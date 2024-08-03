// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/helpers/MockPlatformUtils.h"
#include "fboss/platform/helpers/PlatformNameLib.h"

using namespace ::testing;
using namespace facebook::fboss::platform::helpers;

namespace facebook::fboss::platform::helpers {

TEST(PlatformNameLibTest, NameFromBios) {
  auto platformUtils = MockPlatformUtils();
  auto platformNameLib = PlatformNameLib();

  // Mapped cases
  EXPECT_CALL(platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "janga"}));
  EXPECT_EQ(
      platformNameLib.getPlatformNameFromBios(platformUtils), "JANGA800BIC");
  EXPECT_CALL(platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "Tahan"}));
  EXPECT_EQ(
      platformNameLib.getPlatformNameFromBios(platformUtils), "TAHAN800BC");
  EXPECT_CALL(platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "MINIPACK3"}));
  EXPECT_EQ(
      platformNameLib.getPlatformNameFromBios(platformUtils), "MONTBLANC");
  EXPECT_CALL(platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "MINIPACK3_MCB"}));
  EXPECT_EQ(
      platformNameLib.getPlatformNameFromBios(platformUtils), "MONTBLANC");
  // Non-mapped
  EXPECT_CALL(platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "MORGAN800CC"}));
  EXPECT_EQ(
      platformNameLib.getPlatformNameFromBios(platformUtils), "MORGAN800CC");
  // Error case
  EXPECT_CALL(platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{1, ""}));
  EXPECT_THROW(
      platformNameLib.getPlatformNameFromBios(platformUtils),
      std::runtime_error);
}

} // namespace facebook::fboss::platform::helpers
