// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ExplorationErrorMap.h"

#include <re2/re2.h>

#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::platform_manager {
namespace {
const re2::RE2 kMeruRe{"meru800b[if]a"};
const re2::RE2 kMeruPsuSlotPath{"/SMB_SLOT@0/PSU_SLOT@(?P<SlotNum>[01])"};
} // namespace

ExplorationErrorMap::ExplorationErrorMap(
    const PlatformConfig& config,
    const DataStore& dataStore)
    : platformConfig_(config), dataStore_(dataStore) {}

void ExplorationErrorMap::add(
    const std::string& devicePath,
    const std::string& message) {
  if (auto it = devicePathToErrors_.find(devicePath);
      it != devicePathToErrors_.end()) {
    auto& errorMessages = it->second;
    errorMessages.getMessages().push_back(message);
  } else {
    bool isExpected = isExpectedDeviceToFail(devicePath);
    if (isExpected) {
      ++nExpectedErrs;
    }
    devicePathToErrors_[devicePath] = ErrorMessages(isExpected, {message});
  }
}

void ExplorationErrorMap::add(
    const std::string& slotPath,
    const std::string& deviceName,
    const std::string& errorMessage) {
  add(Utils().createDevicePath(slotPath, deviceName), errorMessage);
}

size_t ExplorationErrorMap::size() {
  return devicePathToErrors_.size();
}

bool ExplorationErrorMap::empty() {
  return size() == 0;
}

bool ExplorationErrorMap::hasExpectedErrors() {
  return nExpectedErrs > 0;
}

size_t ExplorationErrorMap::numExpectedErrors() {
  return nExpectedErrs;
}

Iterator ExplorationErrorMap::begin() {
  return Iterator{devicePathToErrors_.begin()};
}

Iterator ExplorationErrorMap::end() {
  return Iterator{devicePathToErrors_.end()};
}

bool ExplorationErrorMap::isExpectedDeviceToFail(
    const std::string& devicePath) {
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
} // namespace facebook::fboss::platform::platform_manager
