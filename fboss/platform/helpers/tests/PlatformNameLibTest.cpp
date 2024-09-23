// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/MockPlatformFsUtils.h"
#include "fboss/platform/helpers/MockPlatformUtils.h"
#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/helpers/PlatformNameLib.h"

using namespace ::testing;
using namespace facebook::fboss::platform::helpers;

namespace facebook::fboss::platform::helpers {

TEST(PlatformNameLibTest, NameFromBios) {
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto platformFsUtils = std::make_shared<MockPlatformFsUtils>();
  auto platformNameLib = PlatformNameLib(platformUtils, platformFsUtils);

  // Mapped cases
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "janga"}));
  EXPECT_EQ(platformNameLib.getPlatformNameFromBios(), "JANGA800BIC");
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "Tahan"}));
  EXPECT_EQ(platformNameLib.getPlatformNameFromBios(), "TAHAN800BC");
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "MINIPACK3"}));
  EXPECT_EQ(platformNameLib.getPlatformNameFromBios(), "MONTBLANC");
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "MINIPACK3_MCB"}));
  EXPECT_EQ(platformNameLib.getPlatformNameFromBios(), "MONTBLANC");
  // Non-mapped
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "MORGAN800CC"}));
  EXPECT_EQ(platformNameLib.getPlatformNameFromBios(), "MORGAN800CC");
  // Error case
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{1, ""}));
  EXPECT_THROW(platformNameLib.getPlatformNameFromBios(), std::runtime_error);
}

TEST(PlatformNameLibTest, GetPlatformNameNoCache) {
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto platformFsUtils = std::make_shared<MockPlatformFsUtils>();
  auto platformNameLib = PlatformNameLib(platformUtils, platformFsUtils);

  // Cache reads always fail, other ops always succeed.
  EXPECT_CALL(*platformFsUtils, createDirectories).WillRepeatedly(Return(true));
  EXPECT_CALL(*platformFsUtils, getStringFileContent)
      .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(*platformFsUtils, writeStringToFile).WillRepeatedly(Return(true));

  // Success
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "janga"}));
  EXPECT_EQ(platformNameLib.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
  // Failure
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{1, ""}));
  EXPECT_EQ(platformNameLib.getPlatformName(), std::nullopt);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      1);
}

TEST(PlatformNameLibTest, GetPlatformNameCache) {
  fb303::fbData->zeroStats();
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());
  auto platformNameLib = PlatformNameLib(platformUtils, platformFsUtils);

  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .Times(2)
      .WillRepeatedly(Return(std::pair{0, "janga"}));

  // Successful bios read, no write to cache
  EXPECT_EQ(platformNameLib.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
  // Write to cache as PM would
  EXPECT_EQ(platformNameLib.getPlatformNameFromBios(true), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
  // Further calls to dmidecode fail
  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillRepeatedly(Return(std::pair{1, ""}));
  // Cache hit - success with no bios read
  EXPECT_EQ(platformNameLib.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
  // Cache hit 2
  EXPECT_EQ(platformNameLib.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
  // Cache failure + dmidecode failure
  auto platformFsUtilsNoRead = std::make_shared<MockPlatformFsUtils>();
  auto platformNameLibNoRead =
      PlatformNameLib(platformUtils, platformFsUtilsNoRead);
  EXPECT_CALL(*platformFsUtilsNoRead, getStringFileContent)
      .WillRepeatedly(Return(std::nullopt));
  EXPECT_EQ(platformNameLibNoRead.getPlatformName(), std::nullopt);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      1);
}

TEST(PlatformNameLibTest, CounterLogic) {
  fb303::fbData->resetAllData();
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformUtils = std::make_shared<MockPlatformUtils>();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  EXPECT_CALL(*platformUtils, execCommand(PlatformNameLib::dmidecodeCommand))
      .WillOnce(Return(std::pair{0, "janga"}))
      .WillOnce(Return(std::pair{1, ""}))
      .WillOnce(Return(std::pair{0, "janga"}))
      .WillRepeatedly(Return(std::pair{1, ""}));

  auto platformNameLib = PlatformNameLib(platformUtils, platformFsUtils);
  EXPECT_EQ(platformNameLib.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);

  fb303::fbData->resetAllData();
  auto platformNameLib2 = PlatformNameLib(platformUtils, platformFsUtils);
  // After resetting stats but before calling getPlatformName, counter should
  // be uninitialized and therefore getCounter should throw.
  EXPECT_THROW(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      std::invalid_argument);
  EXPECT_EQ(platformNameLib2.getPlatformName(), std::nullopt);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      1);

  fb303::fbData->resetAllData();
  auto platformNameLib3 = PlatformNameLib(platformUtils, platformFsUtils);
  EXPECT_EQ(platformNameLib3.getPlatformNameFromBios(true), "JANGA800BIC");
  EXPECT_EQ(platformNameLib3.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
  auto platformNameLib4 = PlatformNameLib(platformUtils, platformFsUtils);
  EXPECT_EQ(platformNameLib4.getPlatformName(), "JANGA800BIC");
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          PlatformNameLib::kPlatformNameBiosReadFailures),
      0);
}

} // namespace facebook::fboss::platform::helpers
