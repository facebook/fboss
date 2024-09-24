/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiMirrorManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/MacAddress.h>
#include <chrono>
#include <cstdint>
#include <memory>

using namespace std::chrono;

namespace facebook::fboss {

sai_u32_range_t SaiAclTableManager::getFdbDstUserMetaDataRange() const {
  std::optional<SaiSwitchTraits::Attributes::FdbDstUserMetaDataRange> range =
      SaiSwitchTraits::Attributes::FdbDstUserMetaDataRange();
  return *(SaiApiTable::getInstance()->switchApi().getAttribute(
      managerTable_->switchManager().getSwitchSaiId(), range));
}

sai_u32_range_t SaiAclTableManager::getRouteDstUserMetaDataRange() const {
  std::optional<SaiSwitchTraits::Attributes::RouteDstUserMetaDataRange> range =
      SaiSwitchTraits::Attributes::RouteDstUserMetaDataRange();
  return *(SaiApiTable::getInstance()->switchApi().getAttribute(
      managerTable_->switchManager().getSwitchSaiId(), range));
}

sai_u32_range_t SaiAclTableManager::getNeighborDstUserMetaDataRange() const {
  std::optional<SaiSwitchTraits::Attributes::NeighborDstUserMetaDataRange>
      range = SaiSwitchTraits::Attributes::NeighborDstUserMetaDataRange();
  return *(SaiApiTable::getInstance()->switchApi().getAttribute(
      managerTable_->switchManager().getSwitchSaiId(), range));
}

sai_uint32_t SaiAclTableManager::getMetaDataMask(
    sai_uint32_t metaDataMax) const {
  /*
   * Round up to the next highest power of 2
   *
   * The idea is to set all the bits on the right hand side of the most
   * significant set bit to 1 and then increment the value by 1 so it 'rolls
   * over' to the nearest power of 2.
   *
   * Note, right shifting to power of 2 and OR'ing is enough - we don't need to
   * shift and OR by 3, 5, 6 etc. as shift + OR by (1, 2), (1, 4), (2, 4) etc.
   * already achieves that effect.
   *
   * Reference:
   * https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
   */

  // to handle the case when metaDataMax is already power of 2.
  metaDataMax--;
  std::array<int, 5> kNumBitsToShift = {1, 2, 4, 8, 16};
  for (auto numBitsToShift : kNumBitsToShift) {
    metaDataMax |= metaDataMax >> numBitsToShift;
  }
  metaDataMax++;

  // to handle the case when the passed metaDataMax is 0
  metaDataMax += (((metaDataMax == 0)) ? 1 : 0);

  return metaDataMax - 1;
}

std::vector<std::string> SaiAclTableManager::getAllHandleNames() const {
  std::vector<std::string> handleNames;
  for (const auto& [name, handle] : handles_) {
    handleNames.push_back(name);
  }
  return handleNames;
}

AclTableSaiId SaiAclTableManager::addAclTable(
    const std::shared_ptr<AclTable>& addedAclTable,
    cfg::AclStage aclStage) {
  auto saiAclStage =
      managerTable_->aclTableGroupManager().cfgAclStageToSaiAclStage(aclStage);

  /*
   * TODO(skhare)
   * Add single ACL Table for now (called during SaiSwitch::init()).
   * Later, extend SwitchState to carry AclTable, and then process it to
   * addAclTable.
   *
   * After ACL table is added, add it to appropriate ACL group:
   * managerTable_->switchManager().addTableGroupMember(SAI_ACL_STAGE_INGRESS,
   * aclTableSaiId);
   */

  // If we already store a handle for this this Acl Table, fail to add a new one
  auto aclTableName = addedAclTable->getID();
  auto handle = getAclTableHandle(aclTableName);
  if (handle) {
    throw FbossError("attempted to add a duplicate aclTable: ", aclTableName);
  }

  SaiAclTableTraits::AdapterHostKey adapterHostKey;
  SaiAclTableTraits::CreateAttributes attributes;

  std::tie(adapterHostKey, attributes) =
      aclTableCreateAttributes(saiAclStage, addedAclTable);

  auto& aclTableStore = saiStore_->get<SaiAclTableTraits>();
  std::shared_ptr<SaiAclTable> saiAclTable{};
  if (platform_->getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
    if (auto existingAclTable = aclTableStore.get(adapterHostKey)) {
      if (attributes != existingAclTable->attributes() ||
          FLAGS_force_recreate_acl_tables) {
        auto key = existingAclTable->adapterHostKey();
        auto attrs = existingAclTable->attributes();
        existingAclTable = aclTableStore.setObject(key, attrs);
        recreateAclTable(existingAclTable, attributes);
      }
      saiAclTable = std::move(existingAclTable);
    }
  }
  saiAclTable = aclTableStore.setObject(adapterHostKey, attributes);
  auto aclTableHandle = std::make_unique<SaiAclTableHandle>();
  aclTableHandle->aclTable = saiAclTable;
  auto [it, inserted] =
      handles_.emplace(aclTableName, std::move(aclTableHandle));
  CHECK(inserted);

  auto aclTableSaiId = it->second->aclTable->adapterKey();

  // Add ACL Table to group based on the stage
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ACL_TABLE_GROUP)) {
    managerTable_->aclTableGroupManager().addAclTableGroupMember(
        SAI_ACL_STAGE_INGRESS, aclTableSaiId, aclTableName);
  }

  return aclTableSaiId;
}

void SaiAclTableManager::removeAclTable(
    const std::shared_ptr<AclTable>& removedAclTable,
    cfg::AclStage /*aclStage*/) {
  auto aclTableName = removedAclTable->getID();

  // remove from acl table group
  if (hasTableGroups_) {
    managerTable_->aclTableGroupManager().removeAclTableGroupMember(
        SAI_ACL_STAGE_INGRESS, aclTableName);
  }

  // remove from handles
  handles_.erase(aclTableName);
}

bool SaiAclTableManager::needsAclTableRecreate(
    const std::shared_ptr<AclTable>& oldAclTable,
    const std::shared_ptr<AclTable>& newAclTable) {
  if (oldAclTable->getActionTypes() != newAclTable->getActionTypes() ||
      oldAclTable->getPriority() != newAclTable->getPriority() ||
      oldAclTable->getQualifiers() != newAclTable->getQualifiers()) {
    XLOG(DBG2) << "Recreating ACL table";
    return true;
  }
  return false;
}

void SaiAclTableManager::removeAclEntriesFromTable(
    const std::shared_ptr<AclTable>& aclTable) {
  auto aclMap = aclTable->getAclMap().unwrap();
  for (auto const& iter : std::as_const(*aclMap)) {
    const auto& entry = iter.second;
    auto aclEntry = aclMap->getEntry(entry->getID());
    removeAclEntry(aclEntry, aclTable->getID());
  }
}

void SaiAclTableManager::addAclEntriesToTable(
    const std::shared_ptr<AclTable>& aclTable,
    std::shared_ptr<AclMap>& aclMap) {
  for (auto const& iter : std::as_const(*aclMap)) {
    const auto& entry = iter.second;
    auto aclEntry = aclMap->getEntry(entry->getID());
    addAclEntry(aclEntry, aclTable->getID());
  }
}

void SaiAclTableManager::changedAclTable(
    const std::shared_ptr<AclTable>& oldAclTable,
    const std::shared_ptr<AclTable>& newAclTable,
    cfg::AclStage aclStage) {
  /*
   * If the only change in acl table is in acl entries, then the acl entry delta
   * processing will take care of changing those.
   * Changes to ACL table properties will need a remove and readd
   * Ensure that the newly added table also adds the old acls*/
  if (needsAclTableRecreate(oldAclTable, newAclTable)) {
    // Remove acl entries from old acl table before removing the table
    removeAclEntriesFromTable(oldAclTable);
    removeAclTable(oldAclTable, aclStage);
    addAclTable(newAclTable, aclStage);

    // Add the old acl Entries back to new acl table
    auto oldAclMap = oldAclTable->getAclMap().unwrap();
    addAclEntriesToTable(newAclTable, oldAclMap);
  }
}

const SaiAclTableHandle* FOLLY_NULLABLE
SaiAclTableManager::getAclTableHandle(const std::string& aclTableName) const {
  return getAclTableHandleImpl(aclTableName);
}

SaiAclTableHandle* FOLLY_NULLABLE
SaiAclTableManager::getAclTableHandle(const std::string& aclTableName) {
  return getAclTableHandleImpl(aclTableName);
}

SaiAclTableHandle* FOLLY_NULLABLE SaiAclTableManager::getAclTableHandleImpl(
    const std::string& aclTableName) const {
  auto itr = handles_.find(aclTableName);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->aclTable) {
    XLOG(FATAL) << "invalid null Acl table for: " << aclTableName;
  }
  return itr->second.get();
}

sai_acl_ip_frag_t SaiAclTableManager::cfgIpFragToSaiIpFrag(
    cfg::IpFragMatch cfgType) const {
  switch (cfgType) {
    case cfg::IpFragMatch::MATCH_NOT_FRAGMENTED:
      return SAI_ACL_IP_FRAG_NON_FRAG;
    case cfg::IpFragMatch::MATCH_FIRST_FRAGMENT:
      return SAI_ACL_IP_FRAG_HEAD;
    case cfg::IpFragMatch::MATCH_NOT_FRAGMENTED_OR_FIRST_FRAGMENT:
      return SAI_ACL_IP_FRAG_NON_FRAG_OR_HEAD;
    case cfg::IpFragMatch::MATCH_NOT_FIRST_FRAGMENT:
      return SAI_ACL_IP_FRAG_NON_HEAD;
    case cfg::IpFragMatch::MATCH_ANY_FRAGMENT:
      return SAI_ACL_IP_FRAG_ANY;
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP fragment option");
}

sai_acl_ip_type_t SaiAclTableManager::cfgIpTypeToSaiIpType(
    cfg::IpType cfgIpType) const {
  switch (cfgIpType) {
    case cfg::IpType::ANY:
      return SAI_ACL_IP_TYPE_ANY;
    case cfg::IpType::IP:
      return SAI_ACL_IP_TYPE_IP;
    case cfg::IpType::IP4:
      return SAI_ACL_IP_TYPE_IPV4ANY;
    case cfg::IpType::IP6:
      return SAI_ACL_IP_TYPE_IPV6ANY;
    case cfg::IpType::ARP_REQUEST:
      return SAI_ACL_IP_TYPE_ARP_REQUEST;
    case cfg::IpType::ARP_REPLY:
      return SAI_ACL_IP_TYPE_ARP_REPLY;
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP Type option");
}

uint16_t SaiAclTableManager::cfgEtherTypeToSaiEtherType(
    cfg::EtherType cfgEtherType) const {
  switch (cfgEtherType) {
    case cfg::EtherType::ANY:
    case cfg::EtherType::IPv4:
    case cfg::EtherType::IPv6:
    case cfg::EtherType::EAPOL:
    case cfg::EtherType::MACSEC:
    case cfg::EtherType::LLDP:
    case cfg::EtherType::ARP:
    case cfg::EtherType::LACP:
      return static_cast<uint16_t>(cfgEtherType);
  }
  // should return in one of the cases
  throw FbossError("Unsupported EtherType option");
}

sai_uint32_t SaiAclTableManager::cfgLookupClassToSaiMetaDataAndMaskHelper(
    cfg::AclLookupClass lookupClass,
    sai_uint32_t dstUserMetaDataRangeMin,
    sai_uint32_t dstUserMetaDataRangeMax) const {
  auto dstUserMetaData = static_cast<int>(lookupClass);
  if (dstUserMetaData < dstUserMetaDataRangeMin ||
      dstUserMetaData > dstUserMetaDataRangeMax) {
    throw FbossError(
        "attempted to configure dstUserMeta larger than supported by this ASIC",
        dstUserMetaData,
        " supported min: ",
        dstUserMetaDataRangeMin,
        " max: ",
        dstUserMetaDataRangeMax);
  }

  return dstUserMetaData;
}

std::pair<sai_uint32_t, sai_uint32_t>
SaiAclTableManager::cfgLookupClassToSaiFdbMetaDataAndMask(
    cfg::AclLookupClass lookupClass) const {
  if (platform_->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_TRIDENT2) {
    /*
     * lookupClassL2 is not configured on Trident2 or else the ASIC runs out
     * of resources. lookupClassL2 is needed for MH-NIC queue-per-host
     * solution. However, the solution is not applicable for Trident2 as we
     * don't implement queues on Trident2.
     */
    throw FbossError(
        "attempted to configure lookupClassL2 on Trident2, not needed/supported");
  }

  return std::make_pair(
      cfgLookupClassToSaiMetaDataAndMaskHelper(
          lookupClass,
          fdbDstUserMetaDataRangeMin_,
          fdbDstUserMetaDataRangeMax_),
      fdbDstUserMetaDataMask_);
}

std::pair<sai_uint32_t, sai_uint32_t>
SaiAclTableManager::cfgLookupClassToSaiRouteMetaDataAndMask(
    cfg::AclLookupClass lookupClass) const {
  return std::make_pair(
      cfgLookupClassToSaiMetaDataAndMaskHelper(
          lookupClass,
          routeDstUserMetaDataRangeMin_,
          routeDstUserMetaDataRangeMax_),
      routeDstUserMetaDataMask_);
}

std::pair<sai_uint32_t, sai_uint32_t>
SaiAclTableManager::cfgLookupClassToSaiNeighborMetaDataAndMask(
    cfg::AclLookupClass lookupClass) const {
  return std::make_pair(
      cfgLookupClassToSaiMetaDataAndMaskHelper(
          lookupClass,
          neighborDstUserMetaDataRangeMin_,
          neighborDstUserMetaDataRangeMax_),
      neighborDstUserMetaDataMask_);
}

std::vector<sai_int32_t>
SaiAclTableManager::cfgActionTypeListToSaiActionTypeList(
    const std::vector<cfg::AclTableActionType>& actionTypes) const {
  std::vector<sai_int32_t> saiActionTypeList;

  for (const auto& actionType : actionTypes) {
    sai_int32_t saiActionType;
    switch (actionType) {
      case cfg::AclTableActionType::PACKET_ACTION:
        saiActionType = SAI_ACL_ACTION_TYPE_PACKET_ACTION;
        break;
      case cfg::AclTableActionType::COUNTER:
        saiActionType = SAI_ACL_ACTION_TYPE_COUNTER;
        break;
      case cfg::AclTableActionType::SET_TC:
        saiActionType = SAI_ACL_ACTION_TYPE_SET_TC;
        break;
      case cfg::AclTableActionType::SET_DSCP:
        saiActionType = SAI_ACL_ACTION_TYPE_SET_DSCP;
        break;
      case cfg::AclTableActionType::MIRROR_INGRESS:
        saiActionType = SAI_ACL_ACTION_TYPE_MIRROR_INGRESS;
        break;
      case cfg::AclTableActionType::MIRROR_EGRESS:
        saiActionType = SAI_ACL_ACTION_TYPE_MIRROR_EGRESS;
        break;
      case cfg::AclTableActionType::SET_USER_DEFINED_TRAP:
        saiActionType = SAI_ACL_ACTION_TYPE_SET_USER_TRAP_ID;
        break;
      default:
        // should return in one of the cases
        throw FbossError("Unsupported Acl Table action type");
    }
    saiActionTypeList.push_back(saiActionType);
  }

  return saiActionTypeList;
}

bool SaiAclTableManager::isSameAclCounterAttributes(
    const SaiAclCounterTraits::CreateAttributes& fromStore,
    const SaiAclCounterTraits::CreateAttributes& fromSw) {
  bool result = GET_ATTR(AclCounter, TableId, fromStore) ==
          GET_ATTR(AclCounter, TableId, fromSw) &&
      GET_OPT_ATTR(AclCounter, EnablePacketCount, fromStore) ==
          GET_OPT_ATTR(AclCounter, EnablePacketCount, fromSw) &&
      GET_OPT_ATTR(AclCounter, EnableByteCount, fromStore) ==
          GET_OPT_ATTR(AclCounter, EnableByteCount, fromSw);

  if (platform_->getAsic()->isSupported(HwAsic::Feature::ACL_COUNTER_LABEL)) {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
    result = result &&
        GET_OPT_ATTR(AclCounter, Label, fromStore) ==
            GET_OPT_ATTR(AclCounter, Label, fromSw);
#endif
  }
  return result;
}

std::pair<
    std::shared_ptr<SaiAclCounter>,
    std::vector<std::pair<cfg::CounterType, std::string>>>
SaiAclTableManager::addAclCounter(
    const SaiAclTableHandle* aclTableHandle,
    const cfg::TrafficCounter& trafficCount,
    const SaiAclEntryTraits::AdapterHostKey& aclEntryAdapterHostKey) {
  std::vector<std::pair<cfg::CounterType, std::string>> aclCounterTypeAndName;

  SaiAclCounterTraits::Attributes::TableId aclTableId{
      aclTableHandle->aclTable->adapterKey()};

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
  SaiCharArray32 counterLabel{};
  if ((*trafficCount.name()).size() > 31) {
    throw FbossError(
        "ACL Counter Label:",
        *trafficCount.name(),
        " size ",
        (*trafficCount.name()).size(),
        " exceeds max(31)");
  }

  std::copy(
      (*trafficCount.name()).begin(),
      (*trafficCount.name()).end(),
      counterLabel.begin());

  std::optional<SaiAclCounterTraits::Attributes::Label> aclCounterLabel{
      std::nullopt};
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ACL_COUNTER_LABEL)) {
    aclCounterLabel = counterLabel;
  }
#endif

  std::optional<SaiAclCounterTraits::Attributes::EnablePacketCount>
      enablePacketCount{false};
  std::optional<SaiAclCounterTraits::Attributes::EnableByteCount>
      enableByteCount{false};

  for (const auto& counterType : *trafficCount.types()) {
    std::string statSuffix;
    switch (counterType) {
      case cfg::CounterType::PACKETS:
        enablePacketCount =
            SaiAclCounterTraits::Attributes::EnablePacketCount{true};
        statSuffix = "packets";
        break;
      case cfg::CounterType::BYTES:
        enableByteCount =
            SaiAclCounterTraits::Attributes::EnableByteCount{true};
        statSuffix = "bytes";
        break;
      default:
        throw FbossError("Unsupported CounterType for ACL");
    }

    auto statName =
        folly::to<std::string>(*trafficCount.name(), ".", statSuffix);
    aclCounterTypeAndName.push_back(std::make_pair(counterType, statName));
    if (aclCounterRefMap.find(statName) == aclCounterRefMap.end()) {
      // Create fb303 counter since stat is being added/readded again
      aclStats_.reinitStat(statName, std::nullopt);
      aclCounterRefMap[statName] = 1;
    } else {
      aclCounterRefMap[statName]++;
    }
  }

  SaiAclCounterTraits::AdapterHostKey adapterHostKey{
      aclTableId,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      aclCounterLabel,
#endif
      enablePacketCount,
      enableByteCount,
  };

  SaiAclCounterTraits::CreateAttributes attributes{
      aclTableId,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      aclCounterLabel,
#endif
      enablePacketCount,
      enableByteCount,
      std::nullopt, // counterPackets
      std::nullopt, // counterBytes
  };

  // The following logic is added temporarily for 5.1 -> 7.2 warmboot
  // transition. Sai spec 1.10.2 introduces label attributes for acl
  // counters, which requires agent detaching the old acl counter without
  // label attributes from the acl entry and then reattach the new acl
  // counter. In order to attach the new counter, agent needs to set counter
  // attribute to SAI_NULL_OBJECT_ID, and then attach the newly created
  // counter. Following is the logic to check if the attach/detach logic is
  // needed.
  // TODO(zecheng): remove the following logic when 7.2 upgrade completes.
  auto& aclCounterStore = saiStore_->get<SaiAclCounterTraits>();
  auto& aclEntryStore = saiStore_->get<SaiAclEntryTraits>();
  // If acl entry exists and already has different acl counter attached to
  // it, detach the old acl counter and then attach the new one.
  if (auto existingAclEntry = aclEntryStore.get(aclEntryAdapterHostKey)) {
    auto oldActionCounter = AclCounterSaiId{
        GET_OPT_ATTR(AclEntry, ActionCounter, existingAclEntry->attributes())
            .getData()};
    if (auto existingAclCounter = aclCounterStore.find(oldActionCounter)) {
      if (!isSameAclCounterAttributes(
              existingAclCounter->attributes(), attributes)) {
        // Detach the old one
        SaiAclEntryTraits::Attributes::ActionCounter actionCounterAttribute{
            SAI_NULL_OBJECT_ID};
        SaiApiTable::getInstance()->aclApi().setAttribute(
            existingAclEntry->adapterKey(), actionCounterAttribute);
      }
    }
  }
  auto saiAclCounter = aclCounterStore.setObject(adapterHostKey, attributes);

  return std::make_pair(saiAclCounter, aclCounterTypeAndName);
}

AclEntrySaiId SaiAclTableManager::addAclEntry(
    const std::shared_ptr<AclEntry>& addedAclEntry,
    const std::string& aclTableName) {
  // If we attempt to add entry to a table that does not exist, fail.
  auto aclTableHandle = getAclTableHandle(aclTableName);
  if (!aclTableHandle) {
    throw FbossError(
        "attempted to add AclEntry to a AclTable that does not exist: ",
        aclTableName);
  }

  // If we already store a handle for this this Acl Entry, fail to add new one.
  auto aclEntryHandle =
      getAclEntryHandle(aclTableHandle, addedAclEntry->getPriority());
  if (aclEntryHandle) {
    throw FbossError(
        "attempted to add a duplicate aclEntry: ", addedAclEntry->getID());
  }

  auto& aclEntryStore = saiStore_->get<SaiAclEntryTraits>();

  SaiAclEntryTraits::Attributes::TableId aclTableId{
      aclTableHandle->aclTable->adapterKey()};
  SaiAclEntryTraits::Attributes::Priority priority{
      swPriorityToSaiPriority(addedAclEntry->getPriority())};
  SaiAclEntryTraits::AdapterHostKey adapterHostKey{aclTableId, priority};

  std::optional<SaiAclEntryTraits::Attributes::FieldSrcIpV6> fieldSrcIpV6{
      std::nullopt};
  std::optional<SaiAclEntryTraits::Attributes::FieldSrcIpV4> fieldSrcIpV4{
      std::nullopt};
  if (addedAclEntry->getSrcIp().first) {
    if (addedAclEntry->getSrcIp().first.isV6()) {
      auto srcIpV6Mask = folly::IPAddressV6(
          folly::IPAddressV6::fetchMask(addedAclEntry->getSrcIp().second));
      fieldSrcIpV6 = SaiAclEntryTraits::Attributes::FieldSrcIpV6{
          AclEntryFieldIpV6(std::make_pair(
              addedAclEntry->getSrcIp().first.asV6(), srcIpV6Mask))};
    } else if (addedAclEntry->getSrcIp().first.isV4()) {
      auto srcIpV4Mask = folly::IPAddressV4(
          folly::IPAddressV4::fetchMask(addedAclEntry->getSrcIp().second));
      fieldSrcIpV4 = SaiAclEntryTraits::Attributes::FieldSrcIpV4{
          AclEntryFieldIpV4(std::make_pair(
              addedAclEntry->getSrcIp().first.asV4(), srcIpV4Mask))};
    }
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldDstIpV6> fieldDstIpV6{
      std::nullopt};
  std::optional<SaiAclEntryTraits::Attributes::FieldDstIpV4> fieldDstIpV4{
      std::nullopt};
  if (addedAclEntry->getDstIp().first) {
    if (addedAclEntry->getDstIp().first.isV6()) {
      auto dstIpV6Mask = folly::IPAddressV6(
          folly::IPAddressV6::fetchMask(addedAclEntry->getDstIp().second));
      fieldDstIpV6 = SaiAclEntryTraits::Attributes::FieldDstIpV6{
          AclEntryFieldIpV6(std::make_pair(
              addedAclEntry->getDstIp().first.asV6(), dstIpV6Mask))};
    } else if (addedAclEntry->getDstIp().first.isV4()) {
      auto dstIpV4Mask = folly::IPAddressV4(
          folly::IPAddressV4::fetchMask(addedAclEntry->getDstIp().second));
      fieldDstIpV4 = SaiAclEntryTraits::Attributes::FieldDstIpV4{
          AclEntryFieldIpV4(std::make_pair(
              addedAclEntry->getDstIp().first.asV4(), dstIpV4Mask))};
    }
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldSrcPort> fieldSrcPort{
      std::nullopt};
  if (addedAclEntry->getSrcPort()) {
    if (addedAclEntry->getSrcPort().value() !=
        cfg::switch_config_constants::CPU_PORT_LOGICALID()) {
      auto portHandle = managerTable_->portManager().getPortHandle(
          PortID(addedAclEntry->getSrcPort().value()));
      if (!portHandle) {
        throw FbossError(
            "attempted to configure srcPort: ",
            addedAclEntry->getSrcPort().value(),
            " ACL:",
            addedAclEntry->getID());
      }
      fieldSrcPort =
          SaiAclEntryTraits::Attributes::FieldSrcPort{AclEntryFieldSaiObjectIdT(
              std::make_pair(portHandle->port->adapterKey(), kMaskDontCare))};
    } else {
      PortSaiId portSaiId;

      if (platform_->getAsic()->isSupported(
              HwAsic::Feature::CPU_TX_VIA_RECYCLE_PORT)) {
        /*
         * For ASICs that implement CPU tx using recycle port, on tx:
         *  - FBOSS injects pkt from CPU.
         *  - SAI impl sends pkt to recycle port with pipeline bypass.
         *  - Packet is injected into pipeline with srcPort = recycle port.
         *
         *  Thus, if we want to program an ACL to trap such packets to CPU, we
         *  need to use recycle port as the src port qualifier as the packet is
         *  now "originating" from recycle port.
         */
        CHECK(managerTable_->switchManager().getCpuRecyclePort().has_value());
        portSaiId = managerTable_->switchManager().getCpuRecyclePort().value();
      } else {
        portSaiId = managerTable_->switchManager().getCpuPort();
      }

      fieldSrcPort = SaiAclEntryTraits::Attributes::FieldSrcPort{
          AclEntryFieldSaiObjectIdT(std::make_pair(portSaiId, kMaskDontCare))};
    }
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldOutPort> fieldOutPort{
      std::nullopt};
  if (addedAclEntry->getDstPort()) {
    auto portHandle = managerTable_->portManager().getPortHandle(
        PortID(addedAclEntry->getDstPort().value()));
    if (!portHandle) {
      throw FbossError(
          "attempted to configure dstPort: ",
          addedAclEntry->getDstPort().value(),
          " ACL:",
          addedAclEntry->getID());
    }
    fieldOutPort =
        SaiAclEntryTraits::Attributes::FieldOutPort{AclEntryFieldSaiObjectIdT(
            std::make_pair(portHandle->port->adapterKey(), kMaskDontCare))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldL4SrcPort> fieldL4SrcPort{
      std::nullopt};
  if (addedAclEntry->getL4SrcPort()) {
    fieldL4SrcPort = SaiAclEntryTraits::Attributes::FieldL4SrcPort{
        AclEntryFieldU16(std::make_pair(
            addedAclEntry->getL4SrcPort().value(), kL4PortMask))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldL4DstPort> fieldL4DstPort{
      std::nullopt};
  if (addedAclEntry->getL4DstPort()) {
    fieldL4DstPort = SaiAclEntryTraits::Attributes::FieldL4DstPort{
        AclEntryFieldU16(std::make_pair(
            addedAclEntry->getL4DstPort().value(), kL4PortMask))};
  }

  bool matchV4 = !addedAclEntry->getEtherType().has_value() ||
      addedAclEntry->getEtherType().value() == cfg::EtherType::IPv4;
#if !defined(TAJO_SDK) && !defined(BRCM_SAI_SDK_XGS)
  bool matchV6 = !addedAclEntry->getEtherType().has_value() ||
      addedAclEntry->getEtherType().value() == cfg::EtherType::IPv6;
#endif
  std::optional<SaiAclEntryTraits::Attributes::FieldIpProtocol> fieldIpProtocol{
      std::nullopt};
  auto qualifierSet = getSupportedQualifierSet();
  if (qualifierSet.find(cfg::AclTableQualifier::IP_PROTOCOL) !=
          qualifierSet.end() &&
      matchV4 && addedAclEntry->getProto()) {
    fieldIpProtocol = SaiAclEntryTraits::Attributes::FieldIpProtocol{
        AclEntryFieldU8(std::make_pair(
            addedAclEntry->getProto().value(), kIpProtocolMask))};
  }

#if !defined(TAJO_SDK) && !defined(BRCM_SAI_SDK_XGS)
  std::optional<SaiAclEntryTraits::Attributes::FieldIpv6NextHeader>
      fieldIpv6NextHeader{std::nullopt};
  if (qualifierSet.find(cfg::AclTableQualifier::IPV6_NEXT_HEADER) !=
          qualifierSet.end() &&
      matchV6 && addedAclEntry->getProto()) {
    fieldIpv6NextHeader = SaiAclEntryTraits::Attributes::FieldIpv6NextHeader{
        AclEntryFieldU8(std::make_pair(
            addedAclEntry->getProto().value(), kIpv6NextHeaderMask))};
  }
#endif

  std::optional<SaiAclEntryTraits::Attributes::FieldTcpFlags> fieldTcpFlags{
      std::nullopt};
  if (addedAclEntry->getTcpFlagsBitMap()) {
    fieldTcpFlags = SaiAclEntryTraits::Attributes::FieldTcpFlags{
        AclEntryFieldU8(std::make_pair(
            addedAclEntry->getTcpFlagsBitMap().value(), kTcpFlagsMask))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldIpFrag> fieldIpFrag{
      std::nullopt};
  if (addedAclEntry->getIpFrag()) {
    auto ipFragData = cfgIpFragToSaiIpFrag(addedAclEntry->getIpFrag().value());
    fieldIpFrag = SaiAclEntryTraits::Attributes::FieldIpFrag{
        AclEntryFieldU32(std::make_pair(ipFragData, kMaskDontCare))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldIcmpV4Type> fieldIcmpV4Type{
      std::nullopt};
  std::optional<SaiAclEntryTraits::Attributes::FieldIcmpV4Code> fieldIcmpV4Code{
      std::nullopt};
  std::optional<SaiAclEntryTraits::Attributes::FieldIcmpV6Type> fieldIcmpV6Type{
      std::nullopt};
  std::optional<SaiAclEntryTraits::Attributes::FieldIcmpV6Code> fieldIcmpV6Code{
      std::nullopt};
  if (addedAclEntry->getIcmpType()) {
    if (addedAclEntry->getProto()) {
      if (addedAclEntry->getProto().value() == AclEntry::kProtoIcmp) {
        fieldIcmpV4Type = SaiAclEntryTraits::Attributes::FieldIcmpV4Type{
            AclEntryFieldU8(std::make_pair(
                addedAclEntry->getIcmpType().value(), kIcmpTypeMask))};
        if (addedAclEntry->getIcmpCode()) {
          fieldIcmpV4Code = SaiAclEntryTraits::Attributes::FieldIcmpV4Code{
              AclEntryFieldU8(std::make_pair(
                  addedAclEntry->getIcmpCode().value(), kIcmpCodeMask))};
        }
      } else if (addedAclEntry->getProto().value() == AclEntry::kProtoIcmpv6) {
        fieldIcmpV6Type = SaiAclEntryTraits::Attributes::FieldIcmpV6Type{
            AclEntryFieldU8(std::make_pair(
                addedAclEntry->getIcmpType().value(), kIcmpTypeMask))};
        if (addedAclEntry->getIcmpCode()) {
          fieldIcmpV6Code = SaiAclEntryTraits::Attributes::FieldIcmpV6Code{
              AclEntryFieldU8(std::make_pair(
                  addedAclEntry->getIcmpCode().value(), kIcmpCodeMask))};
        }
      }
    }
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldDscp> fieldDscp{
      std::nullopt};
  if (addedAclEntry->getDscp()) {
    fieldDscp = SaiAclEntryTraits::Attributes::FieldDscp{AclEntryFieldU8(
        std::make_pair(addedAclEntry->getDscp().value(), kDscpMask))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldDstMac> fieldDstMac{
      std::nullopt};
  if (addedAclEntry->getDstMac()) {
    fieldDstMac = SaiAclEntryTraits::Attributes::FieldDstMac{AclEntryFieldMac(
        std::make_pair(addedAclEntry->getDstMac().value(), kMacMask()))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldIpType> fieldIpType{
      std::nullopt};
  if (addedAclEntry->getIpType()) {
    auto ipTypeData = cfgIpTypeToSaiIpType(addedAclEntry->getIpType().value());
    fieldIpType = SaiAclEntryTraits::Attributes::FieldIpType{
        AclEntryFieldU32(std::make_pair(ipTypeData, kMaskDontCare))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldTtl> fieldTtl{std::nullopt};
  if (addedAclEntry->getTtl()) {
    fieldTtl =
        SaiAclEntryTraits::Attributes::FieldTtl{AclEntryFieldU8(std::make_pair(
            addedAclEntry->getTtl().value().getValue(),
            addedAclEntry->getTtl().value().getMask()))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta>
      fieldRouteDstUserMeta{std::nullopt};
  if (addedAclEntry->getLookupClassRoute()) {
    fieldRouteDstUserMeta =
        SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta{
            AclEntryFieldU32(cfgLookupClassToSaiRouteMetaDataAndMask(
                addedAclEntry->getLookupClassRoute().value()))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta>
      fieldNeighborDstUserMeta{std::nullopt};
  if (addedAclEntry->getLookupClassNeighbor()) {
    fieldNeighborDstUserMeta =
        SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta{
            AclEntryFieldU32(cfgLookupClassToSaiNeighborMetaDataAndMask(
                addedAclEntry->getLookupClassNeighbor().value()))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldEthertype> fieldEtherType{
      std::nullopt};
  if (addedAclEntry->getEtherType()) {
    auto etherTypeData =
        cfgEtherTypeToSaiEtherType(addedAclEntry->getEtherType().value());
    fieldEtherType = SaiAclEntryTraits::Attributes::FieldEthertype{
        AclEntryFieldU16(std::make_pair(etherTypeData, kEtherTypeMask))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldOuterVlanId>
      fieldOuterVlanId{std::nullopt};
  if (addedAclEntry->getVlanID()) {
    fieldOuterVlanId = SaiAclEntryTraits::Attributes::FieldOuterVlanId{
        AclEntryFieldU16(std::make_pair(
            addedAclEntry->getVlanID().value(), kOuterVlanIdMask))};
  }

#if !defined(TAJO_SDK) || defined(TAJO_SDK_GTE_24_4_90)
  std::optional<SaiAclEntryTraits::Attributes::FieldBthOpcode> fieldBthOpcode{
      std::nullopt};
  if (addedAclEntry->getRoceOpcode()) {
    fieldBthOpcode = SaiAclEntryTraits::Attributes::FieldBthOpcode{
        AclEntryFieldU8(std::make_pair(
            addedAclEntry->getRoceOpcode().value(), kBthOpcodeMask))};
  }
#endif

  std::optional<SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta>
      fieldFdbDstUserMeta{std::nullopt};
  if (addedAclEntry->getLookupClassL2()) {
    fieldFdbDstUserMeta = SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta{
        AclEntryFieldU32(cfgLookupClassToSaiFdbMetaDataAndMask(
            addedAclEntry->getLookupClassL2().value()))};
  }

  // TODO(skhare) Support all other ACL actions
  std::optional<SaiAclEntryTraits::Attributes::ActionPacketAction>
      aclActionPacketAction{std::nullopt};
  const auto& act = addedAclEntry->getActionType();
  if (act == cfg::AclActionType::DENY) {
    aclActionPacketAction = SaiAclEntryTraits::Attributes::ActionPacketAction{
        SAI_PACKET_ACTION_DROP};
  } else {
    aclActionPacketAction = SaiAclEntryTraits::Attributes::ActionPacketAction{
        SAI_PACKET_ACTION_FORWARD};
  }

  std::shared_ptr<SaiAclCounter> saiAclCounter{nullptr};
  std::vector<std::pair<cfg::CounterType, std::string>> aclCounterTypeAndName;
  std::optional<SaiAclEntryTraits::Attributes::ActionCounter> aclActionCounter{
      std::nullopt};

  std::optional<SaiAclEntryTraits::Attributes::ActionSetTC> aclActionSetTC{
      std::nullopt};

  std::optional<SaiAclEntryTraits::Attributes::ActionSetDSCP> aclActionSetDSCP{
      std::nullopt};

  std::optional<SaiAclEntryTraits::Attributes::ActionMirrorIngress>
      aclActionMirrorIngress{};

  std::optional<SaiAclEntryTraits::Attributes::ActionMirrorEgress>
      aclActionMirrorEgress{};

  std::optional<std::string> ingressMirror{std::nullopt};
  std::optional<std::string> egressMirror{std::nullopt};
  std::shared_ptr<SaiHostifUserDefinedTrapHandle> userDefinedTrap{nullptr};

  std::optional<SaiAclEntryTraits::Attributes::ActionMacsecFlow>
      aclActionMacsecFlow{std::nullopt};

#if !defined(TAJO_SDK)
  std::optional<SaiAclEntryTraits::Attributes::ActionSetUserTrap>
      aclActionSetUserTrap{std::nullopt};
#endif

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  std::optional<SaiAclEntryTraits::Attributes::ActionDisableArsForwarding>
      aclActionDisableArsForwarding{std::nullopt};
#endif

  auto action = addedAclEntry->getAclAction();
  if (action) {
    // THRIFT_COPY
    auto matchAction = MatchAction::fromThrift(action->toThrift());
    if (matchAction.getTrafficCounter()) {
      std::tie(saiAclCounter, aclCounterTypeAndName) = addAclCounter(
          aclTableHandle,
          matchAction.getTrafficCounter().value(),
          adapterHostKey);
      aclActionCounter = SaiAclEntryTraits::Attributes::ActionCounter{
          AclEntryActionSaiObjectIdT(
              AclCounterSaiId{saiAclCounter->adapterKey()})};
    }
    // SaiAclTableManager assumes queueId is always equal to tcVal when
    // programming aclActionSetTC. We will remove this assumption by directly
    // using the tc value from setTcAction instead. Before changing prod
    // config to use setTc, we still need to keep sendToQueueAction logics
    // temporaily.
    std::optional<sai_uint8_t> tc = std::nullopt;
    std::optional<bool> sendToCpu = std::nullopt;
    if (matchAction.getSendToQueue()) {
      auto sendToQueue = matchAction.getSendToQueue().value();
      tc = static_cast<sai_uint8_t>(*sendToQueue.first.queueId());
      sendToCpu = sendToQueue.second;
    }
    if (matchAction.getSetTc()) {
      auto setTc = matchAction.getSetTc().value();
      tc = static_cast<sai_uint8_t>(*setTc.first.tcValue());
      if (sendToCpu != std::nullopt && sendToCpu != setTc.second) {
        throw FbossError(
            "Inconsistent actions between sendToQueue with sendToCpu ",
            (sendToCpu ? "true" : "false"),
            " and setTc with sendToCpu ",
            (setTc.second ? "true" : "false"));
      }
      sendToCpu = setTc.second;
    }

    auto setCopyOrTrap = [&aclActionPacketAction](
                             bool supportCopyToCpu,
                             const MatchAction& matchAction) {
      if (matchAction.getToCpuAction()) {
        switch (matchAction.getToCpuAction().value()) {
          case cfg::ToCpuAction::COPY:
            if (!supportCopyToCpu) {
              throw FbossError("COPY_TO_CPU is not supported on this ASIC");
            }
            aclActionPacketAction =
                SaiAclEntryTraits::Attributes::ActionPacketAction{
                    SAI_PACKET_ACTION_COPY};
            break;
          case cfg::ToCpuAction::TRAP:
            aclActionPacketAction =
                SaiAclEntryTraits::Attributes::ActionPacketAction{
                    SAI_PACKET_ACTION_TRAP};
            break;
        }
      }
    };

    if (tc != std::nullopt) {
      if (!sendToCpu) {
        aclActionSetTC = SaiAclEntryTraits::Attributes::ActionSetTC{
            AclEntryActionU8(tc.value())};
      } else {
        /*
         * When sendToCpu is set, a copy of the packet will be sent
         * to CPU.
         * By default, these packets are sent to queue 0.
         * Set TC to set the right traffic class which
         * will be mapped to queue id.
         *
         * TODO(skhare)
         * By default, BCM maps TC i to Queue i for i in [0, 9].
         * Tajo claims to map TC i to Queue i by default as well.
         * However, explicitly set the QoS Map and associate with the CPU port.
         */
        setCopyOrTrap(
            platform_->getAsic()->isSupported(HwAsic::Feature::ACL_COPY_TO_CPU),
            matchAction);
        aclActionSetTC = SaiAclEntryTraits::Attributes::ActionSetTC{
            AclEntryActionU8(tc.value())};
      }
    }

    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::SAI_USER_DEFINED_TRAP) &&
        FLAGS_sai_user_defined_trap) {
      // Some platform requires implementing ACL copy/trap to cpu action
      // along with user defined trap action mapping to the desrited cpu
      // queue, so as to make ACL rule take precedence over hostif trap
      // rule.
#if !defined(TAJO_SDK)
      if (matchAction.getUserDefinedTrap()) {
        auto queueId = *matchAction.getUserDefinedTrap().value().queueId();
        if (tc != std::nullopt && tc != queueId) {
          throw FbossError(
              "Inconsistent ACL action between set tc value",
              tc.value(),
              " and set user defined trap with queue id ",
              queueId);
        }
        userDefinedTrap =
            managerTable_->hostifManager().ensureHostifUserDefinedTrap(queueId);
        aclActionSetUserTrap = SaiAclEntryTraits::Attributes::ActionSetUserTrap{
            AclEntryActionSaiObjectIdT(userDefinedTrap->trap->adapterKey())};
        setCopyOrTrap(
            platform_->getAsic()->isSupported(HwAsic::Feature::ACL_COPY_TO_CPU),
            matchAction);
      }
#endif
    }

    if (matchAction.getIngressMirror().has_value()) {
      std::vector<sai_object_id_t> aclEntryMirrorIngressOidList;
      auto mirrorHandle = managerTable_->mirrorManager().getMirrorHandle(
          matchAction.getIngressMirror().value());
      if (mirrorHandle) {
        aclEntryMirrorIngressOidList.push_back(mirrorHandle->adapterKey());
      }
      ingressMirror = matchAction.getIngressMirror().value();
      aclActionMirrorIngress =
          SaiAclEntryTraits::Attributes::ActionMirrorIngress{
              AclEntryActionSaiObjectIdList(aclEntryMirrorIngressOidList)};
    }

    if (matchAction.getEgressMirror().has_value()) {
      std::vector<sai_object_id_t> aclEntryMirrorEgressOidList;
      auto mirrorHandle = managerTable_->mirrorManager().getMirrorHandle(
          matchAction.getEgressMirror().value());
      if (mirrorHandle) {
        aclEntryMirrorEgressOidList.push_back(mirrorHandle->adapterKey());
      }
      egressMirror = matchAction.getEgressMirror().value();
      aclActionMirrorEgress = SaiAclEntryTraits::Attributes::ActionMirrorEgress{
          AclEntryActionSaiObjectIdList(aclEntryMirrorEgressOidList)};
    }

    if (matchAction.getSetDscp()) {
      const int dscpValue = *matchAction.getSetDscp().value().dscpValue();

      aclActionSetDSCP = SaiAclEntryTraits::Attributes::ActionSetDSCP{
          AclEntryActionU8(dscpValue)};
    }

    if (matchAction.getMacsecFlow()) {
      auto macsecFlowAction = matchAction.getMacsecFlow().value();
      if (*macsecFlowAction.action() ==
          cfg::MacsecFlowPacketAction::MACSEC_FLOW) {
        sai_object_id_t flowId =
            static_cast<sai_object_id_t>(*macsecFlowAction.flowId());
        aclActionMacsecFlow = SaiAclEntryTraits::Attributes::ActionMacsecFlow{
            AclEntryActionSaiObjectIdT(flowId)};
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
        // MACSec flow action should not be set along with regulation packet
        // action otherwise packet action gets priority
        aclActionPacketAction = std::nullopt;
#endif
      } else if (
          *macsecFlowAction.action() == cfg::MacsecFlowPacketAction::FORWARD) {
        aclActionPacketAction =
            SaiAclEntryTraits::Attributes::ActionPacketAction{
                SAI_PACKET_ACTION_FORWARD};
      } else if (
          *macsecFlowAction.action() == cfg::MacsecFlowPacketAction::DROP) {
        aclActionPacketAction =
            SaiAclEntryTraits::Attributes::ActionPacketAction{
                SAI_PACKET_ACTION_DROP};
      } else {
        throw FbossError(
            "Unsupported Macsec Flow action for ACL entry: ",
            addedAclEntry->getID(),
            " Macsec Flow action ",
            apache::thrift::util::enumNameSafe(*macsecFlowAction.action()));
      }
    }
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    if (FLAGS_flowletSwitchingEnable &&
        platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
      if (matchAction.getFlowletAction().has_value()) {
        auto flowletAction = matchAction.getFlowletAction().value();
        switch (flowletAction) {
          case cfg::FlowletAction::FORWARD:
            aclActionDisableArsForwarding =
                SaiAclEntryTraits::Attributes::ActionDisableArsForwarding{
                    false};
            break;
        }
      }
    }
#endif
  }

  // TODO(skhare) At least one field and one action must be specified.
  // Once we add support for all fields and actions, throw error if that is not
  // honored.
  auto matcherIsValid =
      (fieldSrcIpV6.has_value() || fieldDstIpV6.has_value() ||
       fieldSrcIpV4.has_value() || fieldDstIpV4.has_value() ||
       fieldSrcPort.has_value() || fieldOutPort.has_value() ||
       fieldL4SrcPort.has_value() || fieldL4DstPort.has_value() ||
       fieldIpProtocol.has_value() || fieldTcpFlags.has_value() ||
       fieldIpFrag.has_value() || fieldIcmpV4Type.has_value() ||
       fieldIcmpV4Code.has_value() || fieldIcmpV6Type.has_value() ||
       fieldIcmpV6Code.has_value() || fieldDscp.has_value() ||
       fieldDstMac.has_value() || fieldIpType.has_value() ||
       fieldTtl.has_value() || fieldFdbDstUserMeta.has_value() ||
       fieldRouteDstUserMeta.has_value() || fieldEtherType.has_value() ||
       fieldNeighborDstUserMeta.has_value() || fieldOuterVlanId.has_value() ||
#if !defined(TAJO_SDK) || defined(TAJO_SDK_GTE_24_4_90)
       fieldBthOpcode.has_value() ||
#endif
#if !defined(TAJO_SDK) && !defined(BRCM_SAI_SDK_XGS)
       fieldIpv6NextHeader.has_value() ||
#endif
       platform_->getAsic()->isSupported(HwAsic::Feature::EMPTY_ACL_MATCHER));
  if (fieldSrcPort.has_value()) {
    auto srcPortQualifierSupported = platform_->getAsic()->isSupported(
        HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER);
#if defined(TAJO_SDK_VERSION_1_42_8)
    srcPortQualifierSupported = false;
#endif
    matcherIsValid &= srcPortQualifierSupported;
  }
  auto actionIsValid =
      (aclActionPacketAction.has_value() || aclActionCounter.has_value() ||
       aclActionSetTC.has_value() || aclActionSetDSCP.has_value() ||
       aclActionMirrorIngress.has_value() ||
       aclActionMirrorEgress.has_value() || aclActionMacsecFlow.has_value()
#if !defined(TAJO_SDK)
       || aclActionSetUserTrap.has_value()
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
       || aclActionDisableArsForwarding.has_value()
#endif
      );

  if (!(matcherIsValid && actionIsValid)) {
    XLOG(WARNING) << "Unsupported field/action for aclEntry: "
                  << addedAclEntry->getID() << " MactherValid "
                  << ((matcherIsValid) ? "true" : "false") << " ActionValid "
                  << ((actionIsValid) ? "true" : "false");
    return AclEntrySaiId{0};
  }

  SaiAclEntryTraits::CreateAttributes attributes{
      aclTableId,
      priority,
      true,
      fieldSrcIpV6,
      fieldDstIpV6,
      fieldSrcIpV4,
      fieldDstIpV4,
      fieldSrcPort,
      fieldOutPort,
      fieldL4SrcPort,
      fieldL4DstPort,
      fieldIpProtocol,
      fieldTcpFlags,
      fieldIpFrag,
      fieldIcmpV4Type,
      fieldIcmpV4Code,
      fieldIcmpV6Type,
      fieldIcmpV6Code,
      fieldDscp,
      fieldDstMac,
      fieldIpType,
      fieldTtl,
      fieldFdbDstUserMeta,
      fieldRouteDstUserMeta,
      fieldNeighborDstUserMeta,
      fieldEtherType,
      fieldOuterVlanId,
#if !defined(TAJO_SDK) || defined(TAJO_SDK_GTE_24_4_90)
      fieldBthOpcode,
#endif
#if !defined(TAJO_SDK) && !defined(BRCM_SAI_SDK_XGS)
      fieldIpv6NextHeader,
#endif
      aclActionPacketAction,
      aclActionCounter,
      aclActionSetTC,
      aclActionSetDSCP,
      aclActionMirrorIngress,
      aclActionMirrorEgress,
      aclActionMacsecFlow,
// action not supported by tajo. Besides, user defined trap
// is used to make ACL take precedence over Hostif trap.
// Tajo already supports this behavior
#if !defined(TAJO_SDK)
      aclActionSetUserTrap,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      aclActionDisableArsForwarding,
#endif
  };

  auto saiAclEntry = aclEntryStore.setObject(adapterHostKey, attributes);
  auto entryHandle = std::make_unique<SaiAclEntryHandle>();
  entryHandle->aclEntry = saiAclEntry;
  entryHandle->aclCounter = saiAclCounter;
  entryHandle->aclCounterTypeAndName = aclCounterTypeAndName;
  entryHandle->ingressMirror = ingressMirror;
  entryHandle->egressMirror = egressMirror;
  entryHandle->userDefinedTrap = userDefinedTrap;
  auto [it, inserted] = aclTableHandle->aclTableMembers.emplace(
      addedAclEntry->getPriority(), std::move(entryHandle));
  CHECK(inserted);

  XLOG(DBG2) << "added acl entry " << addedAclEntry->getID() << " priority "
             << addedAclEntry->getPriority();

  auto enabled = SaiApiTable::getInstance()->aclApi().getAttribute(
      it->second->aclEntry->adapterKey(),
      SaiAclEntryTraits::Attributes::Enabled{});
  CHECK(enabled) << "Acl entry: " << addedAclEntry->getID() << " not enabled";
  return it->second->aclEntry->adapterKey();
}

void SaiAclTableManager::removeAclEntry(
    const std::shared_ptr<AclEntry>& removedAclEntry,
    const std::string& aclTableName) {
  // If we attempt to remove entry for a table that does not exist, fail.
  auto aclTableHandle = getAclTableHandle(aclTableName);
  if (!aclTableHandle) {
    throw FbossError(
        "attempted to remove AclEntry to a AclTable that does not exist: ",
        aclTableName);
  }

  // If we attempt to remove entry that does not exist, fail.
  auto itr =
      aclTableHandle->aclTableMembers.find(removedAclEntry->getPriority());
  if (itr == aclTableHandle->aclTableMembers.end()) {
    // an acl entry that uses cpu port as qualifier may not have been created
    // even if it exists in switch state.
    XLOG(ERR) << "attempted to remove aclEntry which does not exist: ",
        removedAclEntry->getID();
    return;
  }

  aclTableHandle->aclTableMembers.erase(itr);

  auto action = removedAclEntry->getAclAction();
  if (action && action->cref<switch_state_tags::trafficCounter>()) {
    // THRIFT_COPY
    removeAclCounter(
        action->cref<switch_state_tags::trafficCounter>()->toThrift());
  }
  XLOG(DBG2) << "removed acl  entry " << removedAclEntry->getID()
             << " priority " << removedAclEntry->getPriority();
}

void SaiAclTableManager::removeAclCounter(
    const cfg::TrafficCounter& trafficCount) {
  for (const auto& counterType : *trafficCount.types()) {
    auto statName =
        utility::statNameFromCounterType(*trafficCount.name(), counterType);
    auto entry = aclCounterRefMap.find(statName);
    if (entry != aclCounterRefMap.end()) {
      entry->second--;
      if (entry->second == 0) {
        // Counter no longer used. Remove from fb303 counters
        aclStats_.removeStat(statName);
        aclCounterRefMap.erase(entry);
      }
    } else {
      throw FbossError("Acl counter ", statName, " not found om counter map");
    }
  }
}

void SaiAclTableManager::changedAclEntry(
    const std::shared_ptr<AclEntry>& oldAclEntry,
    const std::shared_ptr<AclEntry>& newAclEntry,
    const std::string& aclTableName) {
  /*
   * ASIC/SAI implementation typically does not allow modifying an ACL entry.
   * Thus, remove and re-add.
   */
  XLOG(DBG2) << "changing acl entry " << oldAclEntry->getID();
  removeAclEntry(oldAclEntry, aclTableName);
  addAclEntry(newAclEntry, aclTableName);
}

const SaiAclEntryHandle* FOLLY_NULLABLE SaiAclTableManager::getAclEntryHandle(
    const SaiAclTableHandle* aclTableHandle,
    int priority) const {
  auto itr = aclTableHandle->aclTableMembers.find(priority);
  if (itr == aclTableHandle->aclTableMembers.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->aclEntry) {
    XLOG(FATAL) << "invalid null Acl entry for: " << priority;
  }
  return itr->second.get();
}

void SaiAclTableManager::programMirror(
    const SaiAclEntryHandle* aclEntryHandle,
    MirrorDirection direction,
    MirrorAction action,
    const std::optional<std::string>& mirrorId) {
  if (!mirrorId.has_value()) {
    XLOG(DBG) << "mirror session not configured: ";
    return;
  }

  std::vector<sai_object_id_t> mirrorOidList{};
  if (action == MirrorAction::START) {
    auto mirrorHandle =
        managerTable_->mirrorManager().getMirrorHandle(mirrorId.value());
    if (mirrorHandle) {
      mirrorOidList.push_back(mirrorHandle->adapterKey());
    }
  }

  if (direction == MirrorDirection::INGRESS) {
    aclEntryHandle->aclEntry->setOptionalAttribute(
        SaiAclEntryTraits::Attributes::ActionMirrorIngress{mirrorOidList});
  } else {
    aclEntryHandle->aclEntry->setOptionalAttribute(
        SaiAclEntryTraits::Attributes::ActionMirrorEgress{mirrorOidList});
  }
}

void SaiAclTableManager::programMirrorOnAllAcls(
    const std::optional<std::string>& mirrorId,
    MirrorAction action) {
  for (const auto& handle : handles_) {
    for (const auto& aclMember : handle.second->aclTableMembers) {
      if (aclMember.second->getIngressMirror() == mirrorId) {
        programMirror(
            aclMember.second.get(), MirrorDirection::INGRESS, action, mirrorId);
      }
      if (aclMember.second->getEgressMirror() == mirrorId) {
        programMirror(
            aclMember.second.get(), MirrorDirection::EGRESS, action, mirrorId);
      }
    }
  }
}

void SaiAclTableManager::removeUnclaimedAclEntries() {
  auto& aclEntryStore = saiStore_->get<SaiAclEntryTraits>();
  aclEntryStore.removeUnexpectedUnclaimedWarmbootHandles();
}

void SaiAclTableManager::updateStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());

  for (const auto& handle : handles_) {
    for (const auto& aclMember : handle.second->aclTableMembers) {
      for (const auto& [counterType, counterName] :
           aclMember.second->aclCounterTypeAndName) {
        switch (counterType) {
          case cfg::CounterType::PACKETS: {
            auto counterPackets =
                SaiApiTable::getInstance()->aclApi().getAttribute(
                    aclMember.second->aclCounter->adapterKey(),
                    SaiAclCounterTraits::Attributes::CounterPackets());
            aclStats_.updateStat(now, counterName, counterPackets);
          } break;
          case cfg::CounterType::BYTES: {
            auto counterBytes =
                SaiApiTable::getInstance()->aclApi().getAttribute(
                    aclMember.second->aclCounter->adapterKey(),
                    SaiAclCounterTraits::Attributes::CounterBytes());
            aclStats_.updateStat(now, counterName, counterBytes);
          } break;
          default:
            throw FbossError("Unsupported CounterType for ACL");
        }
      }
    }
  }
}

// Loop through all the ACL tables and compute the free entries and counters
std::pair<int32_t, int32_t> SaiAclTableManager::getAclResourceUsage() {
  int32_t aclEntriesFree = 0, aclCountersFree = 0;
  for (const auto& handle : handles_) {
    auto aclTableHandle = handle.second.get();
    auto aclTableId = aclTableHandle->aclTable->adapterKey();
    auto& aclApi = SaiApiTable::getInstance()->aclApi();

    aclEntriesFree += aclApi.getAttribute(
        aclTableId, SaiAclTableTraits::Attributes::AvailableEntry{});
    aclCountersFree += aclApi.getAttribute(
        aclTableId, SaiAclTableTraits::Attributes::AvailableCounter{});
  }
  return std::make_pair(aclEntriesFree, aclCountersFree);
}

std::set<cfg::AclTableQualifier> SaiAclTableManager::getSupportedQualifierSet()
    const {
  /*
   * Not all the qualifiers are supported by every ASIC.
   * Moreover, different ASICs have different max key widths.
   * Thus, enabling all the supported qualifiers in the same ACL Table could
   * overflow the max key width.
   *
   * Thus, only enable a susbet of supported qualifiers based on the ASIC
   * capability.
   */
  bool isTajo = platform_->getAsic()->getAsicVendor() ==
      HwAsic::AsicVendor::ASIC_VENDOR_TAJO;
  bool isTrident2 =
      platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_TRIDENT2;
  bool isJericho2 =
      platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2;
  bool isJericho3 =
      platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3;
  bool isTomahawk5 =
      platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK5;

  if (isTajo) {
    std::set<cfg::AclTableQualifier> tajoQualifiers = {
        cfg::AclTableQualifier::SRC_IPV6,
        cfg::AclTableQualifier::DST_IPV6,
        cfg::AclTableQualifier::SRC_IPV4,
        cfg::AclTableQualifier::DST_IPV4,
        cfg::AclTableQualifier::IP_PROTOCOL,
        cfg::AclTableQualifier::DSCP,
        cfg::AclTableQualifier::IP_TYPE,
        cfg::AclTableQualifier::TTL,
        cfg::AclTableQualifier::LOOKUP_CLASS_L2,
        cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
        cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};

#if defined(TAJO_SDK_GTE_24_4_90)
    std::vector<cfg::AclTableQualifier> tajoExtraQualifierList = {
        cfg::AclTableQualifier::ETHER_TYPE,
        cfg::AclTableQualifier::BTH_OPCODE,
        cfg::AclTableQualifier::SRC_PORT,
        cfg::AclTableQualifier::L4_SRC_PORT,
        cfg::AclTableQualifier::L4_DST_PORT,
        cfg::AclTableQualifier::TCP_FLAGS,
        cfg::AclTableQualifier::IP_FRAG,
        cfg::AclTableQualifier::ICMPV4_TYPE,
        cfg::AclTableQualifier::ICMPV4_CODE,
        cfg::AclTableQualifier::ICMPV6_TYPE,
        cfg::AclTableQualifier::ICMPV6_CODE,
        cfg::AclTableQualifier::DST_MAC,
    };
    for (const auto& qualifier : tajoExtraQualifierList) {
      tajoQualifiers.insert(qualifier);
    }
#endif
    return tajoQualifiers;
  } else if (isJericho2) {
    // TODO(skhare)
    // Extend this list once the SAI implementation supports more qualifiers
    std::set<cfg::AclTableQualifier> jericho2Qualifiers = {
        cfg::AclTableQualifier::SRC_IPV6,
        cfg::AclTableQualifier::DST_IPV6,
        cfg::AclTableQualifier::SRC_PORT,
        cfg::AclTableQualifier::DSCP,
        cfg::AclTableQualifier::IP_PROTOCOL,
        cfg::AclTableQualifier::IP_TYPE,
        cfg::AclTableQualifier::TTL,
    };
    return jericho2Qualifiers;
  } else if (isJericho3) {
    std::set<cfg::AclTableQualifier> jericho3Qualifiers = {
        cfg::AclTableQualifier::ETHER_TYPE,
        cfg::AclTableQualifier::SRC_IPV6,
        cfg::AclTableQualifier::DST_IPV6,
        cfg::AclTableQualifier::SRC_IPV4,
        cfg::AclTableQualifier::DST_IPV4,
        cfg::AclTableQualifier::SRC_PORT,
        cfg::AclTableQualifier::DSCP,
        cfg::AclTableQualifier::TTL,
        cfg::AclTableQualifier::IP_PROTOCOL,
        cfg::AclTableQualifier::IPV6_NEXT_HEADER,
        cfg::AclTableQualifier::IP_TYPE,
        cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
        cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE,
        cfg::AclTableQualifier::L4_SRC_PORT,
        cfg::AclTableQualifier::L4_DST_PORT,
        cfg::AclTableQualifier::ICMPV4_TYPE,
        cfg::AclTableQualifier::ICMPV4_CODE,
        cfg::AclTableQualifier::ICMPV6_TYPE,
        cfg::AclTableQualifier::ICMPV6_CODE,
        cfg::AclTableQualifier::DST_MAC,
        cfg::AclTableQualifier::BTH_OPCODE};
    return jericho3Qualifiers;
  } else {
    std::set<cfg::AclTableQualifier> bcmQualifiers = {
        cfg::AclTableQualifier::SRC_IPV6,
        cfg::AclTableQualifier::DST_IPV6,
        cfg::AclTableQualifier::SRC_IPV4,
        cfg::AclTableQualifier::DST_IPV4,
        cfg::AclTableQualifier::L4_SRC_PORT,
        cfg::AclTableQualifier::L4_DST_PORT,
        cfg::AclTableQualifier::IP_PROTOCOL,
        cfg::AclTableQualifier::TCP_FLAGS,
        cfg::AclTableQualifier::SRC_PORT,
        cfg::AclTableQualifier::OUT_PORT,
        cfg::AclTableQualifier::IP_FRAG,
        cfg::AclTableQualifier::ICMPV4_TYPE,
        cfg::AclTableQualifier::ICMPV4_CODE,
        cfg::AclTableQualifier::ICMPV6_TYPE,
        cfg::AclTableQualifier::ICMPV6_CODE,
        cfg::AclTableQualifier::DSCP,
        cfg::AclTableQualifier::DST_MAC,
        cfg::AclTableQualifier::IP_TYPE,
        cfg::AclTableQualifier::TTL,
        cfg::AclTableQualifier::LOOKUP_CLASS_L2,
        cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
        cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};

    /*
     * FdbDstUserMetaData is required only for MH-NIC queue-per-host solution.
     * However, the solution is not applicable for Trident2 as FBOSS does not
     * implement queues on Trident2.
     * Furthermore, Trident2 supports fewer ACL qualifiers than other
     * hardwares. Thus, avoid programming unncessary qualifiers (or else we
     * run out resources).
     */
    if (isTrident2) {
      bcmQualifiers.erase(cfg::AclTableQualifier::LOOKUP_CLASS_L2);
    }
    // TH5 fails creating ACL table after adding vlan as qualifier with 10.0
    // CS00012342272
    if (isTomahawk5) {
      bcmQualifiers.erase(cfg::AclTableQualifier::OUTER_VLAN);
    }

    return bcmQualifiers;
  }
}

void SaiAclTableManager::addDefaultAclTable() {
  if (handles_.find(kAclTable1) != handles_.end()) {
    throw FbossError("default acl table already exists.");
  }
  // TODO(saranicholas): set appropriate table priority
  state::AclTableFields aclTableFields{};
  aclTableFields.priority() = 0;
  aclTableFields.id() = kAclTable1;
  auto table1 = std::make_shared<AclTable>(std::move(aclTableFields));
  addAclTable(table1, cfg::AclStage::INGRESS);
}

void SaiAclTableManager::removeDefaultAclTable() {
  if (handles_.find(kAclTable1) == handles_.end()) {
    return;
  }
  // remove from acl table group
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ACL_TABLE_GROUP)) {
    managerTable_->aclTableGroupManager().removeAclTableGroupMember(
        SAI_ACL_STAGE_INGRESS, kAclTable1);
  }
  handles_.erase(kAclTable1);
}

bool SaiAclTableManager::isQualifierSupported(
    const std::string& aclTableName,
    cfg::AclTableQualifier qualifier) const {
  auto handle = getAclTableHandle(aclTableName);
  if (!handle) {
    throw FbossError("ACL table ", aclTableName, " not found.");
  }
  auto attributes = handle->aclTable->attributes();

  auto hasField = [attributes](auto field) {
    if (!field) {
      return false;
    }
    return field->value();
  };

  using cfg::AclTableQualifier;
  switch (qualifier) {
    case cfg::AclTableQualifier::SRC_IPV6:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldSrcIpV6>>(
              attributes));
    case cfg::AclTableQualifier::DST_IPV6:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldDstIpV6>>(
              attributes));
    case cfg::AclTableQualifier::SRC_IPV4:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldDstIpV4>>(
              attributes));
    case cfg::AclTableQualifier::DST_IPV4:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldSrcIpV4>>(
              attributes));
    case cfg::AclTableQualifier::L4_SRC_PORT:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldL4SrcPort>>(
              attributes));
    case cfg::AclTableQualifier::L4_DST_PORT:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldL4DstPort>>(
              attributes));
    case cfg::AclTableQualifier::IP_PROTOCOL:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldIpProtocol>>(
              attributes));
    case cfg::AclTableQualifier::TCP_FLAGS:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldTcpFlags>>(
              attributes));
    case cfg::AclTableQualifier::SRC_PORT:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldSrcPort>>(
              attributes));
    case cfg::AclTableQualifier::OUT_PORT:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldOutPort>>(
              attributes));
    case cfg::AclTableQualifier::IP_FRAG:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldIpFrag>>(
              attributes));
    case cfg::AclTableQualifier::ICMPV4_TYPE:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldIcmpV4Type>>(
              attributes));
    case cfg::AclTableQualifier::ICMPV4_CODE:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldIcmpV4Code>>(
              attributes));
    case cfg::AclTableQualifier::ICMPV6_TYPE:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldIcmpV6Type>>(
              attributes));
    case cfg::AclTableQualifier::ICMPV6_CODE:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldIcmpV6Code>>(
              attributes));
    case cfg::AclTableQualifier::DSCP:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldDscp>>(
              attributes));
    case cfg::AclTableQualifier::DST_MAC:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldDstMac>>(
              attributes));
    case cfg::AclTableQualifier::IP_TYPE:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldIpType>>(
              attributes));
    case cfg::AclTableQualifier::TTL:
      return hasField(
          std::get<std::optional<SaiAclTableTraits::Attributes::FieldTtl>>(
              attributes));
    case cfg::AclTableQualifier::LOOKUP_CLASS_L2:
      return hasField(
          std::get<std::optional<
              SaiAclTableTraits::Attributes::FieldFdbDstUserMeta>>(attributes));
    case cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR:
      return hasField(
          std::get<std::optional<
              SaiAclTableTraits::Attributes::FieldRouteDstUserMeta>>(
              attributes));
    case cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE:
      return hasField(
          std::get<std::optional<
              SaiAclTableTraits::Attributes::FieldRouteDstUserMeta>>(
              attributes));
    case cfg::AclTableQualifier::ETHER_TYPE:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldEthertype>>(
              attributes));
    case cfg::AclTableQualifier::OUTER_VLAN:
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldOuterVlanId>>(
              attributes));

    case cfg::AclTableQualifier::BTH_OPCODE:
#if !defined(TAJO_SDK) || defined(TAJO_SDK_GTE_24_4_90)
      return hasField(
          std::get<
              std::optional<SaiAclTableTraits::Attributes::FieldBthOpcode>>(
              attributes));
#else
      return false;
#endif
    case cfg::AclTableQualifier::IPV6_NEXT_HEADER:
#if !defined(TAJO_SDK) && !defined(BRCM_SAI_SDK_XGS)
      return hasField(
          std::get<std::optional<
              SaiAclTableTraits::Attributes::FieldIpv6NextHeader>>(attributes));
#else
      return false;
#endif
    case cfg::AclTableQualifier::UDF:
      /* not supported */
      return false;
  }
  return false;
}

bool SaiAclTableManager::areQualifiersSupported(
    const std::string& aclTableName,
    const std::set<cfg::AclTableQualifier>& qualifiers) const {
  return std::all_of(
      std::begin(qualifiers),
      std::end(qualifiers),
      [this, aclTableName](auto qualifier) {
        return isQualifierSupported(aclTableName, qualifier);
      });
}

bool SaiAclTableManager::areQualifiersSupportedInDefaultAclTable(
    const std::set<cfg::AclTableQualifier>& qualifiers) const {
  return areQualifiersSupported(kAclTable1, qualifiers);
}

void SaiAclTableManager::recreateAclTable(
    std::shared_ptr<SaiAclTable>& aclTable,
    const SaiAclTableTraits::CreateAttributes& newAttributes) {
  bool aclTableUpdateSupport =
      platform_->getAsic()->isSupported(HwAsic::Feature::SAI_ACL_TABLE_UPDATE);
#if defined(TAJO_SDK_VERSION_1_42_8)
  aclTableUpdateSupport = false;
#endif
  if (!aclTableUpdateSupport) {
    XLOG(WARNING) << "feature to update acl table is not supported";
    return;
  }
  XLOG(DBG2) << "refreshing acl table schema";
  auto adapterHostKey = aclTable->adapterHostKey();
  auto& aclEntryStore = saiStore_->get<SaiAclEntryTraits>();

  std::map<
      SaiAclEntryTraits::AdapterHostKey,
      SaiAclEntryTraits::CreateAttributes>
      entries{};
  // remove acl entries from acl table, retain their attributes
  for (const auto& entry : aclEntryStore) {
    auto key = entry.second.lock()->adapterHostKey();
    if (std::get<SaiAclEntryTraits::Attributes::TableId>(key) !=
        static_cast<sai_object_id_t>(aclTable->adapterKey())) {
      continue;
    }
    auto value = entry.second.lock()->attributes();
    auto aclEntry = aclEntryStore.setObject(key, value);
    entries.emplace(key, value);
    aclEntry.reset();
  }
  // remove group member and acl table, since store holds only weak ptr after
  // setObject is invoked, clearing returned shared ptr is enough to destroy SAI
  // object and call SAI remove API.
  std::shared_ptr<SaiAclTableGroupMember> groupMember{};
  SaiAclTableGroupMemberTraits::AdapterHostKey memberAdapterHostKey{};
  SaiAclTableGroupMemberTraits::CreateAttributes memberAttrs{};
  auto& aclGroupMemberStore = saiStore_->get<SaiAclTableGroupMemberTraits>();
  sai_object_id_t aclTableGroupId{};
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ACL_TABLE_GROUP)) {
    for (auto entry : aclGroupMemberStore) {
      auto member = entry.second.lock();
      auto key = member->adapterHostKey();
      auto attrs = member->attributes();
      if (std::get<SaiAclTableGroupMemberTraits::Attributes::TableId>(attrs) !=
          static_cast<sai_object_id_t>(aclTable->adapterKey())) {
        continue;
      }
      groupMember = aclGroupMemberStore.setObject(key, attrs);
      memberAdapterHostKey = groupMember->adapterHostKey();
      memberAttrs = groupMember->attributes();
      break;
    }
    aclTableGroupId =
        std::get<SaiAclTableGroupMemberTraits::Attributes::TableGroupId>(
            memberAttrs)
            .value();
    managerTable_->switchManager().resetIngressAcl();
    // reset group member
    groupMember.reset();
  }
  // remove acl table
  aclTable.reset();

  // update acl table
  auto& aclTableStore = saiStore_->get<SaiAclTableTraits>();
  aclTable = aclTableStore.setObject(adapterHostKey, newAttributes);
  // restore acl table group member

  sai_object_id_t tableId = aclTable->adapterKey();
  if (platform_->getAsic()->isSupported(HwAsic::Feature::ACL_TABLE_GROUP)) {
    std::get<SaiAclTableGroupMemberTraits::Attributes::TableId>(
        memberAdapterHostKey) = tableId;
    std::get<SaiAclTableGroupMemberTraits::Attributes::TableId>(memberAttrs) =
        tableId;
    aclGroupMemberStore.addWarmbootHandle(memberAdapterHostKey, memberAttrs);
    managerTable_->switchManager().setIngressAcl(aclTableGroupId);
  }
  // skip recreating acl entries as acl entry information is lost.
  // this happens because SAI API layer returns default values for unset ACL
  // entry attributes. some of the attributes may be unsupported in sdk or could
  // stretch the key width beyind what's supported.
}

void SaiAclTableManager::removeUnclaimedAclCounter() {
  saiStore_->get<SaiAclCounterTraits>().removeUnclaimedWarmbootHandlesIf(
      [](const auto& aclCounter) {
        aclCounter->release();
        return true;
      });
}

AclStats SaiAclTableManager::getAclStats() const {
  AclStats aclStats;
  for (const auto& handle : handles_) {
    for (const auto& aclMember : handle.second->aclTableMembers) {
      for (const auto& [counterType, counterName] :
           aclMember.second->aclCounterTypeAndName) {
        aclStats.statNameToCounterMap()->insert(
            {counterName, aclStats_.getCumulativeValueIf(counterName)});
      }
    }
  }
  return aclStats;
}

std::shared_ptr<AclTable> SaiAclTableManager::reconstructAclTable(
    int /*priority*/,
    const std::string& /*name*/) const {
  throw FbossError("reconstructAclTable not implemented in SaiAclTableManager");
}

std::shared_ptr<AclEntry> SaiAclTableManager::reconstructAclEntry(
    const std::string& /*tableName*/,
    const std::string& /*aclEntryName*/,
    int /*priority*/) const {
  throw FbossError("reconstructAclEntry not implemented in SaiAclTableManager");
}
} // namespace facebook::fboss
