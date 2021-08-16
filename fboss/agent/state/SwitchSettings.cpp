/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SwitchSettings.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/state/NodeBase-defs.h"

namespace {
constexpr auto kL2LearningMode = "l2LearningMode";
constexpr auto kQcmEnable = "qcmEnable";
constexpr auto kPtpTcEnable = "ptpTcEnable";
constexpr auto kL2AgeTimerSeconds = "l2AgeTimerSeconds";
constexpr auto kMaxRouteCounterIDs = "maxRouteCounterIDs";
constexpr auto kBlockNeighbors = "blockNeighbors";
constexpr auto kBlockNeighborVlanID = "blockNeighborVlanID";
constexpr auto kBlockNeighborIP = "blockNeighborIP";
} // namespace

namespace facebook::fboss {

folly::dynamic SwitchSettingsFields::toFollyDynamic() const {
  folly::dynamic switchSettings = folly::dynamic::object;

  switchSettings[kL2LearningMode] = static_cast<int>(l2LearningMode);
  switchSettings[kQcmEnable] = static_cast<bool>(qcmEnable);
  switchSettings[kPtpTcEnable] = static_cast<bool>(ptpTcEnable);
  switchSettings[kL2AgeTimerSeconds] = static_cast<int>(l2AgeTimerSeconds);
  switchSettings[kMaxRouteCounterIDs] = static_cast<int>(maxRouteCounterIDs);

  switchSettings[kBlockNeighbors] = folly::dynamic::array;
  for (const auto& [vlanID, ipAddress] : blockNeighbors) {
    folly::dynamic jsonEntry = folly::dynamic::object;
    jsonEntry[kBlockNeighborVlanID] = folly::to<std::string>(vlanID);
    jsonEntry[kBlockNeighborIP] = folly::to<std::string>(ipAddress);
    switchSettings[kBlockNeighbors].push_back(jsonEntry);
  }

  return switchSettings;
}

SwitchSettingsFields SwitchSettingsFields::fromFollyDynamic(
    const folly::dynamic& json) {
  SwitchSettingsFields switchSettings = SwitchSettingsFields();

  if (json.find(kL2LearningMode) != json.items().end()) {
    switchSettings.l2LearningMode =
        cfg::L2LearningMode(json[kL2LearningMode].asInt());
  }
  if (json.find(kQcmEnable) != json.items().end()) {
    switchSettings.qcmEnable = json[kQcmEnable].asBool();
  }
  if (json.find(kPtpTcEnable) != json.items().end()) {
    switchSettings.ptpTcEnable = json[kPtpTcEnable].asBool();
  }
  if (json.find(kL2AgeTimerSeconds) != json.items().end()) {
    switchSettings.l2AgeTimerSeconds = json[kL2AgeTimerSeconds].asInt();
  }
  if (json.find(kMaxRouteCounterIDs) != json.items().end()) {
    switchSettings.maxRouteCounterIDs = json[kMaxRouteCounterIDs].asInt();
  }

  if (json.find(kBlockNeighbors) != json.items().end()) {
    for (const auto& entry : json[kBlockNeighbors]) {
      switchSettings.blockNeighbors.emplace_back(
          entry[kBlockNeighborVlanID].asInt(),
          entry[kBlockNeighborIP].asString());
    }
  }

  return switchSettings;
}

SwitchSettings* SwitchSettings::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newSwitchSettings = clone();
  auto* ptr = newSwitchSettings.get();
  (*state)->resetSwitchSettings(std::move(newSwitchSettings));
  return ptr;
}

bool SwitchSettings::operator==(const SwitchSettings& switchSettings) const {
  return (
      (getFields()->l2LearningMode == switchSettings.getL2LearningMode()) &&
      (getFields()->qcmEnable == switchSettings.isQcmEnable()) &&
      (getFields()->ptpTcEnable == switchSettings.isPtpTcEnable()) &&
      (getFields()->qcmEnable == switchSettings.isQcmEnable()) &&
      (getFields()->l2AgeTimerSeconds ==
       switchSettings.getL2AgeTimerSeconds()) &&
      getFields()->blockNeighbors == switchSettings.getBlockNeighbors());
}

template class NodeBaseT<SwitchSettings, SwitchSettingsFields>;

} // namespace facebook::fboss
