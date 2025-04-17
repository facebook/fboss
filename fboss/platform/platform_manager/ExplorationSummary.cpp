// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ExplorationSummary.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::platform_manager {
namespace {
const re2::RE2 kMeruRe{"meru800b[if]a"};
const re2::RE2 kMeruPsuSlotPath{"/SMB_SLOT@0/PSU_SLOT@(?P<SlotNum>[01])"};
} // namespace

ExplorationSummary::ExplorationSummary(
    const PlatformConfig& config,
    const DataStore& dataStore)
    : platformConfig_(config), dataStore_(dataStore) {}

void ExplorationSummary::addError(
    ExplorationErrorType errorType,
    const std::string& devicePath,
    const std::string& message) {
  ExplorationError newError;
  newError.errorType() = toExplorationErrorTypeStr(errorType);
  newError.message() = message;
  if (isDeviceExpectedToFail(devicePath)) {
    devicePathToExpectedErrors_[devicePath].push_back(newError);
    nExpectedErrs_++;
  } else {
    devicePathToErrors_[devicePath].push_back(newError);
    nErrs_++;
  }
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
    auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
    XLOG(INFO) << fmt::format(
        "Unexpected Failures in Device {} for PmUnit {} at SlotPath {}",
        deviceName,
        dataStore_.hasPmUnit(slotPath)
            ? *dataStore_.getPmUnitInfo(slotPath).name()
            : "<ABSENT>",
        slotPath);
    for (int i = 1; const auto& error : explorationErrors) {
      XLOG(INFO) << fmt::format("{}. {}", i++, *error.message());
    }
  }
  for (const auto& [devicePath, explorationErrors] :
       devicePathToExpectedErrors_) {
    auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
    XLOG(INFO) << fmt::format(
        "Expected Failures in Device {} for PmUnit {} at SlotPath {}",
        deviceName,
        dataStore_.hasPmUnit(slotPath)
            ? *dataStore_.getPmUnitInfo(slotPath).name()
            : "<ABSENT>",
        slotPath);
    for (int i = 1; const auto& error : explorationErrors) {
      XLOG(INFO) << fmt::format("{}. {}", i++, *error.message());
    }
  }
}

void ExplorationSummary::publishCounters(ExplorationStatus finalStatus) {
  fb303::fbData->setCounter(
      kExplorationFail,
      finalStatus != ExplorationStatus::SUCCEEDED &&
          finalStatus != ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS);
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

ExplorationStatus ExplorationSummary::summarize() {
  ExplorationStatus finalStatus = ExplorationStatus::FAILED;
  if (devicePathToErrors_.empty() && devicePathToExpectedErrors_.empty()) {
    finalStatus = ExplorationStatus::SUCCEEDED;
  } else if (devicePathToErrors_.empty()) {
    finalStatus = ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS;
  }
  // Exploration summary reporting.
  print(finalStatus);
  publishCounters(finalStatus);
  return finalStatus;
}

bool ExplorationSummary::isDeviceExpectedToFail(const std::string& devicePath) {
  return false;
}

std::unordered_map<std::string, std::vector<ExplorationError>>
ExplorationSummary::getFailedDevices() {
  return devicePathToErrors_;
}
} // namespace facebook::fboss::platform::platform_manager
