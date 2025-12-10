// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ExplorationSummary.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/ScubaLogger.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::platform_manager {
namespace {
const re2::RE2 kPsuSlotPath{R"(/PSU_SLOT@\d+$)"};
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
  if ((errorType == ExplorationErrorType::SLOT_PM_UNIT_ABSENCE ||
       errorType == ExplorationErrorType::RUN_DEVMAP_SYMLINK) &&
      isSlotExpectedToBeEmpty(devicePath)) {
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

void ExplorationSummary::publishToScuba(ExplorationStatus finalStatus) {
  // Log individual errors
  for (const auto& [devicePath, explorationErrors] : devicePathToErrors_) {
    for (const auto& error : explorationErrors) {
      std::unordered_map<std::string, std::string> normals;

      normals["platform"] = *platformConfig_.platformName();
      normals["device_path"] = devicePath;
      normals["event"] = *error.errorType();
      normals["error_message"] = *error.message();
      normals["exploration_status"] =
          apache::thrift::util::enumNameSafe(finalStatus);

      ScubaLogger::log(normals);
      XLOG(INFO) << "Logged Platform Manager error to Scuba: " << devicePath;
    }
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
  publishToScuba(finalStatus);
  return finalStatus;
}

bool ExplorationSummary::isSlotExpectedToBeEmpty(
    const std::string& devicePath) {
  auto [slotPath, _] = Utils().parseDevicePath(devicePath);
  if (re2::RE2::PartialMatch(slotPath, kPsuSlotPath)) {
    return true;
  }
  return false;
}

std::map<std::string, std::vector<ExplorationError>>
ExplorationSummary::getFailedDevices() {
  return devicePathToErrors_;
}
} // namespace facebook::fboss::platform::platform_manager
