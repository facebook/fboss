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
#include <string>
#include <vector>

#include "fboss/platform/platform_checks/PlatformCheck.h"

namespace facebook::fboss::platform::fixmyfboss {

/**
 * Runs checks applicable to a platform and turns exceptions into ERROR
 * results, so one broken check cannot abort the whole run.
 */
class CheckRunner {
 public:
  explicit CheckRunner(std::string platformName)
      : platformName_(std::move(platformName)) {}

  bool appliesToPlatform(const platform_checks::PlatformCheck& check) const;

  std::vector<platform_checks::CheckResult> run(
      const std::vector<std::unique_ptr<platform_checks::PlatformCheck>>&
          checks) const;

 private:
  platform_checks::CheckResult runOne(
      platform_checks::PlatformCheck& check) const;

  std::string platformName_;
};

} // namespace facebook::fboss::platform::fixmyfboss
