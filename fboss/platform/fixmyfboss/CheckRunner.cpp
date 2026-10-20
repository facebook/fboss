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

#include <algorithm>

#include <stdexcept>

#include <folly/logging/xlog.h>
#include <re2/re2.h>

namespace facebook::fboss::platform::fixmyfboss {

using namespace platform_checks;

CheckRunner::CheckRunner(std::string platformName, RunOptions options)
    : platformName_(std::move(platformName)), options_(std::move(options)) {
  if (options_.nameFilter) {
    RE2::Options re2Options;
    re2Options.set_case_sensitive(false);
    re2Options.set_log_errors(false);
    auto filter = std::make_shared<RE2>(*options_.nameFilter, re2Options);
    if (!filter->ok()) {
      throw std::invalid_argument(filter->error());
    }
    nameFilter_ = std::move(filter);
  }
}

bool CheckRunner::isSelected(const PlatformCheck& check) const {
  return !nameFilter_ || RE2::PartialMatch(check.getName(), *nameFilter_);
}

bool CheckRunner::appliesToPlatform(const PlatformCheck& check) const {
  auto supportedPlatforms = check.getSupportedPlatforms();
  // Empty set means all platforms are supported
  return supportedPlatforms.empty() ||
      supportedPlatforms.contains(platformName_);
}

std::vector<PlannedCheck> CheckRunner::plan(
    const std::vector<std::unique_ptr<PlatformCheck>>& checks) const {
  std::vector<PlannedCheck> planned;
  for (const auto& check : checks) {
    if (!check || !isSelected(*check)) {
      continue;
    }
    PlannedCheck& p = planned.emplace_back(
        PlannedCheck{
            .check = *check, .appliesToPlatform = appliesToPlatform(*check)});
    if (!p.appliesToPlatform) {
      p.skipReason = "Not supported on " + platformName_;
      continue;
    }
    // A check's skip reason may query the hosts and throw.
    try {
      p.skipReason = getSkipReason(*check);
    } catch (const std::exception& ex) {
      p.skipReasonError = ex.what();
    }
  }
  std::stable_partition(planned.begin(), planned.end(), [](const auto& p) {
    return !p.check.get().isDisruptive();
  });
  return planned;
}

std::vector<CheckResult> CheckRunner::run(
    const std::vector<PlannedCheck>& plan) const {
  std::vector<CheckResult> results;
  results.reserve(plan.size());
  for (const auto& planned : plan) {
    if (planned.appliesToPlatform) {
      results.push_back(runOne(planned));
    }
  }
  return results;
}

std::optional<std::string> CheckRunner::getSkipReason(
    const PlatformCheck& check) const {
  // Before the check's own reason, which may query the hosts.
  if (check.isDisruptive() && !options_.includeDisruptive) {
    return "Disruptive; pass --include-disruptive to run";
  }
  return check.getSkipReason();
}

CheckResult CheckRunner::runOne(const PlannedCheck& planned) const {
  auto& check = planned.check.get();
  if (planned.skipReason) {
    CheckResult skipped;
    skipped.checkType() = check.getType();
    skipped.checkName() = check.getName();
    skipped.status() = CheckStatus::SKIPPED;
    skipped.errorMessage() = *planned.skipReason;
    return skipped;
  }
  XLOG(DBG2) << "Running check: " << check.getDescription();
  try {
    if (planned.skipReasonError) {
      throw std::runtime_error(*planned.skipReasonError);
    }
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
