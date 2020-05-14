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
} // namespace

namespace facebook::fboss {

folly::dynamic SwitchSettingsFields::toFollyDynamic() const {
  folly::dynamic switchSettings = folly::dynamic::object;

  switchSettings[kL2LearningMode] = static_cast<int>(l2LearningMode);
  switchSettings[kQcmEnable] = static_cast<bool>(qcmEnable);

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
      (getFields()->qcmEnable == switchSettings.isQcmEnable()));
}

template class NodeBaseT<SwitchSettings, SwitchSettingsFields>;

} // namespace facebook::fboss
