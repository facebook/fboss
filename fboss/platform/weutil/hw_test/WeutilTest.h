#pragma once

#include <gtest/gtest.h>

namespace facebook::fboss::platform {

class WeutilInterface;

class WeutilTest : public ::testing::Test {
 public:
  ~WeutilTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<WeutilInterface> weutilInstance;
};
} // namespace facebook::fboss::platform
