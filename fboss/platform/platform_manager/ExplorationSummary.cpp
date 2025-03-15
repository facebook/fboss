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
  // This is a interim solution until we solidfy (D61807863).
  // there's no standarization of all version fields yet across vendors.
  // E.g for meru P1/P2, we don't care about productProductionState and P1
  // doesn't have IDPROM to read without some modifications. What do we do in
  // that case? Also, these versions may not be the only thing differentiating
  // the version. So it's a bit early to formalize as thrift definition. For
  // now, hard coded to unblock DSF push testing.
  if (re2::RE2::FullMatch(*platformConfig_.platformName(), kMeruRe)) {
    const auto& [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
    if (!dataStore_.hasPmUnit(slotPath)) {
      // Case-P1, PM will try to create IDPROM but it will fail.
      // PM call ExplorationErrorMap::add but at that point, dataStore won't
      // have the PmUnitInfo.
      if (devicePath == "/[IDPROM]") {
        return true;
      }

      // Case-P1 (Viper), Viper P1 only has one PSU slot when two are defined
      // in config. Check if the other PSU slot has no error. In P2, we
      // confirmed that two PSUs will be plugged-in. Unfortunately, since
      // missing PmUnit in the slot, we can't distinguish P1 and P2. So if one
      // PSU is missing, it'll be treated as expected which isn't hard error
      // in P2.
      if (uint slotNum;
          re2::RE2::FullMatch(slotPath, kMeruPsuSlotPath, &slotNum)) {
        if (!devicePathToErrors_.contains(
                fmt::format("/SMB_SLOT@0/PSU_SLOT@{}", !slotNum))) {
          return true;
        }
      }
    } else {
      auto pmUnitInfo = dataStore_.getPmUnitInfo(slotPath);
      auto productVersion =
          pmUnitInfo.version() ? *pmUnitInfo.version()->productVersion() : 0;
      // Case-P2, PM successfully read productVersion and it can check whether
      // the platform is P2. If so, we expect SCM_IDPROM_P1 to fail.
      if (productVersion >= 2) {
        if (devicePath == "/[SCM_IDPROM_P1]") {
          return true;
        }
      } else {
        // Case-P1 (Viper), the following sensors aren't supported.
        if (devicePath == "/SMB_SLOT@0/[SMB_MGMT_TMP75]" ||
            devicePath == "/SMB_SLOT@0/[SMB_MAX6581]") {
          return true;
        }
      }
    }
  }
  return false;
}

std::unordered_map<std::string, std::vector<ExplorationError>>
ExplorationSummary::getFailedDevices() {
  return devicePathToErrors_;
}
} // namespace facebook::fboss::platform::platform_manager
