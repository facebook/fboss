/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_checks/checks/MacAddressCheck.h"
#include "fboss/platform/platform_checks/checks/PciDeviceCheck.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"

namespace facebook::fboss::platform {

class PlatformHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    platformConfig_ = platform_manager::ConfigUtils().getConfig();
  }

 protected:
  platform_manager::PlatformConfig platformConfig_;
};

TEST_F(PlatformHwTest, CorrectMacX86) {
  platform_checks::MacAddressCheck macCheck;
  auto result = macCheck.run();

  if (result.status() != platform_checks::CheckStatus::OK) {
    std::string failureMsg = "MAC address check failed";
    if (result.errorMessage().has_value()) {
      failureMsg += ": " + *result.errorMessage();
    }
    FAIL() << failureMsg;
  }
}

TEST_F(PlatformHwTest, PCIDevicesPresent) {
  platform_checks::PciDeviceCheck pciCheck;
  auto result = pciCheck.run();

  if (result.status() != platform_checks::CheckStatus::OK) {
    std::string failureMsg = "PCI device check failed";
    if (result.errorMessage().has_value()) {
      failureMsg += ": " + *result.errorMessage();
    }
    FAIL() << failureMsg;
  }
}

} // namespace facebook::fboss::platform

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
