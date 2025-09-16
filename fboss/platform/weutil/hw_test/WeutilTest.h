#pragma once

#include <gtest/gtest.h>
#include "fboss/platform/weutil/Weutil.h"

namespace facebook::fboss::platform {

class WeutilTest : public ::testing::Test {
 public:
  ~WeutilTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<WeutilInterface> weutilInstance;
  weutil_config::WeutilConfig config;
  std::unordered_map<std::string, weutil_config::FruEepromConfig> fruList;
};
} // namespace facebook::fboss::platform
