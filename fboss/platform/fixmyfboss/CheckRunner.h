/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fboss/platform/platform_checks/PlatformCheck.h"

namespace facebook::fboss::platform::fixmyfboss {

struct RunOptions {
  // Run checks that reboot or power cycle parts of the switch.
  bool includeDisruptive{false};
};

/**
 * Runs checks applicable to a platform and turns exceptions into ERROR
 * results, so one broken check cannot abort the whole run. Disruptive checks
 * run last, so that they cannot affect the other checks.
 */
class CheckRunner {
 public:
  explicit CheckRunner(std::string platformName, RunOptions options = {})
      : platformName_(std::move(platformName)), options_(options) {}

  bool appliesToPlatform(const platform_checks::PlatformCheck& check) const;

  std::vector<platform_checks::CheckResult> run(
      const std::vector<std::unique_ptr<platform_checks::PlatformCheck>>&
          checks) const;

 private:
  platform_checks::CheckResult runOne(
      platform_checks::PlatformCheck& check) const;
  std::optional<std::string> getSkipReason(
      const platform_checks::PlatformCheck& check) const;

  std::string platformName_;
  RunOptions options_;
};

} // namespace facebook::fboss::platform::fixmyfboss
