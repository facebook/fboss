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
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/MacAddress.h>

namespace facebook::fboss {

SaiAclTableManager::SaiAclTableManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable),
      platform_(platform),
      aclEntryMinimumPriority_(
          SaiApiTable::getInstance()->switchApi().getAttribute(
              managerTable_->switchManager().getSwitchSaiId(),
              SaiSwitchTraits::Attributes::AclEntryMinimumPriority())),
      aclEntryMaximumPriority_(
          SaiApiTable::getInstance()->switchApi().getAttribute(
              managerTable_->switchManager().getSwitchSaiId(),
              SaiSwitchTraits::Attributes::AclEntryMaximumPriority())),
      fdbDstUserMetaDataRangeMin_(getFdbDstUserMetaDataRange().min),
      fdbDstUserMetaDataRangeMax_(getFdbDstUserMetaDataRange().max),
      fdbDstUserMetaDataMask_(getMetaDataMask(fdbDstUserMetaDataRangeMax_)),
      routeDstUserMetaDataRangeMin_(getRouteDstUserMetaDataRange().min),
      routeDstUserMetaDataRangeMax_(getRouteDstUserMetaDataRange().max),
      routeDstUserMetaDataMask_(getMetaDataMask(routeDstUserMetaDataRangeMax_)),
      neighborDstUserMetaDataRangeMin_(getNeighborDstUserMetaDataRange().min),
      neighborDstUserMetaDataRangeMax_(getNeighborDstUserMetaDataRange().max),
      neighborDstUserMetaDataMask_(
          getMetaDataMask(neighborDstUserMetaDataRangeMax_)) {}

sai_u32_range_t SaiAclTableManager::getFdbDstUserMetaDataRange() const {
  return SaiApiTable::getInstance()->switchApi().getAttribute(
      managerTable_->switchManager().getSwitchSaiId(),
      SaiSwitchTraits::Attributes::FdbDstUserMetaDataRange());
}

sai_u32_range_t SaiAclTableManager::getRouteDstUserMetaDataRange() const {
  return SaiApiTable::getInstance()->switchApi().getAttribute(
      managerTable_->switchManager().getSwitchSaiId(),
      SaiSwitchTraits::Attributes::RouteDstUserMetaDataRange());
}

sai_u32_range_t SaiAclTableManager::getNeighborDstUserMetaDataRange() const {
  return SaiApiTable::getInstance()->switchApi().getAttribute(
      managerTable_->switchManager().getSwitchSaiId(),
      SaiSwitchTraits::Attributes::NeighborDstUserMetaDataRange());
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

std::
    pair<SaiAclTableTraits::AdapterHostKey, SaiAclTableTraits::CreateAttributes>
    SaiAclTableManager::createAclTableHelper() {
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_SWITCH};
  std::vector<sai_int32_t> actionTypeList{SAI_ACL_ACTION_TYPE_PACKET_ACTION,
                                          SAI_ACL_ACTION_TYPE_COUNTER,
                                          SAI_ACL_ACTION_TYPE_MIRROR_INGRESS,
                                          SAI_ACL_ACTION_TYPE_MIRROR_EGRESS,
                                          SAI_ACL_ACTION_TYPE_SET_TC,
                                          SAI_ACL_ACTION_TYPE_SET_DSCP};

  /*
   * Tajo either does not support following qualifier or enabling those
   * overflows max key width. Thus, disable those on Tajo for now.
   * Except fieldIpType, all the other fields are not used in prod today.
   * TODO(skhare) Enable fieldIpType after Tajo bug is fixed.
   */
  bool isTajo =
      platform_->getAsic()->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO;
  auto fieldL4SrcPort = isTajo ? false : true;
  auto fieldL4DstPort = isTajo ? false : true;
  auto fieldTcpFlags = isTajo ? false : true;
  auto fieldSrcPort = isTajo ? false : true;
  auto fieldOutPort = isTajo ? false : true;
  auto fieldIpFrag = isTajo ? false : true;
  auto fieldIcmpV4Type = isTajo ? false : true;
  auto fieldIcmpV4Code = isTajo ? false : true;
  auto fieldIcmpV6Type = isTajo ? false : true;
  auto fieldIcmpV6Code = isTajo ? false : true;
  auto fieldDstMac = isTajo ? false : true;
  auto fieldIpType = isTajo ? false : true;

  /*
   * FdbDstUserMetaData is required only for MH-NIC queue-per-host solution.
   * However, the solution is not applicable for Trident2 as FBOSS does not
   * implement queues on Trident2.
   * Furthermore, Trident2 supports fewer ACL qualifiers than other
   * hardwares. Thus, avoid programming unncessary qualifiers (or else we run
   * out resources).
   */
  auto fieldFdbDstUserMeta = platform_->getAsic()->getAsicType() !=
          HwAsic::AsicType::ASIC_TYPE_TRIDENT2
      ? true
      : false;

  SaiAclTableTraits::AdapterHostKey adapterHostKey{
      SAI_ACL_STAGE_INGRESS,
      bindPointList,
      actionTypeList,
      true, // srcIpv6
      true, // dstIpv6
      true, // srcIpV4
      true, // dstIpV4
      fieldL4SrcPort,
      fieldL4DstPort,
      true, // ipProtocol
      fieldTcpFlags,
      fieldSrcPort,
      fieldOutPort,
      fieldIpFrag,
      fieldIcmpV4Type,
      fieldIcmpV4Code,
      fieldIcmpV6Type,
      fieldIcmpV6Code,
      true, // dscp
      fieldDstMac,
      fieldIpType,
      true, // ttl
      fieldFdbDstUserMeta,
      true, // route meta
      true, // neighbor meta
  };

  SaiAclTableTraits::CreateAttributes attributes{adapterHostKey};

  return std::make_pair(adapterHostKey, attributes);
}

AclTableSaiId SaiAclTableManager::addAclTable(const std::string& aclTableName) {
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
  auto handle = getAclTableHandle(aclTableName);
  if (handle) {
    throw FbossError("attempted to add a duplicate aclTable: ", aclTableName);
  }

  SaiAclTableTraits::AdapterHostKey adapterHostKey;
  SaiAclTableTraits::CreateAttributes attributes;

  std::tie(adapterHostKey, attributes) = createAclTableHelper();

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  auto& aclTableStore = s->get<SaiAclTableTraits>();

  auto saiAclTable = aclTableStore.setObject(adapterHostKey, attributes);
  auto aclTableHandle = std::make_unique<SaiAclTableHandle>();
  aclTableHandle->aclTable = saiAclTable;
  auto [it, inserted] =
      handles_.emplace(aclTableName, std::move(aclTableHandle));
  CHECK(inserted);

  auto aclTableSaiId = it->second->aclTable->adapterKey();

  // Add ACL Table to group based on the stage
  managerTable_->aclTableGroupManager().addAclTableGroupMember(
      SAI_ACL_STAGE_INGRESS, aclTableSaiId, aclTableName);

  return aclTableSaiId;
}

void SaiAclTableManager::removeAclTable() {
  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, and then process it to remove
   * AclTable.
   *
   * Before ACL table is removed, remove it from appropriate ACL group:
   * managerTable_->switchManager().removeTableGroupMember(SAI_ACL_STAGE_INGRESS,
   * aclTableSaiId);
   */
  CHECK(false);
}

void SaiAclTableManager::changedAclTable() {
  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, and then process it to change
   * AclTable.
   * (We would likely have to removeAclTable() and re addAclTable() due to ASIC
   * limitations.
   */
  CHECK(false);
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

sai_uint32_t SaiAclTableManager::swPriorityToSaiPriority(int priority) const {
  /*
   * TODO(skhare)
   * When adding HwAclPriorityTests, add a test to verify that SAI
   * implementation treats larger value of priority as higher priority.
   * SwitchState: smaller ACL ID means higher priority.
   * BCM API: larger priority means higher priority.
   * BCM SAI: larger priority means higher priority (TODO: confirm).
   * Tajo SAI: larger priority means higher priority.
   * SAI spec: does not define?
   * But larger priority means higher priority is documented here:
   * https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Proposal-ACL-1.md
   */
  sai_uint32_t saiPriority = aclEntryMaximumPriority_ - priority;
  if (saiPriority < aclEntryMinimumPriority_) {
    throw FbossError(
        "Acl Entry priority out of range. Supported: [",
        aclEntryMinimumPriority_,
        ", ",
        aclEntryMaximumPriority_,
        "], specified: ",
        saiPriority);
  }

  return saiPriority;
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
  }
  // should return in one of the cases
  throw FbossError("Unsupported IP Type option");
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
      HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
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

std::shared_ptr<SaiAclCounter> SaiAclTableManager::addAclCounter(
    const SaiAclTableHandle* aclTableHandle,
    const cfg::TrafficCounter& trafficCount) {
  SaiAclCounterTraits::Attributes::TableId aclTableId{
      aclTableHandle->aclTable->adapterKey()};
  std::optional<SaiAclCounterTraits::Attributes::EnablePacketCount>
      enablePacketCount{false};
  std::optional<SaiAclCounterTraits::Attributes::EnableByteCount>
      enableByteCount{false};

  for (const auto& counterType : *trafficCount.types_ref()) {
    switch (counterType) {
      case cfg::CounterType::PACKETS:
        enablePacketCount =
            SaiAclCounterTraits::Attributes::EnablePacketCount{true};
        break;
      case cfg::CounterType::BYTES:
        enableByteCount =
            SaiAclCounterTraits::Attributes::EnableByteCount{true};
        break;
      default:
        throw FbossError("Unsupported CounterType for ACL");
    }
  }

  SaiAclCounterTraits::AdapterHostKey adapterHostKey{
      aclTableId,
      enablePacketCount,
      enableByteCount,
  };

  SaiAclCounterTraits::CreateAttributes attributes{
      aclTableId,
      enablePacketCount,
      enableByteCount,
      std::nullopt, // counterPackets
      std::nullopt, // counterBytes
  };

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  auto& aclCounterStore = s->get<SaiAclCounterTraits>();

  auto saiAclCounter = aclCounterStore.setObject(adapterHostKey, attributes);

  return saiAclCounter;
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

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  auto& aclEntryStore = s->get<SaiAclEntryTraits>();

  SaiAclEntryTraits::Attributes::TableId aclTableId{
      aclTableHandle->aclTable->adapterKey()};
  SaiAclEntryTraits::Attributes::Priority priority{
      swPriorityToSaiPriority(addedAclEntry->getPriority())};

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
  // TODO(skhare) support cpu source port (SaiCpuPortHandle)
  if (addedAclEntry->getSrcPort() &&
      addedAclEntry->getSrcPort().value() !=
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

  std::optional<SaiAclEntryTraits::Attributes::FieldIpProtocol> fieldIpProtocol{
      std::nullopt};
  if (addedAclEntry->getProto()) {
    fieldIpProtocol = SaiAclEntryTraits::Attributes::FieldIpProtocol{
        AclEntryFieldU8(std::make_pair(
            addedAclEntry->getProto().value(), kIpProtocolMask))};
  }

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
      if (addedAclEntry->getProto().value() == AclEntryFields::kProtoIcmp) {
        fieldIcmpV4Type = SaiAclEntryTraits::Attributes::FieldIcmpV4Type{
            AclEntryFieldU8(std::make_pair(
                addedAclEntry->getIcmpType().value(), kIcmpTypeMask))};
        if (addedAclEntry->getIcmpCode()) {
          fieldIcmpV4Code = SaiAclEntryTraits::Attributes::FieldIcmpV4Code{
              AclEntryFieldU8(std::make_pair(
                  addedAclEntry->getIcmpCode().value(), kIcmpCodeMask))};
        }
      } else if (
          addedAclEntry->getProto().value() == AclEntryFields::kProtoIcmpv6) {
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
  std::optional<SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta>
      fieldNeighborDstUserMeta{std::nullopt};

  if (addedAclEntry->getLookupClass()) {
    fieldRouteDstUserMeta =
        SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta{
            AclEntryFieldU32(cfgLookupClassToSaiRouteMetaDataAndMask(
                addedAclEntry->getLookupClass().value()))};
    fieldNeighborDstUserMeta =
        SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta{
            AclEntryFieldU32(cfgLookupClassToSaiNeighborMetaDataAndMask(
                addedAclEntry->getLookupClass().value()))};
  }

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
  }

  std::shared_ptr<SaiAclCounter> saiAclCounter{nullptr};
  std::optional<SaiAclEntryTraits::Attributes::ActionCounter> aclActionCounter{
      std::nullopt};

  std::optional<SaiAclEntryTraits::Attributes::ActionSetTC> aclActionSetTC{
      std::nullopt};

  std::optional<SaiAclEntryTraits::Attributes::ActionSetDSCP> aclActionSetDSCP{
      std::nullopt};

  auto action = addedAclEntry->getAclAction();
  if (action) {
    if (action.value().getTrafficCounter()) {
      saiAclCounter = addAclCounter(
          aclTableHandle, action.value().getTrafficCounter().value());
      aclActionCounter = SaiAclEntryTraits::Attributes::ActionCounter{
          AclEntryActionSaiObjectIdT(
              AclCounterSaiId{saiAclCounter->adapterKey()})};
    }

    if (action.value().getSendToQueue()) {
      auto sendToQueue = action.value().getSendToQueue().value();
      bool sendToCpu = sendToQueue.second;
      if (!sendToCpu) {
        auto queueId =
            static_cast<sai_uint8_t>(*sendToQueue.first.queueId_ref());
        aclActionSetTC = SaiAclEntryTraits::Attributes::ActionSetTC{
            AclEntryActionU8(queueId)};
      } else {
        /*
         *  When sendToCpu is set, a copy of the packet will be sent
         * to CPU.
         * TODO: By default, these packets are sent to queue
         * 0. Use TC to set the right traffic class which
         * will be mapped to queue id.
         */
        if (platform_->getAsic()->isSupported(
                HwAsic::Feature::ACL_COPY_TO_CPU)) {
          aclActionPacketAction =
              SaiAclEntryTraits::Attributes::ActionPacketAction{
                  SAI_PACKET_ACTION_COPY};
        }
      }
    }

    if (action.value().getSetDscp()) {
      const int dscpValue =
          *action.value().getSetDscp().value().dscpValue_ref();

      aclActionSetDSCP = SaiAclEntryTraits::Attributes::ActionSetDSCP{
          AclEntryActionU8(dscpValue)};
    }
  }

  // TODO(skhare) At least one field and one action must be specified.
  // Once we add support for all fields and actions, throw error if that is not
  // honored.
  if (!((fieldSrcIpV6.has_value() || fieldDstIpV6.has_value() ||
         fieldSrcIpV4.has_value() || fieldDstIpV6.has_value() ||
         fieldSrcPort.has_value() || fieldOutPort.has_value() ||
         fieldL4SrcPort.has_value() || fieldL4DstPort.has_value() ||
         fieldIpProtocol.has_value() || fieldTcpFlags.has_value() ||
         fieldIpFrag.has_value() || fieldIcmpV4Type.has_value() ||
         fieldIcmpV4Code.has_value() || fieldIcmpV6Type.has_value() ||
         fieldIcmpV6Code.has_value() || fieldDscp.has_value() ||
         fieldDstMac.has_value() || fieldIpType.has_value() ||
         fieldTtl.has_value() || fieldFdbDstUserMeta.has_value() ||
         fieldRouteDstUserMeta.has_value() ||
         fieldNeighborDstUserMeta.has_value()) &&
        (aclActionPacketAction.has_value() || aclActionCounter.has_value() ||
         aclActionSetTC.has_value() || aclActionSetDSCP.has_value()))) {
    XLOG(DBG)
        << "Unsupported field/action for aclEntry: addedAclEntry->getID())";
    return AclEntrySaiId{0};
  }

  SaiAclEntryTraits::AdapterHostKey adapterHostKey{aclTableId, priority};

  SaiAclEntryTraits::CreateAttributes attributes{
      aclTableId,
      priority,
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
      aclActionPacketAction,
      aclActionCounter,
      aclActionSetTC,
      aclActionSetDSCP,
      std::nullopt, // mirrorIngress
      std::nullopt, // mirrorEgress
  };

  auto saiAclEntry = aclEntryStore.setObject(adapterHostKey, attributes);
  auto entryHandle = std::make_unique<SaiAclEntryHandle>();
  entryHandle->aclEntry = saiAclEntry;
  entryHandle->aclCounter = saiAclCounter;

  auto [it, inserted] = aclTableHandle->aclTableMembers.emplace(
      addedAclEntry->getPriority(), std::move(entryHandle));
  CHECK(inserted);

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
    throw FbossError(
        "attempted to remove aclEntry which does not exist: ",
        removedAclEntry->getID());
  }

  aclTableHandle->aclTableMembers.erase(itr);
}

void SaiAclTableManager::changedAclEntry(
    const std::shared_ptr<AclEntry>& oldAclEntry,
    const std::shared_ptr<AclEntry>& newAclEntry,
    const std::string& aclTableName) {
  /*
   * ASIC/SAI implementation typically does not allow modifying an ACL entry.
   * Thus, remove and re-add.
   */
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

} // namespace facebook::fboss
