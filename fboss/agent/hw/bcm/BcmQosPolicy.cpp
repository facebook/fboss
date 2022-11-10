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
  programTrafficClassToPgIfNeeded(getDefaultTrafficClassToPg());
  programPfcPriorityToPgIfNeeded(getDefaultPfcPriorityToPg());
  programPfcPriorityToQueueIfNeeded({});
}

void BcmQosPolicy::updateIngressDscpQosMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  if (!ingressDscpQosMap_) {
    programIngressDscpQosMap(newQosPolicy);
    return;
  }
  auto oldRules =
      oldQosPolicy->getDscpMap()->cref<switch_state_tags::from>()->toThrift();
  auto newRules =
      newQosPolicy->getDscpMap()->cref<switch_state_tags::from>()->toThrift();
  std::vector<state::TrafficClassToQosAttributeEntry> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    ingressDscpQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(*qosRule.trafficClass()),
        *qosRule.attr());
  }

  std::vector<state::TrafficClassToQosAttributeEntry> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    ingressDscpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(*qosRule.trafficClass()),
        *qosRule.attr());
  }
}

std::vector<int> BcmQosPolicy::readPfcPriorityToPg(const BcmSwitch* hw) {
  std::vector<int> pfcPri2Pg = {};
  if (!hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return pfcPri2Pg;
  }
  int maxSize = getDefaultPfcPriorityToPg().size();
  pfcPri2Pg.resize(maxSize);
  int arrayCount = 0;
  auto rv = bcm_cosq_priority_group_pfc_priority_mapping_profile_get(
      hw->getUnit(),
      getDefaultProfileId(),
      maxSize,
      pfcPri2Pg.data(),
      &arrayCount);
  bcmCheckError(
      rv,
      "Failed bcm_cosq_priority_group_pfc_priority_mapping_profile_get, size: ",
      maxSize);
  return pfcPri2Pg;
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

void BcmQosPolicy::programPfcPriorityToPgIfNeeded(
    const std::vector<int>& newPfcPriorityPg) {
  auto currPfcPriorityToPg = readPfcPriorityToPg(hw_);
  if (currPfcPriorityToPg == newPfcPriorityPg) {
    return;
  }
  programPfcPriorityToPg(newPfcPriorityPg);
}

std::vector<bcm_cosq_pfc_class_map_config_t>
BcmQosPolicy::getBcmHwDefaultsPfcPriToQueueMapping() {
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

std::vector<bcm_cosq_pfc_class_map_config_t>
BcmQosPolicy::readPfcPriorityToQueue(const BcmSwitch* hw) {
  std::vector<bcm_cosq_pfc_class_map_config_t> pfcPri2QueueId = {};
  if (!hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return pfcPri2QueueId;
  }

  int arrayCount = 0;
  int maxSize = getDefaultPfcPriorityToPg().size();
  pfcPri2QueueId.resize(maxSize);
  auto rv = bcm_cosq_pfc_class_config_profile_get(
      hw->getUnit(),
      getDefaultProfileId(),
      maxSize,
      pfcPri2QueueId.data(),
      &arrayCount);
  bcmCheckError(
      rv, "Failed bcm_cosq_pfc_class_config_profile_get, maxSize: ", maxSize);
  return pfcPri2QueueId;
}

void BcmQosPolicy::programPfcPriorityToQueue(
    const std::vector<bcm_cosq_pfc_class_map_config_t>& pfcPriorityToQueue) {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }

  XLOG(DBG2) << "Start to program pfcPriorityToQueue";
  auto tmpPfcPriorityToQueue =
      const_cast<std::vector<bcm_cosq_pfc_class_map_config_t>&>(
          pfcPriorityToQueue);
  auto rv = bcm_cosq_pfc_class_config_profile_set(
      hw_->getUnit(),
      getDefaultProfileId(),
      pfcPriorityToQueue.size(),
      tmpPfcPriorityToQueue.data());
  bcmCheckError(
      rv,
      "Failed to program bcm_cosq_pfc_class_config_profile_set, size: ",
      tmpPfcPriorityToQueue.size());
}

bool BcmQosPolicy::comparePfcPriorityToQueue(
    std::vector<bcm_cosq_pfc_class_map_config_t>& newPfcPriorityToQueue,
    std::vector<bcm_cosq_pfc_class_map_config_t>& currPfcPriorityToQueue) {
  // return false if both do not match
  if (newPfcPriorityToQueue.size() != currPfcPriorityToQueue.size()) {
    return false;
  }
  int arrSize = newPfcPriorityToQueue.size();
  for (int i = 0; i < arrSize; ++i) {
    if ((newPfcPriorityToQueue.at(i).cos_list_bmp !=
         currPfcPriorityToQueue.at(i).cos_list_bmp) ||
        (newPfcPriorityToQueue.at(i).pfc_optimized !=
         currPfcPriorityToQueue.at(i).pfc_optimized) ||
        (newPfcPriorityToQueue.at(i).pfc_enable !=
         currPfcPriorityToQueue.at(i).pfc_enable)) {
      return false;
    }
  }
  return true;
}

void BcmQosPolicy::programPfcPriorityToQueueIfNeeded(
    const std::vector<int>& newPfcPriorityToQueue) {
  auto currCosPfcMap = readPfcPriorityToQueue(hw_);
  auto newCosPfcMap = BcmQosPolicy::getBcmHwDefaultsPfcPriToQueueMapping();
  CHECK_GE(newCosPfcMap.size(), newPfcPriorityToQueue.size())
      << "Default pfc pri to queue mapping size : " << newCosPfcMap.size()
      << " is smaller than pfc priority to queue size : "
      << newPfcPriorityToQueue.size();
  for (auto i = 0; i < newPfcPriorityToQueue.size(); ++i) {
    newCosPfcMap.at(i).pfc_enable = 1;
    newCosPfcMap.at(i).pfc_optimized = 0;
    newCosPfcMap.at(i).cos_list_bmp = (1 << newPfcPriorityToQueue[i]);
  }
  if (BcmQosPolicy::comparePfcPriorityToQueue(newCosPfcMap, currCosPfcMap)) {
    return;
  }
  programPfcPriorityToQueue(newCosPfcMap);
}

void BcmQosPolicy::programTrafficClassToPgIfNeeded(
    const std::vector<int>& trafficClassPg) {
  // program unicast mapping
  programPriorityGroupMappingIfNeeded(
      bcmCosqInputPriPriorityGroupUcMapping,
      trafficClassPg,
      "bcmCosqInputPriPriorityGroupUcMapping");

  // program mcast mapping
  programPriorityGroupMappingIfNeeded(
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
  auto oldRules =
      oldQosPolicy->getExpMap()->cref<switch_state_tags::from>()->toThrift();
  auto newRules =
      newQosPolicy->getExpMap()->cref<switch_state_tags::from>()->toThrift();

  std::vector<state::TrafficClassToQosAttributeEntry> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    ingressExpQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(*qosRule.trafficClass()),
        *qosRule.attr());
  }

  std::vector<state::TrafficClassToQosAttributeEntry> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    ingressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(*qosRule.trafficClass()),
        *qosRule.attr());
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

  auto oldRules =
      oldQosPolicy->getExpMap()->cref<switch_state_tags::to>()->toThrift();
  auto newRules =
      newQosPolicy->getExpMap()->cref<switch_state_tags::to>()->toThrift();

  std::vector<state::TrafficClassToQosAttributeEntry> toBeRemoved;
  std::set_difference(
      oldRules.begin(),
      oldRules.end(),
      newRules.begin(),
      newRules.end(),
      std::inserter(toBeRemoved, toBeRemoved.end()));
  for (const auto& qosRule : toBeRemoved) {
    egressExpQosMap_->removeRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(*qosRule.trafficClass()),
        *qosRule.attr());
  }

  std::vector<state::TrafficClassToQosAttributeEntry> toBeAdded;
  std::set_difference(
      newRules.begin(),
      newRules.end(),
      oldRules.begin(),
      oldRules.end(),
      std::inserter(toBeAdded, toBeAdded.end()));
  for (const auto& qosRule : toBeAdded) {
    egressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(*qosRule.trafficClass()),
        *qosRule.attr());
  }
}

bool BcmQosPolicy::policyMatches(
    const std::shared_ptr<QosPolicy>& qosPolicy) const {
  if (ingressDscpQosMap_->size() !=
      qosPolicy->getDscpMap()->cref<switch_state_tags::from>()->size()) {
    return false;
  }
  auto& from = qosPolicy->getDscpMap()->cref<switch_state_tags::from>();
  for (const auto& rule : std::as_const(*from)) {
    auto tc = rule->cref<switch_state_tags::trafficClass>()->toThrift();
    auto dscp = rule->cref<switch_state_tags::attr>()->toThrift();
    if (!ingressDscpQosMap_->ruleExists(
            BcmPortQueueManager::CosQToBcmInternalPriority(tc), dscp)) {
      return false;
    }
  }
  return true;
}

void BcmQosPolicy::programIngressDscpQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getDscpMap()->cref<switch_state_tags::from>()->empty()) {
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
  auto& from = qosPolicy->getDscpMap()->cref<switch_state_tags::from>();
  for (const auto& rule : std::as_const(*from)) {
    auto tc = rule->cref<switch_state_tags::trafficClass>()->toThrift();
    auto dscp = rule->cref<switch_state_tags::attr>()->toThrift();

    ingressDscpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(tc), dscp);
  }
}

void BcmQosPolicy::programIngressExpQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getExpMap()->cref<switch_state_tags::from>()->empty()) {
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
  auto& from = qosPolicy->getExpMap()->cref<switch_state_tags::from>();
  for (const auto& rule : std::as_const(*from)) {
    auto tc = rule->cref<switch_state_tags::trafficClass>()->toThrift();
    auto exp = rule->cref<switch_state_tags::attr>()->toThrift();
    ingressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(tc), exp);
  }
}

void BcmQosPolicy::programEgressExpQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (qosPolicy->getExpMap()->cref<switch_state_tags::to>()->empty()) {
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
  auto& to = qosPolicy->getExpMap()->cref<switch_state_tags::to>();
  for (const auto& rule : std::as_const(*to)) {
    auto tc = rule->cref<switch_state_tags::trafficClass>()->toThrift();
    auto exp = rule->cref<switch_state_tags::attr>()->toThrift();

    egressExpQosMap_->addRule(
        BcmPortQueueManager::CosQToBcmInternalPriority(tc), exp);
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

std::vector<int> BcmQosPolicy::readPriorityGroupMapping(
    const BcmSwitch* hw,
    const bcm_cosq_priority_group_mapping_profile_type_t profileType,
    const std::string& profileTypeStr) {
  std::vector<int> tc2PgId;
  int tcToPgSize = getDefaultTrafficClassToPg().size();
  int arrayCount = 0;

  if (!hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return tc2PgId;
  }

  tc2PgId.resize(tcToPgSize);
  auto rv = bcm_cosq_priority_group_mapping_profile_get(
      hw->getUnit(),
      getDefaultProfileId(),
      profileType,
      tcToPgSize,
      tc2PgId.data(),
      &arrayCount);
  bcmCheckError(rv, "failed to read ", profileTypeStr, " type: ", profileType);
  return tc2PgId;
}

void BcmQosPolicy::programPriorityGroupMapping(
    const bcm_cosq_priority_group_mapping_profile_type_t profileType,
    const std::vector<int>& trafficClassToPgId,
    const std::string& profileTypeStr) {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }

  XLOG(DBG2) << "Program trafficClassToPg";
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

void BcmQosPolicy::programPriorityGroupMappingIfNeeded(
    const bcm_cosq_priority_group_mapping_profile_type_t profileType,
    const std::vector<int>& newTrafficClassToPgId,
    const std::string& profileTypeStr) {
  auto currTrafficClassToPgId =
      readPriorityGroupMapping(hw_, profileType, profileTypeStr);
  if (currTrafficClassToPgId == newTrafficClassToPgId) {
    return;
  }
  programPriorityGroupMapping(
      profileType, newTrafficClassToPgId, profileTypeStr);
}

void BcmQosPolicy::programPfcPriorityToPgMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  std::vector<int> pfcPriorityToPg = getDefaultPfcPriorityToPg();
  if (auto pfcPriorityToPgMap = qosPolicy->getPfcPriorityToPgId()) {
    // override with what user configures
    for (const auto& [prio, pg] : std::as_const(*pfcPriorityToPgMap)) {
      CHECK_GT(pfcPriorityToPg.size(), static_cast<int>(prio))
          << " Policy: " << qosPolicy->getName()
          << " has pfcPriorityToPgMap size " << pfcPriorityToPg.size()
          << " which is smaller than pfc priority " << prio;
      pfcPriorityToPg[prio] = pg->toThrift();
    }
  }
  programPfcPriorityToPgIfNeeded(pfcPriorityToPg);
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
    for (const auto& [tc, pg] : std::as_const(*trafficClassToPgMap)) {
      CHECK_GT(trafficClassToPg.size(), static_cast<int>(tc))
          << " Policy: " << qosPolicy->getName()
          << " has trafficClassToPgMap size " << trafficClassToPg.size()
          << " which is smaller than traffic class " << tc;
      trafficClassToPg[tc] = pg->toThrift();
    }
  }
  programTrafficClassToPgIfNeeded(trafficClassToPg);
}

void BcmQosPolicy::programPfcPriorityToQueueMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  std::vector<int> pfcPriorityToQueue = {};
  if (const auto& pfcPriorityToQueueMap =
          qosPolicy->getPfcPriorityToQueueId()) {
    pfcPriorityToQueue.assign(
        cfg::switch_config_constants::PFC_PRIORITY_VALUE_MAX() + 1, 0);
    for (const auto& [prio, queue] : std::as_const(*pfcPriorityToQueueMap)) {
      CHECK_GT(pfcPriorityToQueue.size(), static_cast<int>(prio))
          << " Policy: " << qosPolicy->getName()
          << " has pfcPriorityToQueueMap size " << pfcPriorityToQueue.size()
          << " which is smaller than pfc priority value " << prio;
      pfcPriorityToQueue[prio] = queue->toThrift();
    }
  }
  programPfcPriorityToQueueIfNeeded(pfcPriorityToQueue);
}

} // namespace facebook::fboss
