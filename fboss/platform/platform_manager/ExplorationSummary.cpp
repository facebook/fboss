// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ExplorationSummary.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/StructuredLogger.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::platform_manager {

ExplorationSummary::ExplorationSummary(const PlatformConfig& config)
    : platformConfig_(config) {}

void ExplorationSummary::addError(
    ExplorationErrorType errorType,
    const std::string& devicePath,
    const std::string& message) {
  ExplorationError newError;
  newError.errorType() = toExplorationErrorTypeStr(errorType);
  newError.message() = message;

  if (isExpectedError(platformConfig_, errorType, devicePath)) {
    devicePathToExpectedErrors_[devicePath].push_back(newError);
    nExpectedErrs_++;
    return;
  }
  devicePathToErrors_[devicePath].push_back(newError);
  nErrs_++;
}

void ExplorationSummary::addError(
    ExplorationErrorType errorType,
    const std::string& slotPath,
    const std::string& deviceName,
    const std::string& errorMessage) {
  addError(
      errorType, Utils().createDevicePath(slotPath, deviceName), errorMessage);
}

void ExplorationSummary::print(ExplorationStatus finalStatus) {
  if (finalStatus == ExplorationStatus::SUCCEEDED) {
    XLOG(INFO) << fmt::format(
        "Successfully explored {}...", *platformConfig_.platformName());
  } else {
    XLOG(INFO) << fmt::format(
        "Explored {} with {} unexpected errors and {} expected errors...",
        *platformConfig_.platformName(),
        nErrs_,
        nExpectedErrs_);
  }
  for (const auto& [devicePath, explorationErrors] : devicePathToErrors_) {
    XLOG_FIRST_N(INFO, 1) << "=========== UNEXPECTED ERRORS ===========";
    for (const auto& error : explorationErrors) {
      XLOG(INFO) << fmt::format("{}: {}", devicePath, *error.message());
    }
  }
  for (const auto& [devicePath, explorationErrors] :
       devicePathToExpectedErrors_) {
    XLOG_FIRST_N(INFO, 1) << "=========== EXPECTED ERRORS ===========";
    for (const auto& error : explorationErrors) {
      XLOG(INFO) << fmt::format("{}: {}", devicePath, *error.message());
    }
  }
}

void ExplorationSummary::publishCounters(ExplorationStatus finalStatus) {
  fb303::fbData->setCounter(
      kExplorationFail,
      finalStatus != ExplorationStatus::SUCCEEDED &&
          finalStatus != ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS);
  fb303::fbData->setCounter(
      kExplorationSucceeded, finalStatus == ExplorationStatus::SUCCEEDED);
  fb303::fbData->setCounter(
      kExplorationSucceededWithExpectedErrors,
      finalStatus == ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS);
  fb303::fbData->setCounter(kTotalFailures, nErrs_);
  fb303::fbData->setCounter(kTotalExpectedFailures, nExpectedErrs_);

  std::unordered_map<std::string, uint> numErrPerType;
  for (const auto& [devicePath, explorationErrors] : devicePathToErrors_) {
    for (const auto& error : explorationErrors) {
      numErrPerType[*error.errorType()]++;
    }
  }
  for (const auto errorType : explorationErrorTypeList()) {
    const auto errorTypeStr = toExplorationErrorTypeStr(errorType);
    const auto key = fmt::format(kExplorationFailByType, errorTypeStr);
    fb303::fbData->setCounter(key, numErrPerType[errorTypeStr]);
  }
}

ExplorationStatus ExplorationSummary::summarize(
    const std::unordered_map<std::string, std::string>& firmwareVersions,
    const std::unordered_map<std::string, std::string>& hardwareVersions) {
  ExplorationStatus finalStatus = ExplorationStatus::FAILED;
  if (devicePathToErrors_.empty() && devicePathToExpectedErrors_.empty()) {
    finalStatus = ExplorationStatus::SUCCEEDED;
  } else if (devicePathToErrors_.empty()) {
    finalStatus = ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS;
  }
  // Exploration summary reporting.
  print(finalStatus);
  publishCounters(finalStatus);

  helpers::StructuredLogger structuredLogger("platform_manager");
  structuredLogger.addFwTags(firmwareVersions);
  structuredLogger.setTags(hardwareVersions);

  // Log individual errors
  for (const auto& [devicePath, explorationErrors] : devicePathToErrors_) {
    for (const auto& error : explorationErrors) {
      structuredLogger.logAlert(
          *error.errorType(),
          *error.message(),
          {
              {"device_path", devicePath},
              {"exploration_status",
               apache::thrift::util::enumNameSafe(finalStatus)},
          });
      XLOG(INFO) << "Logged Platform Manager error: " << devicePath;
    }
  }

  // Log exploration completion
  structuredLogger.logEvent(
      "exploration_complete",
      {
          {"exploration_status",
           apache::thrift::util::enumNameSafe(finalStatus)},
      });

  return finalStatus;
}

std::map<std::string, std::vector<ExplorationError>>
ExplorationSummary::getFailedDevices() {
  return devicePathToErrors_;
}
} // namespace facebook::fboss::platform::platform_manager
