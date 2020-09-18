/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiWredManager.h"

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/state/PortQueue.h"

namespace facebook::fboss {

std::shared_ptr<SaiWred> SaiWredManager::getOrCreateProfile(
    const PortQueue& queue) {
  if (!queue.getAqms().size()) {
    return nullptr;
  }
  auto c = profileCreateAttrs(queue);
  return wredProfiles_
      .refOrEmplace(c, c, c, managerTable_->switchManager().getSwitchSaiId())
      .first;
}

SaiWredTraits::CreateAttributes SaiWredManager::profileCreateAttrs(
    const PortQueue& queue) const {
  SaiWredTraits::CreateAttributes attrs;
  using Attributes = SaiWredTraits::Attributes;
  std::get<Attributes::GreenEnable>(attrs) = false;
  std::get<Attributes::EcnMarkMode>(attrs) = SAI_ECN_MARK_MODE_NONE;

  for (const auto& [type, aqm] : queue.getAqms()) {
    auto thresholds = (*aqm.detection_ref()).get_linear();
    auto [minLen, maxLen] = std::make_pair(
        *thresholds.minimumLength_ref(), *thresholds.maximumLength_ref());
    switch (type) {
      case cfg::QueueCongestionBehavior::EARLY_DROP:
        std::get<Attributes::GreenEnable>(attrs) = true;
        std::get<std::optional<Attributes::GreenMinThreshold>>(attrs) = minLen;
        std::get<std::optional<Attributes::GreenMaxThreshold>>(attrs) = maxLen;
        break;
      case cfg::QueueCongestionBehavior::ECN:
        std::get<Attributes::EcnMarkMode>(attrs) = SAI_ECN_MARK_MODE_GREEN;
        std::get<std::optional<Attributes::EcnGreenMinThreshold>>(attrs) =
            minLen;
        std::get<std::optional<Attributes::EcnGreenMaxThreshold>>(attrs) =
            maxLen;
        break;
    }
  }
  return attrs;
}
} // namespace facebook::fboss
