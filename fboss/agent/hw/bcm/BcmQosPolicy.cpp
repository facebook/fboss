/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosPolicy.h"

#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/state/QosPolicy.h"

namespace facebook {
namespace fboss {

BcmQosPolicy::BcmQosPolicy(
    BcmSwitch* hw,
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  auto warmBootCache = hw->getWarmBootCache();
  auto qosMapItr = warmBootCache->findIngressQosMap(qosPolicy->getRules());
  if (qosMapItr != warmBootCache->ingressQosMaps_end()) {
    qosMap_ = std::move(*qosMapItr);
    warmBootCache->programmed(qosMapItr);
  } else {
    qosMap_ = std::make_unique<BcmQosMap>(hw);
    for (const auto& qosRule : qosPolicy->getRules()) {
      qosMap_->addRule(qosRule);
    }
  }
}

BcmQosPolicyHandle BcmQosPolicy::getHandle() const {
  return static_cast<BcmQosPolicyHandle>(qosMap_->getHandle());
}

void BcmQosPolicy::update(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  CHECK(oldQosPolicy->getID() == oldQosPolicy->getID());
  const auto& oldRules = oldQosPolicy->getRules();
  const auto& newRules = newQosPolicy->getRules();

  std::vector<QosRule> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    qosMap_->removeRule(qosRule);
  }

  std::vector<QosRule> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    qosMap_->addRule(qosRule);
  }
}

bool BcmQosPolicy::policyMatches(
    const std::shared_ptr<QosPolicy>& qosPolicy) const {
  return qosMap_->rulesMatch(qosPolicy->getRules());
}

} // namespace fboss
} // namespace facebook
