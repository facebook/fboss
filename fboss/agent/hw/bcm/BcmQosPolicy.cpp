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

namespace facebook::fboss {

BcmQosPolicy::BcmQosPolicy(
    BcmSwitch* hw,
    const std::shared_ptr<QosPolicy>& qosPolicy)
    : hw_(hw) {
  programIngressDscpQosMap(qosPolicy);
  programIngressExpQosMap(qosPolicy);
  programEgressExpQosMap(qosPolicy);
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
  updateIngressDscpQosMap(oldQosPolicy, newQosPolicy);
  updateEgressExpQosMap(oldQosPolicy, newQosPolicy);
  updateIngressExpQosMap(oldQosPolicy, newQosPolicy);
}

void BcmQosPolicy::updateIngressDscpQosMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  if (!ingressDscpQosMap_) {
    programIngressDscpQosMap(newQosPolicy);
    return;
  }
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
void BcmQosPolicy::updateIngressExpQosMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  CHECK(oldQosPolicy->getID() == newQosPolicy->getID());
  if (!ingressExpQosMap_) {
    programIngressExpQosMap(newQosPolicy);
    return;
  }
  const auto& oldRules = oldQosPolicy->getExpMap().from();
  const auto& newRules = newQosPolicy->getExpMap().from();

  std::vector<TrafficClassToQosAttributeMapEntry<EXP>> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    ingressExpQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.trafficClass()),
        qosRule.attr());
  }

  std::vector<TrafficClassToQosAttributeMapEntry<EXP>> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    ingressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.trafficClass()),
        qosRule.attr());
  }
}

void BcmQosPolicy::updateEgressExpQosMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  CHECK(oldQosPolicy->getID() == newQosPolicy->getID());
  if (!egressExpQosMap_) {
    programEgressExpQosMap(newQosPolicy);
    return;
  }
  const auto& oldRules = oldQosPolicy->getExpMap().to();
  const auto& newRules = newQosPolicy->getExpMap().to();

  std::vector<TrafficClassToQosAttributeMapEntry<EXP>> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    egressExpQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(qosRule.trafficClass()),
        qosRule.attr());
  }

  std::vector<TrafficClassToQosAttributeMapEntry<EXP>> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    egressExpQosMap_->addRule(
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
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getDscpMap().from().empty()) {
    return;
  }
  auto warmBootCache = hw_->getWarmBootCache();
  auto qosMapItr =
      warmBootCache->findIngressDscpMap(qosPolicy->getDscpMap().from());
  auto qosMapItr2 = warmBootCache->findQosMap(
      qosPolicy->getName(), BcmQosMap::Type::IP_INGRESS);
  if (qosMapItr != warmBootCache->qosMaps_end()) {
    // TODO(pshaikh): remove this after a push as earlier version may not have
    // required bcm switch's warmboot state.
    ingressDscpQosMap_ = std::move(*qosMapItr);
    warmBootCache->programmed(qosMapItr);
    if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
      warmBootCache->programmed(qosMapItr2);
    }
    return;
  }

  if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
    ingressDscpQosMap_ = std::make_unique<BcmQosMap>(
        hw_,
        BcmQosMap::getQosMapFlags(BcmQosMap::Type::IP_INGRESS),
        qosMapItr2->second);
    warmBootCache->programmed(qosMapItr2);
    return;
  }

  ingressDscpQosMap_ =
      std::make_unique<BcmQosMap>(hw_, BcmQosMap::Type::IP_INGRESS);
  for (const auto& dscpToTrafficClass : qosPolicy->getDscpMap().from()) {
    ingressDscpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(
            dscpToTrafficClass.trafficClass()),
        dscpToTrafficClass.attr());
  }
}

void BcmQosPolicy::programIngressExpQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getExpMap().from().empty()) {
    return;
  }
  auto warmBootCache = hw_->getWarmBootCache();
  auto qosMapItr =
      warmBootCache->findIngressExpMap(qosPolicy->getExpMap().from());
  auto qosMapItr2 = warmBootCache->findQosMap(
      qosPolicy->getName(), BcmQosMap::Type::MPLS_INGRESS);
  if (qosMapItr != warmBootCache->qosMaps_end()) {
    // TODO(pshaikh): remove this after a push as earlier version may not have
    // required bcm switch's warmboot state.
    ingressExpQosMap_ = std::move(*qosMapItr);
    warmBootCache->programmed(qosMapItr);
    if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
      warmBootCache->programmed(qosMapItr2);
    }
    return;
  }

  if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
    ingressExpQosMap_ = std::make_unique<BcmQosMap>(
        hw_,
        BcmQosMap::getQosMapFlags(BcmQosMap::Type::MPLS_INGRESS),
        qosMapItr2->second);
    warmBootCache->programmed(qosMapItr2);
    return;
  }
  ingressExpQosMap_ =
      std::make_unique<BcmQosMap>(hw_, BcmQosMap::Type::MPLS_INGRESS);
  for (const auto& expToTrafficClass : qosPolicy->getExpMap().from()) {
    ingressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(
            expToTrafficClass.trafficClass()),
        expToTrafficClass.attr());
  }
}

void BcmQosPolicy::programEgressExpQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getExpMap().to().empty()) {
    return;
  }
  auto warmBootCache = hw_->getWarmBootCache();
  auto qosMapItr = warmBootCache->findEgressExpMap(qosPolicy->getExpMap().to());
  auto qosMapItr2 = warmBootCache->findQosMap(
      qosPolicy->getName(), BcmQosMap::Type::MPLS_EGRESS);
  if (qosMapItr != warmBootCache->qosMaps_end()) {
    // TODO(pshaikh): remove this after a push as earlier version may not have
    // required bcm switch's warmboot state.
    egressExpQosMap_ = std::move(*qosMapItr);
    warmBootCache->programmed(qosMapItr);
    if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
      warmBootCache->programmed(qosMapItr2);
    }
    return;
  }
  if (qosMapItr2 != warmBootCache->QosMapKey2QosMapIdEnd()) {
    egressExpQosMap_ = std::make_unique<BcmQosMap>(
        hw_,
        BcmQosMap::getQosMapFlags(BcmQosMap::Type::MPLS_EGRESS),
        qosMapItr2->second);
    warmBootCache->programmed(qosMapItr2);
    return;
  }
  egressExpQosMap_ =
      std::make_unique<BcmQosMap>(hw_, BcmQosMap::Type::MPLS_EGRESS);
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

} // namespace facebook::fboss
