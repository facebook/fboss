/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/weutil/ContentValidator.h"
#include "fboss/platform/weutil/Weutil.h"

namespace facebook::fboss::platform {

class WeutilTest : public ::testing::Test {
 public:
  void SetUp() override {
    weutilInstance_ = createWeUtilIntf("chassis", "", 0);
    config_ = getWeUtilConfig();

    for (const auto& [eepromName, eepromConfig] : *config_.fruEepromList()) {
      std::string fruName = eepromName;
      std::transform(
          fruName.begin(), fruName.end(), fruName.begin(), ::tolower);
      fruList_[fruName] = eepromConfig;
    }
  }

 protected:
  std::unique_ptr<WeutilInterface> weutilInstance_;
  weutil_config::WeutilConfig config_;
  std::unordered_map<std::string, weutil_config::FruEepromConfig> fruList_;
};

TEST_F(WeutilTest, getWedgeInfo) {
  EXPECT_GT(weutilInstance_->getContents().size(), 0);
}

TEST_F(WeutilTest, getEepromPaths) {
  EXPECT_GT(config_.fruEepromList()->size(), 0);
}

TEST_F(WeutilTest, ValidateAllEepromContents) {
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  bool isDarwin = platformName && *platformName == "DARWIN";
  EXPECT_GT(config_.fruEepromList()->size(), 0);
  for (const auto& [fruName, eepromConfig] : fruList_) {
    try {
      auto weutilInstance =
          createWeUtilIntf(fruName, "", *eepromConfig.offset());
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

TEST_F(WeutilTest, getInfoJson) {
  for (const auto& [fruName, eepromConfig] : fruList_) {
    try {
      auto weutilInstance = createWeUtilIntf(
          fruName, *eepromConfig.path(), *eepromConfig.offset());
      auto contents = weutilInstance->getContents();
      EXPECT_GT(contents.size(), 0)
          << "EEPROM " << fruName << " returned empty contents";
      auto json = weutilInstance->getInfoJson();
      EXPECT_GE(json.size(), 3)
          << "EEPROM " << fruName << " returned empty json";
    } catch (const std::exception& e) {
      FAIL() << "Exception when testing EEPROM content in JSON format "
             << fruName << ": " << e.what();
    }
  }
}
} // namespace facebook::fboss::platform

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
