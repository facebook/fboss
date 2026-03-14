// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/MockPlatformFsUtils.h"
#include "fboss/platform/platform_manager/PciExplorer.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

TEST(PciSubDeviceRuntimeErrorTest, ExceptionBehavior) {
  const std::string errorMsg = "Test error message";
  const std::string pmUnitScopedName = "TEST_DEVICE";

  PciSubDeviceRuntimeError error(errorMsg, pmUnitScopedName);

  // Verify exception message and pmUnitScopedName
  EXPECT_EQ(error.getPmUnitScopedName(), pmUnitScopedName);
  EXPECT_STREQ(error.what(), errorMsg.c_str());

  // Verify can be caught as std::runtime_error
  try {
    throw PciSubDeviceRuntimeError("Test exception", "DEVICE_1");
  } catch (const std::runtime_error& e) {
    EXPECT_STREQ(e.what(), "Test exception");
  }

  // Verify different instances have independent pmUnitScopedName
  PciSubDeviceRuntimeError error1("Error 1", "DEVICE_A");
  PciSubDeviceRuntimeError error2("Error 2", "DEVICE_B");
  EXPECT_EQ(error1.getPmUnitScopedName(), "DEVICE_A");
  EXPECT_EQ(error2.getPmUnitScopedName(), "DEVICE_B");
}

TEST(PciExplorerTest, Construction) {
  // Default construction
  EXPECT_NO_THROW(PciExplorer());

  // Construction with custom PlatformFsUtils
  auto customFsUtils = std::make_shared<MockPlatformFsUtils>();
  EXPECT_NO_THROW({ PciExplorer pciExplorer{customFsUtils}; });
}
