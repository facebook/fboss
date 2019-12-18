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
  programIngressDscpQosMap(hw, qosPolicy);
  programIngressExpQosMap(hw, qosPolicy);
  programEgressExpQosMap(hw, qosPolicy);
}

BcmQosPolicyHandle BcmQosPolicy::getHandle(BcmQosMap::Type type) const {
  switch (type) {
    case BcmQosMap::Type::IP_INGRESS:
      if (ingressDscpQosMap_) {
        return static_cast<BcmQosPolicyHandle>(ingressDscpQosMap_->getHandle());
      }
      break;
    case BcmQosMap::Type::IP_EGRESS:
      return static_cast<BcmQosPolicyHandle>(-1);
      break;
    case BcmQosMap::Type::MPLS_INGRESS:
      if (ingressExpQosMap_) {
        return static_cast<BcmQosPolicyHandle>(ingressExpQosMap_->getHandle());
      }

      break;
    case BcmQosMap::Type::MPLS_EGRESS:
      if (egressExpQosMap_) {
        return static_cast<BcmQosPolicyHandle>(egressExpQosMap_->getHandle());
      }
      break;
  }
  return static_cast<BcmQosPolicyHandle>(-1);
}

void BcmQosPolicy::update(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  CHECK(oldQosPolicy->getID() == newQosPolicy->getID());
  const auto& oldRules = oldQosPolicy->getDscpMap().from();
  const auto& newRules = newQosPolicy->getDscpMap().from();

  std::vector<TrafficClassToQosAttributeMapEntry<DSCP>> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    ingressDscpQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.trafficClass()),
        qosRule.attr());
  }

  std::vector<TrafficClassToQosAttributeMapEntry<DSCP>> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    ingressDscpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.trafficClass()),
        qosRule.attr());
  }
}

bool BcmQosPolicy::policyMatches(
    const std::shared_ptr<QosPolicy>& qosPolicy) const {
  if (ingressDscpQosMap_->size() != qosPolicy->getDscpMap().from().size()) {
    return false;
  }
  for (const auto& rule : qosPolicy->getDscpMap().from()) {
    if (!ingressDscpQosMap_->ruleExists(
            BcmPortQueueManager::CosQToBcmInternalPriority(rule.trafficClass()),
            rule.attr())) {
      return false;
    }
  }
  return true;
}

void BcmQosPolicy::programIngressDscpQosMap(
    BcmSwitch* hw,
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  auto warmBootCache = hw->getWarmBootCache();
  auto qosMapItr =
      warmBootCache->findIngressDscpMap(qosPolicy->getDscpMap().from());
  if (qosMapItr != warmBootCache->qosMaps_end()) {
    // TODO(pshaikh): remove this after a push as earlier version may not have
    // required bcm switch's warmboot state.
    ingressDscpQosMap_ = std::move(*qosMapItr);
    warmBootCache->programmed(qosMapItr);
    return;
  }

  auto qosMapItr2 = warmBootCache->findQosMap(
      qosPolicy->getName(), BcmQosMap::Type::IP_INGRESS);
  if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
    ingressDscpQosMap_ = std::make_unique<BcmQosMap>(
        hw, BcmQosMap::Type::IP_INGRESS, qosMapItr2->second);
    warmBootCache->programmed(qosMapItr2);
    return;
  }

  ingressDscpQosMap_ =
      std::make_unique<BcmQosMap>(hw, BcmQosMap::Type::IP_INGRESS);
  for (const auto& dscpToTrafficClass : qosPolicy->getDscpMap().from()) {
    ingressDscpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(
            dscpToTrafficClass.trafficClass()),
        dscpToTrafficClass.attr());
  }
}

void BcmQosPolicy::programIngressExpQosMap(
    BcmSwitch* hw,
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  auto warmBootCache = hw->getWarmBootCache();
  auto qosMapItr = warmBootCache->findQosMap(
      qosPolicy->getName(), BcmQosMap::Type::MPLS_INGRESS);
  if (qosMapItr != warmBootCache->QosMapKey2QosMapIdEnd()) {
    ingressExpQosMap_ = std::make_unique<BcmQosMap>(
        hw,
        BcmQosMap::getQosMapFlags(BcmQosMap::Type::MPLS_INGRESS),
        qosMapItr->second);
    warmBootCache->programmed(qosMapItr);
    return;
  }
  ingressExpQosMap_ =
      std::make_unique<BcmQosMap>(hw, BcmQosMap::Type::MPLS_INGRESS);
  for (const auto& expToTrafficClass : qosPolicy->getExpMap().from()) {
    ingressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(
            expToTrafficClass.trafficClass()),
        expToTrafficClass.attr());
  }
}

void BcmQosPolicy::programEgressExpQosMap(
    BcmSwitch* hw,
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getExpMap().to().empty()) {
    return;
  }
  auto warmBootCache = hw->getWarmBootCache();
  auto qosMapItr = warmBootCache->findQosMap(
      qosPolicy->getName(), BcmQosMap::Type::MPLS_EGRESS);
  if (qosMapItr != warmBootCache->QosMapKey2QosMapIdEnd()) {
    egressExpQosMap_ = std::make_unique<BcmQosMap>(
        hw,
        BcmQosMap::getQosMapFlags(BcmQosMap::Type::MPLS_EGRESS),
        qosMapItr->second);
    warmBootCache->programmed(qosMapItr);
    return;
  }
  egressExpQosMap_ =
      std::make_unique<BcmQosMap>(hw, BcmQosMap::Type::MPLS_EGRESS);
  for (const auto& trafficClassToExp : qosPolicy->getExpMap().to()) {
    egressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(
            trafficClassToExp.trafficClass()),
        trafficClassToExp.attr());
  }
}

const BcmQosMap* BcmQosPolicy::getIngressDscpQosMap() const {
  return ingressDscpQosMap_.get();
}

const BcmQosMap* BcmQosPolicy::getIngressExpQosMap() const {
  return ingressExpQosMap_.get();
}

const BcmQosMap* BcmQosPolicy::getEgressExpQosMap() const {
  return egressExpQosMap_.get();
}
} // namespace fboss
} // namespace facebook
