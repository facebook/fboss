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

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::fixmyfboss {

using namespace platform_checks;

bool CheckRunner::appliesToPlatform(const PlatformCheck& check) const {
  auto supportedPlatforms = check.getSupportedPlatforms();
  // Empty set means all platforms are supported
  return supportedPlatforms.empty() ||
      supportedPlatforms.contains(platformName_);
}

std::vector<CheckResult> CheckRunner::run(
    const std::vector<std::unique_ptr<PlatformCheck>>& checks) const {
  std::vector<CheckResult> results;
  for (const auto& check : checks) {
    if (appliesToPlatform(*check)) {
      results.push_back(runOne(*check));
    }
  }
  return results;
}

CheckResult CheckRunner::runOne(PlatformCheck& check) const {
  XLOG(DBG2) << "Running check: " << check.getDescription();
  try {
    auto result = check.run();
    XLOG(DBG2) << "Check " << check.getDescription()
               << " completed with status: "
               << static_cast<int>(*result.status());
    return result;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception in check " << check.getDescription() << ": "
              << e.what();
    CheckResult errorResult;
    errorResult.checkType() = check.getType();
    errorResult.checkName() = check.getName();
    errorResult.status() = CheckStatus::ERROR;
    errorResult.errorMessage() =
        std::string("Exception during check execution: ") + e.what();
    return errorResult;
  }
}

} // namespace facebook::fboss::platform::fixmyfboss
