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

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/state/PortQueue.h"

namespace facebook::fboss {

#if !defined(BRCM_SAI_SDK_XGS_AND_DNX) and !defined(CHENAB_SAI_SDK)
constexpr auto kDefaultDropProbability = 100;
#endif

std::shared_ptr<SaiWred> SaiWredManager::getOrCreateProfile(
    const PortQueue& queue) {
  if (!queue.getAqms() || queue.getAqms()->empty()) {
    return nullptr;
  }
  auto attributes = profileCreateAttrs(queue);
  auto& store = saiStore_->get<SaiWredTraits>();
  SaiWredTraits::AdapterHostKey k = tupleProjection<
      SaiWredTraits::CreateAttributes,
      SaiWredTraits::AdapterHostKey>(attributes);
  return store.setObject(k, attributes);
}

SaiWredTraits::CreateAttributes SaiWredManager::profileCreateAttrs(
    const PortQueue& queue) const {
  SaiWredTraits::CreateAttributes attrs;
  using Attributes = SaiWredTraits::Attributes;
  std::get<Attributes::GreenEnable>(attrs) = false;
  std::get<Attributes::EcnMarkMode>(attrs) = SAI_ECN_MARK_MODE_NONE;
  auto& greenMin =
      std::get<std::optional<Attributes::GreenMinThreshold>>(attrs);
  auto& greenMax =
      std::get<std::optional<Attributes::GreenMaxThreshold>>(attrs);
  auto& greenDropProbability =
      std::get<std::optional<Attributes::GreenDropProbability>>(attrs);
  auto& ecnGreenMin =
      std::get<std::optional<Attributes::EcnGreenMinThreshold>>(attrs);
  auto& ecnGreenMax =
      std::get<std::optional<Attributes::EcnGreenMaxThreshold>>(attrs);
#if defined(TAJO_SDK)
  // TAJO SDK populates greenMin/Max value to ecnGreenMin/Max if nullptr, so use
  // 0 here to avoid that
  std::tie(greenMin, greenMax, greenDropProbability, ecnGreenMin, ecnGreenMax) =
      std::make_tuple(0, 0, kDefaultDropProbability, 0, 0);
#elif !defined(BRCM_SAI_SDK_XGS_AND_DNX) and !defined(CHENAB_SAI_SDK)
  std::tie(greenMin, greenMax, greenDropProbability, ecnGreenMin, ecnGreenMax) =
      std::make_tuple(
          0, 0, kDefaultDropProbability, std::nullopt, std::nullopt);
#endif
  for (const auto& aqm : std::as_const(*queue.getAqms())) {
    // THRIFT_COPY
    auto thresholds = aqm->cref<switch_config_tags::detection>()
                          ->cref<switch_config_tags::linear>()
                          ->toThrift();
    auto [minLen, maxLen] = std::make_pair(
        *thresholds.minimumLength(), *thresholds.maximumLength());
    auto probability = *thresholds.probability();
    switch (aqm->cref<switch_config_tags::behavior>()->cref()) {
      case cfg::QueueCongestionBehavior::EARLY_DROP:
        std::get<Attributes::GreenEnable>(attrs) = true;
        greenMin = minLen;
        greenMax = maxLen;
        greenDropProbability = probability;
        break;
      case cfg::QueueCongestionBehavior::ECN:
        std::get<Attributes::EcnMarkMode>(attrs) = SAI_ECN_MARK_MODE_GREEN;
        ecnGreenMin = minLen;
        ecnGreenMax = maxLen;
        break;
    }
  }
  return attrs;
}

void SaiWredManager::removeUnclaimedWredProfile() {
  saiStore_->get<SaiWredTraits>().removeUnclaimedWarmbootHandlesIf(
      [](const auto& wred) {
        wred->release();
        return true;
      });
}

} // namespace facebook::fboss
