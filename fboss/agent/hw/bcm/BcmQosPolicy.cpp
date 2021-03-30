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

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/hw/bcm/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/QosPolicy.h"

namespace facebook::fboss {

BcmQosPolicy::BcmQosPolicy(
    BcmSwitch* hw,
    const std::shared_ptr<QosPolicy>& qosPolicy)
    : hw_(hw) {
  programIngressDscpQosMap(qosPolicy);
  programIngressExpQosMap(qosPolicy);
  programEgressExpQosMap(qosPolicy);
  initTrafficClassToPgMap(qosPolicy);
  initPfcPriorityToPgMap(qosPolicy);
  initPfcPriorityToQueueMap(qosPolicy);
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
  updateTrafficClassToPgMap(oldQosPolicy, newQosPolicy);
  updatePfcPriorityToPgMap(oldQosPolicy, newQosPolicy);
  updatePfcPriorityToQueueMap(oldQosPolicy, newQosPolicy);
}

void BcmQosPolicy::remove() {
  programTrafficClassToPg(getDefaultTrafficClassToPg());
  programPfcPriorityToPg(getDefaultPfcPriorityToPg());
  programPfcPriorityToQueue({});
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

void BcmQosPolicy::programPfcPriorityToPg(
    const std::vector<int>& pfcPriorityPg) {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  XLOG(DBG2) << "Start to program pfcPriorityToPg";
  auto& tmpPfcPriorityToPg = const_cast<std::vector<int>&>(pfcPriorityPg);
  // an array with index representing pfc priority
  // value is PG Id
  auto rv = bcm_cosq_priority_group_pfc_priority_mapping_profile_set(
      hw_->getUnit(),
      getDefaultProfileId(),
      pfcPriorityPg.size(),
      tmpPfcPriorityToPg.data());
  bcmCheckError(
      rv,
      "Failed to program bcm_cosq_priority_group_pfc_priority_mapping_profile_set, size: ",
      pfcPriorityPg.size());
}

std::vector<bcm_cosq_pfc_class_map_config_t>
BcmQosPolicy::initializeBcmPfcPriToQueueMapping() {
  std::vector<bcm_cosq_pfc_class_map_config_t> cosq_pfc_map;
  // default this struct is all 0
  for (int pfcPri = 0;
       pfcPri <= cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX();
       ++pfcPri) {
    bcm_cosq_pfc_class_map_config_t config;
#ifdef IS_OSS /* TODO: remove once OSS support added */
    memset(&config, 0, sizeof(bcm_cosq_pfc_class_map_config_t));
#else
    bcm_cosq_pfc_class_map_config_t_init(&config);
#endif
    cosq_pfc_map.emplace_back(config);
  }
  return cosq_pfc_map;
}

void BcmQosPolicy::programPfcPriorityToQueue(
    const std::vector<int>& pfcPriorityToQueue) {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }

  XLOG(DBG2) << "Start to program pfcPriorityToQueue";
  auto cosq_pfc_map = BcmQosPolicy::initializeBcmPfcPriToQueueMapping();

  CHECK_GE(cosq_pfc_map.size(), pfcPriorityToQueue.size())
      << "Default pfc pri to queue mapping size : " << cosq_pfc_map.size()
      << " is smaller than pfc priority to queue size : "
      << pfcPriorityToQueue.size();
  for (auto i = 0; i < pfcPriorityToQueue.size(); ++i) {
    cosq_pfc_map.at(i).pfc_enable = 1;
    cosq_pfc_map.at(i).pfc_optimized = 0;
    cosq_pfc_map.at(i).cos_list_bmp |= (1 << pfcPriorityToQueue[i]);
  }

  auto rv = bcm_cosq_pfc_class_config_profile_set(
      hw_->getUnit(),
      getDefaultProfileId(),
      cosq_pfc_map.size(),
      cosq_pfc_map.data());
  bcmCheckError(
      rv,
      "Failed to program bcm_cosq_pfc_class_config_profile_set, size: ",
      cosq_pfc_map.size());
}

void BcmQosPolicy::programTrafficClassToPg(
    const std::vector<int>& trafficClassPg) {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  XLOG(DBG2) << "Start to program trafficClassToPg";
  // program unicast mapping
  programPriorityGroupMapping(
      bcmCosqInputPriPriorityGroupUcMapping,
      trafficClassPg,
      "bcmCosqInputPriPriorityGroupUcMapping");

  // program mcast mapping
  programPriorityGroupMapping(
      bcmCosqInputPriPriorityGroupMcMapping,
      trafficClassPg,
      "bcmCosqInputPriPriorityGroupMcMapping");
}

// case 1: !old cfg, !new cfg
// case 2: !old cfg, new cfg
// case 3: old cfg, !new cfg
// case 4: old cfg!= new cfg
void BcmQosPolicy::updatePfcPriorityToPgMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  if (!newQosPolicy->getPfcPriorityToPgId() &&
      !oldQosPolicy->getPfcPriorityToPgId()) {
    // case 1
    // nothing to do with pfc priority <-> pg id mapping
    return;
  }
  // all other cases handled here
  programPfcPriorityToPgMap(newQosPolicy);
}

// case 1: !old cfg, !new cfg
// case 2: !old cfg, new cfg
// case 3: old cfg, !new cfg
// case 4: old cfg!= new cfg
void BcmQosPolicy::updateTrafficClassToPgMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  if (!newQosPolicy->getTrafficClassToPgId() &&
      !oldQosPolicy->getTrafficClassToPgId()) {
    // case 1
    // nothing to do with traffic class <-> pg id mapping
    return;
  }
  // all other cases handled here
  programTrafficClassToPgMap(newQosPolicy);
}

// case 1: !old cfg, !new cfg
// case 2: !old cfg, new cfg
// case 3: old cfg, !new cfg
// case 4: old cfg!= new cfg
void BcmQosPolicy::updatePfcPriorityToQueueMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  if (!newQosPolicy->getPfcPriorityToQueueId() &&
      !oldQosPolicy->getPfcPriorityToQueueId()) {
    // case 1
    // nothing to do with traffic class <-> pg id mapping
    return;
  }
  // all other cases handled here
  programPfcPriorityToQueueMap(newQosPolicy);
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
      warmBootCache->findQosMap(qosPolicy, BcmQosMap::Type::IP_INGRESS);

  if (qosMapItr != warmBootCache->QosMapId2QosMapEnd()) {
    ingressDscpQosMap_ = std::move(qosMapItr->second);
    warmBootCache->programmed(
        qosPolicy->getName(), BcmQosMap::Type::IP_INGRESS, qosMapItr);
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
      warmBootCache->findQosMap(qosPolicy, BcmQosMap::Type::MPLS_INGRESS);

  if (qosMapItr != warmBootCache->QosMapId2QosMapEnd()) {
    ingressExpQosMap_ = std::move(qosMapItr->second);
    warmBootCache->programmed(
        qosPolicy->getName(), BcmQosMap::Type::MPLS_INGRESS, qosMapItr);
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
  auto qosMapItr =
      warmBootCache->findQosMap(qosPolicy, BcmQosMap::Type::MPLS_EGRESS);

  if (qosMapItr != warmBootCache->QosMapId2QosMapEnd()) {
    egressExpQosMap_ = std::move(qosMapItr->second);
    warmBootCache->programmed(
        qosPolicy->getName(), BcmQosMap::Type::MPLS_EGRESS, qosMapItr);
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

void BcmQosPolicy::programPriorityGroupMapping(
    const bcm_cosq_priority_group_mapping_profile_type_t profileType,
    const std::vector<int>& trafficClassToPgId,
    const std::string& profileTypeStr) {
  auto tmpTrafficClassToPgId =
      const_cast<std::vector<int>&>(trafficClassToPgId);
  auto rv = bcm_cosq_priority_group_mapping_profile_set(
      hw_->getUnit(),
      getDefaultProfileId(),
      profileType,
      trafficClassToPgId.size(),
      tmpTrafficClassToPgId.data());
  bcmCheckError(
      rv,
      "failed to program ",
      profileTypeStr,
      " size: ",
      trafficClassToPgId.size(),
      " type: ",
      profileType);
}

void BcmQosPolicy::programPfcPriorityToPgMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  std::vector<int> pfcPriorityToPg = getDefaultPfcPriorityToPg();
  if (auto pfcPriorityToPgMap = qosPolicy->getPfcPriorityToPgId()) {
    // override with what user configures
    for (const auto& entry : *pfcPriorityToPgMap) {
      CHECK_GT(pfcPriorityToPg.size(), static_cast<int>(entry.first))
          << " Policy: " << qosPolicy->getName()
          << " has pfcPriorityToPgMap size " << pfcPriorityToPg.size()
          << " which is smaller than pfc priority " << entry.first;
      pfcPriorityToPg[entry.first] = entry.second;
    }
  }
  programPfcPriorityToPg(pfcPriorityToPg);
}

void BcmQosPolicy::initTrafficClassToPgMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (!qosPolicy->getTrafficClassToPgId()) {
    return;
  }
  programTrafficClassToPgMap(qosPolicy);
}

void BcmQosPolicy::initPfcPriorityToPgMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (!qosPolicy->getPfcPriorityToPgId()) {
    return;
  }
  programPfcPriorityToPgMap(qosPolicy);
}

void BcmQosPolicy::initPfcPriorityToQueueMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (!qosPolicy->getPfcPriorityToQueueId()) {
    return;
  }
  programPfcPriorityToQueueMap(qosPolicy);
}

void BcmQosPolicy::programTrafficClassToPgMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  // init the array with HW defaults
  std::vector<int> trafficClassToPg = getDefaultTrafficClassToPg();
  if (auto trafficClassToPgMap = qosPolicy->getTrafficClassToPgId()) {
    // override with what user configures
    for (const auto& entry : *trafficClassToPgMap) {
      CHECK_GT(trafficClassToPg.size(), static_cast<int>(entry.first))
          << " Policy: " << qosPolicy->getName()
          << " has trafficClassToPgMap size " << trafficClassToPg.size()
          << " which is smaller than traffic class " << entry.first;
      trafficClassToPg[entry.first] = entry.second;
    }
  }
  programTrafficClassToPg(trafficClassToPg);
}

void BcmQosPolicy::programPfcPriorityToQueueMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  std::vector<int> pfcPriorityToQueue = {};
  if (const auto& pfcPriorityToQueueMap =
          qosPolicy->getPfcPriorityToQueueId()) {
    pfcPriorityToQueue.assign(
        cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX() + 1, 0);
    for (const auto& entry : *pfcPriorityToQueueMap) {
      CHECK_GT(pfcPriorityToQueue.size(), static_cast<int>(entry.first))
          << " Policy: " << qosPolicy->getName()
          << " has pfcPriorityToQueueMap size " << pfcPriorityToQueue.size()
          << " which is smaller than pfc priority value " << entry.first;
      pfcPriorityToQueue[entry.first] = entry.second;
    }
  }
  programPfcPriorityToQueue(pfcPriorityToQueue);
}

} // namespace facebook::fboss
