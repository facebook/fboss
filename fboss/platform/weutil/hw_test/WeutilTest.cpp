/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/weutil/hw_test/WeutilTest.h"

#include <gtest/gtest.h>
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/weutil/ContentValidator.h"
#include "fboss/platform/weutil/Weutil.h"

namespace facebook::fboss::platform {

WeutilTest::~WeutilTest() {}

void WeutilTest::SetUp() {
  weutilInstance = createWeUtilIntf("chassis", "", 0);
}

void WeutilTest::TearDown() {}

TEST_F(WeutilTest, getWedgeInfo) {
  EXPECT_GT(weutilInstance->getContents().size(), 0);
}

TEST_F(WeutilTest, getEepromPaths) {
  auto config = getWeUtilConfig();
  EXPECT_GT(config.fruEepromList()->size(), 0);
}

TEST_F(WeutilTest, ValidateAllEepromContents) {
  auto config = getWeUtilConfig();
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  bool isDarwin = platformName && *platformName == "DARWIN";
  EXPECT_GT(config.fruEepromList()->size(), 0);
  for (const auto& [eepromName, eepromConfig] : *config.fruEepromList()) {
    std::string fruName = eepromName;
    std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::tolower);
    try {
      auto weutilInstance = createWeUtilIntf(fruName, "", 0);
      auto contents = weutilInstance->getContents();
      EXPECT_GT(contents.size(), 0)
          << "EEPROM " << fruName << " returned empty contents";
      // Darwin content validation not supported yet!
      if (!isDarwin) {
        EXPECT_TRUE(ContentValidator().isValid(contents))
            << "EEPROM " << fruName << " contents failed validation";
      }
    } catch (const std::exception& e) {
      FAIL() << "Exception when testing EEPROM " << fruName << ": " << e.what();
    }
  }
}

} // namespace facebook::fboss::platform

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
