/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fixmyfboss/CheckRunner.h"

#include <stdexcept>

#include <gtest/gtest.h>

using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_checks;

namespace {

class FakeCheck : public PlatformCheck {
 public:
  FakeCheck(
      std::string name,
      std::set<std::string> platforms = {},
      bool throws = false)
      : name_(std::move(name)),
        platforms_(std::move(platforms)),
        throws_(throws) {}

  CheckResult run() override {
    if (throws_) {
      throw std::runtime_error("boom");
    }
    return makeOK();
  }
  CheckType getType() const override {
    return CheckType::PCI_DEVICE_CHECK;
  }
  std::string getDescription() const override {
    return name_;
  }
  std::string getName() const override {
    return name_;
  }
  std::set<std::string> getSupportedPlatforms() const override {
    return platforms_;
  }

 private:
  std::string name_;
  std::set<std::string> platforms_;
  bool throws_;
};

std::vector<std::string> names(const std::vector<CheckResult>& results) {
  std::vector<std::string> out;
  for (const auto& result : results) {
    out.push_back(*result.checkName());
  }
  return out;
}

} // namespace

TEST(CheckRunnerTest, RunsOnlyChecksForPlatform) {
  std::vector<std::unique_ptr<PlatformCheck>> checks;
  checks.push_back(std::make_unique<FakeCheck>("any"));
  checks.push_back(
      std::make_unique<FakeCheck>("other", std::set<std::string>{"OTHER"}));
  checks.push_back(
      std::make_unique<FakeCheck>("mine", std::set<std::string>{"MONTBLANC"}));

  auto results = fixmyfboss::CheckRunner("MONTBLANC").run(checks);

  EXPECT_EQ(names(results), (std::vector<std::string>{"any", "mine"}));
}

TEST(CheckRunnerTest, ExceptionBecomesError) {
  std::vector<std::unique_ptr<PlatformCheck>> checks;
  checks.push_back(
      std::make_unique<FakeCheck>(
          "throws", std::set<std::string>{}, /*throws=*/true));

  auto results = fixmyfboss::CheckRunner("MONTBLANC").run(checks);

  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(*results[0].status(), CheckStatus::ERROR);
  EXPECT_EQ(
      *results[0].errorMessage(), "Exception during check execution: boom");
}

TEST(CheckRunnerTest, UnavailableHostIsSkippedWithReason) {
  class NeedsHostCheck : public FakeCheck {
   public:
    NeedsHostCheck() : FakeCheck("needs host", {}, /*throws=*/true) {}
    std::optional<std::string> getSkipReason() const override {
      return "no BMC";
    }
  };
  std::vector<std::unique_ptr<PlatformCheck>> checks;
  checks.push_back(std::make_unique<NeedsHostCheck>());

  auto results = fixmyfboss::CheckRunner("MONTBLANC").run(checks);

  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(*results[0].status(), CheckStatus::SKIPPED);
  EXPECT_EQ(*results[0].errorMessage(), "no BMC");
}
