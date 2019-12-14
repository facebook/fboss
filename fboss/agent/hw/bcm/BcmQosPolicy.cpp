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

#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
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
    l3IngressQosMap_ = std::move(*qosMapItr);
    warmBootCache->programmed(qosMapItr);
  } else {
    l3IngressQosMap_ =
        std::make_unique<BcmQosMap>(hw, BcmQosMap::Type::IP_INGRESS);
    for (const auto& qosRule : qosPolicy->getRules()) {
      l3IngressQosMap_->addRule(
          BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.queueId),
          qosRule.dscp);
    }
  }
}

BcmQosPolicyHandle BcmQosPolicy::getHandle() const {
  return static_cast<BcmQosPolicyHandle>(l3IngressQosMap_->getHandle());
}

void BcmQosPolicy::update(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  CHECK(oldQosPolicy->getID() == newQosPolicy->getID());
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
    l3IngressQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.queueId),
        qosRule.dscp);
  }

  std::vector<QosRule> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    l3IngressQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.queueId),
        qosRule.dscp);
  }
}

bool BcmQosPolicy::policyMatches(
    const std::shared_ptr<QosPolicy>& qosPolicy) const {
  if (l3IngressQosMap_->size() != qosPolicy->getRules().size()) {
    return false;
  }
  for (const auto& rule : qosPolicy->getRules()) {
    if (!l3IngressQosMap_->ruleExists(
            BcmPortQueueManager::CosQToBcmInternalPriority(rule.queueId),
            rule.dscp)) {
      return false;
    }
  }
  return true;
}

} // namespace fboss
} // namespace facebook
