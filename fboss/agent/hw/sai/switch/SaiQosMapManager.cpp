/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

SaiQosMapManager::SaiQosMapManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiQosMap> SaiQosMapManager::setDscpToTcQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  const auto& newDscpMap = qosPolicy->getDscpMap();
  std::vector<sai_qos_map_t> mapToValueList;
  const auto& entries = newDscpMap->get<switch_state_tags::from>();
  mapToValueList.reserve(entries->size());
  for (const auto& entry : std::as_const(*entries)) {
    sai_qos_map_t mapping{};
    mapping.key.dscp = entry->cref<switch_state_tags::attr>()->cref();
    mapping.value.tc = entry->cref<switch_state_tags::trafficClass>()->cref();
    mapToValueList.push_back(mapping);
  }
  // set the dscp -> tc mapping in SAI
  SaiQosMapTraits::Attributes::Type typeAttribute{SAI_QOS_MAP_TYPE_DSCP_TO_TC};
  SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute{
      mapToValueList};
  auto& store = saiStore_->get<SaiQosMapTraits>();
  SaiQosMapTraits::AdapterHostKey k{typeAttribute, mapToValueListAttribute};
  SaiQosMapTraits::CreateAttributes c = k;
  return store.setObject(k, c);
}

std::shared_ptr<SaiQosMap> SaiQosMapManager::setExpToTcQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (!managerTable_->switchManager().isMplsQoSMapSupported()) {
    return nullptr;
  }
  std::vector<sai_qos_map_t> mapToValueList;
  const auto& newExpMap = qosPolicy->getExpMap();
  const auto& entries = newExpMap->cref<switch_state_tags::from>();
  mapToValueList.reserve(entries->size());
  for (const auto& entry : std::as_const(*entries)) {
    sai_qos_map_t mapping{};
    mapping.key.mpls_exp = entry->cref<switch_state_tags::attr>()->cref();
    mapping.value.tc = entry->cref<switch_state_tags::trafficClass>()->cref();
    mapToValueList.push_back(mapping);
  }
  // set the exp -> tc mapping in SAI
  SaiQosMapTraits::Attributes::Type typeAttribute{
      SAI_QOS_MAP_TYPE_MPLS_EXP_TO_TC};
  SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute{
      mapToValueList};
  auto& store = saiStore_->get<SaiQosMapTraits>();
  SaiQosMapTraits::AdapterHostKey k{typeAttribute, mapToValueListAttribute};
  SaiQosMapTraits::CreateAttributes c = k;
  return store.setObject(k, c);
}

std::shared_ptr<SaiQosMap> SaiQosMapManager::setTcToExpQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  if (!managerTable_->switchManager().isMplsQoSMapSupported()) {
    return nullptr;
  }
  std::vector<sai_qos_map_t> mapToValueList;
  const auto& newExpMap = qosPolicy->getExpMap();
  const auto& entries = newExpMap->cref<switch_state_tags::to>();
  mapToValueList.reserve(entries->size());
  for (const auto& entry : std::as_const(*entries)) {
    sai_qos_map_t mapping{};
    mapping.key.tc = entry->cref<switch_state_tags::trafficClass>()->cref();
    mapping.key.color = SAI_PACKET_COLOR_GREEN;
    mapping.value.mpls_exp = entry->cref<switch_state_tags::attr>()->cref();
    mapToValueList.push_back(mapping);
  }
  // set the tc -> exp mapping in SAI
  SaiQosMapTraits::Attributes::Type typeAttribute{
      SAI_QOS_MAP_TYPE_TC_AND_COLOR_TO_MPLS_EXP};
  SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute{
      mapToValueList};
  auto& store = saiStore_->get<SaiQosMapTraits>();
  SaiQosMapTraits::AdapterHostKey k{typeAttribute, mapToValueListAttribute};
  SaiQosMapTraits::CreateAttributes c = k;
  return store.setObject(k, c);
}

std::shared_ptr<SaiQosMap> SaiQosMapManager::setTcToQueueQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy,
    bool voq = false) {
  const auto& newTcToQueueIdMap = voq ? qosPolicy->getTrafficClassToVoqId()
                                      : qosPolicy->getTrafficClassToQueueId();
  std::vector<sai_qos_map_t> mapToValueList;
  mapToValueList.reserve(newTcToQueueIdMap->size());
  for (const auto& [tc, queue] : std::as_const(*newTcToQueueIdMap)) {
    const auto& q = queue->ref();
    sai_qos_map_t mapping{};
    mapping.key.tc = tc;
    mapping.value.queue_index = q;
    mapToValueList.push_back(mapping);
  }
  // set the tc->queue mapping in SAI
  SaiQosMapTraits::Attributes::Type typeAttribute{SAI_QOS_MAP_TYPE_TC_TO_QUEUE};
  SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute{
      mapToValueList};
  auto& store = saiStore_->get<SaiQosMapTraits>();
  SaiQosMapTraits::AdapterHostKey k{typeAttribute, mapToValueListAttribute};
  SaiQosMapTraits::CreateAttributes c = k;
  return store.setObject(k, c);
}

std::shared_ptr<SaiQosMap> SaiQosMapManager::setTcToPgQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  const auto& newTcToPgMap = qosPolicy->getTrafficClassToPgId();
  std::vector<sai_qos_map_t> mapToValueList;
  mapToValueList.reserve(newTcToPgMap->size());
  for (const auto& [tc, pg] : std::as_const(*newTcToPgMap)) {
    sai_qos_map_t mapping{};
    mapping.key.tc = tc;
    mapping.value.pg = pg->cref();
    mapToValueList.push_back(mapping);
  }
  // set the tc->pg mapping in SAI
  SaiQosMapTraits::Attributes::Type typeAttribute{
      SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP};
  SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute{
      mapToValueList};
  auto& store = saiStore_->get<SaiQosMapTraits>();
  SaiQosMapTraits::AdapterHostKey k{typeAttribute, mapToValueListAttribute};
  SaiQosMapTraits::CreateAttributes c = k;
  return store.setObject(k, c);
}

std::shared_ptr<SaiQosMap> SaiQosMapManager::setPfcPriorityToQueueQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy) {
  const auto& newPfcPriorityToQueueMap = qosPolicy->getPfcPriorityToQueueId();
  std::vector<sai_qos_map_t> mapToValueList;
  mapToValueList.reserve(newPfcPriorityToQueueMap->size());
  for (const auto& [pfcPriority, queue] :
       std::as_const(*newPfcPriorityToQueueMap)) {
    sai_qos_map_t mapping{};
    mapping.key.prio = pfcPriority;
    mapping.value.queue_index = queue->cref();
    mapToValueList.push_back(mapping);
  }
  // set the pfcPriority->queue mapping in SAI
  SaiQosMapTraits::Attributes::Type typeAttribute{
      SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE};
  SaiQosMapTraits::Attributes::MapToValueList mapToValueListAttribute{
      mapToValueList};
  auto& store = saiStore_->get<SaiQosMapTraits>();
  SaiQosMapTraits::AdapterHostKey k{typeAttribute, mapToValueListAttribute};
  SaiQosMapTraits::CreateAttributes c = k;
  return store.setObject(k, c);
}

void SaiQosMapManager::setQosMaps(
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  std::string qosPolicyName = newQosPolicy->getName();
  XLOG(DBG2) << "Setting QoS map: " << qosPolicyName;
  std::unique_ptr<SaiQosMapHandle> handle = std::make_unique<SaiQosMapHandle>();
  handle->dscpToTcMap = setDscpToTcQosMap(newQosPolicy);
  handle->tcToQueueMap = setTcToQueueQosMap(newQosPolicy);
  handle->expToTcMap = setExpToTcQosMap(newQosPolicy);
  handle->tcToExpMap = setTcToExpQosMap(newQosPolicy);
  if (platform_->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    if (newQosPolicy->getTrafficClassToPgId()) {
      handle->tcToPgMap = setTcToPgQosMap(newQosPolicy);
    }
    if (newQosPolicy->getPfcPriorityToQueueId()) {
      handle->pfcPriorityToQueueMap = setPfcPriorityToQueueQosMap(newQosPolicy);
    }
  }
  if (newQosPolicy->getTrafficClassToVoqId() &&
      !newQosPolicy->getTrafficClassToVoqId()->empty()) {
    handle->tcToVoqMap = setTcToQueueQosMap(newQosPolicy, true);
  }
  handles_[qosPolicyName] = std::move(handle);
}

void SaiQosMapManager::addQosMap(
    const std::shared_ptr<QosPolicy>& newQosPolicy,
    bool isDefault) {
  std::string qosPolicyName = newQosPolicy->getName();
  if (handles_.find(qosPolicyName) != handles_.end()) {
    throw FbossError("Failed to add QoS map: already programmed");
  }
  if (isDefault) {
    defaultQosPolicyName_ = qosPolicyName;
  }
  return setQosMaps(newQosPolicy);
}

void SaiQosMapManager::removeQosMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy) {
  std::string qosPolicyName = oldQosPolicy->getName();
  if (handles_.find(qosPolicyName) == handles_.end()) {
    throw FbossError("Failed to remove QoS map: none programmed");
  }
  handles_.erase(qosPolicyName);
}

void SaiQosMapManager::changeQosMap(
    const std::shared_ptr<QosPolicy>& oldQosPolicy,
    const std::shared_ptr<QosPolicy>& newQosPolicy) {
  std::string qosPolicyName = newQosPolicy->getName();
  if (handles_.find(qosPolicyName) == handles_.end()) {
    throw FbossError("Failed to change QoS map: none programmed");
  }
  return setQosMaps(newQosPolicy);
}

const SaiQosMapHandle* FOLLY_NULLABLE SaiQosMapManager::getQosMap(
    const std::optional<std::string>& qosPolicyName) const {
  return getQosMapImpl(qosPolicyName);
}
SaiQosMapHandle* FOLLY_NULLABLE
SaiQosMapManager::getQosMap(const std::optional<std::string>& qosPolicyName) {
  return getQosMapImpl(qosPolicyName);
}
SaiQosMapHandle* FOLLY_NULLABLE SaiQosMapManager::getQosMapImpl(
    const std::optional<std::string>& qosPolicyName) const {
  std::string name;
  if (!qosPolicyName) {
    name = defaultQosPolicyName_;
  } else {
    name = qosPolicyName.value();
  }
  if (handles_.find(name) == handles_.end()) {
    return nullptr;
  }
  return handles_.at(name).get();
}
} // namespace facebook::fboss
