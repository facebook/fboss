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

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

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
              SaiSwitchTraits::Attributes::AclEntryMaximumPriority())) {}

AclTableSaiId SaiAclTableManager::addAclTable(const std::string& aclTableName) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

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

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  auto& aclTableStore = s->get<SaiAclTableTraits>();
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_SWITCH};
  std::vector<sai_int32_t> actionTypeList{SAI_ACL_ACTION_TYPE_PACKET_ACTION,
                                          SAI_ACL_ACTION_TYPE_MIRROR_INGRESS,
                                          SAI_ACL_ACTION_TYPE_MIRROR_EGRESS,
                                          SAI_ACL_ACTION_TYPE_SET_TC,
                                          SAI_ACL_ACTION_TYPE_SET_DSCP};

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
      ? std::make_optional(
            SaiAclTableTraits::Attributes::FieldFdbDstUserMeta{true})
      : std::nullopt;

  SaiAclTableTraits::AdapterHostKey adapterHostKey{
      SAI_ACL_STAGE_INGRESS,
      bindPointList,
      actionTypeList,
      true, // srcIpv6
      true, // dstIpv6
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // inPort
      true, // outPort
      true, // ipFrag
      true, // dscp
      true, // dstMac
      true, // ipType
      true, // ttl
      fieldFdbDstUserMeta, // fdb meta
      true, // route meta
      true // neighbor meta
  };
  SaiAclTableTraits::CreateAttributes attributes{
      SAI_ACL_STAGE_INGRESS,
      bindPointList,
      actionTypeList,
      true, // srcIpv6
      true, // dstIpv6
      true, // l4SrcPort
      true, // l4DstPort
      true, // ipProtocol
      true, // tcpFlags
      true, // inPort
      true, // outPort
      true, // ipFrag
      true, // dscp
      true, // dstMac
      true, // ipType
      true, // ttl
      fieldFdbDstUserMeta, // fdb meta
      true, // route meta
      true // neighbor meta
  };

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
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

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
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

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
   * SwitchState: smaller ACL ID means higher priority.
   * BCM API: larger priority means higher priority.
   * BCM SAI: larger priority means higher priority (TODO: confirm).
   * Tajo SAI: ?  TODO find the behavior and then remove below CHECK
   * SAI spec: does not define?
   * But larger priority means higher priority is documented here:
   * https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Proposal-ACL-1.md
   */
  CHECK(
      platform_->getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TAJO);

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

AclEntrySaiId SaiAclTableManager::addAclEntry(
    const std::shared_ptr<AclEntry>& addedAclEntry,
    const std::string& aclTableName) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

  // If we attempt to add entry to a table that does not exist, fail.
  auto aclTableHandle = getAclTableHandle(aclTableName);
  if (!aclTableHandle) {
    throw FbossError(
        "attempted to add AclEntry to a AclTable that does not exist: ",
        aclTableName);
  }

  // If we already store a handle for this this Acl Entry, fail to add new one.
  auto aclEntryHandle =
      getAclEntryHandle(aclTableHandle, addedAclEntry->getID());
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

  // TODO(skhare) Support all other ACL fields
  std::optional<SaiAclEntryTraits::Attributes::FieldSrcIpV6> fieldSrcIpV6{
      std::nullopt};
  if (addedAclEntry->getSrcIp().first &&
      addedAclEntry->getSrcIp().first.isV6()) {
    auto srcIpV6Mask = folly::IPAddressV6(
        folly::IPAddressV6::fetchMask(addedAclEntry->getSrcIp().second));
    fieldSrcIpV6 = SaiAclEntryTraits::Attributes::FieldSrcIpV6{
        AclEntryFieldIpV6(std::make_pair(
            addedAclEntry->getSrcIp().first.asV6(), srcIpV6Mask))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldDstIpV6> fieldDstIpV6{
      std::nullopt};
  if (addedAclEntry->getDstIp().first &&
      addedAclEntry->getDstIp().first.isV6()) {
    auto dstIpV6Mask = folly::IPAddressV6(
        folly::IPAddressV6::fetchMask(addedAclEntry->getDstIp().second));
    fieldDstIpV6 = SaiAclEntryTraits::Attributes::FieldDstIpV6{
        AclEntryFieldIpV6(std::make_pair(
            addedAclEntry->getDstIp().first.asV6(), dstIpV6Mask))};
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

  std::optional<SaiAclEntryTraits::Attributes::FieldDscp> fieldDscp{
      std::nullopt};
  if (addedAclEntry->getDscp()) {
    fieldDscp = SaiAclEntryTraits::Attributes::FieldDscp{AclEntryFieldU8(
        std::make_pair(addedAclEntry->getDscp().value(), kDscpMask))};
  }

  std::optional<SaiAclEntryTraits::Attributes::FieldTtl> fieldTtl{std::nullopt};
  if (addedAclEntry->getTtl()) {
    fieldTtl =
        SaiAclEntryTraits::Attributes::FieldTtl{AclEntryFieldU8(std::make_pair(
            addedAclEntry->getTtl().value().getValue(),
            addedAclEntry->getTtl().value().getMask()))};
  }

  // TODO(skhare) Support all other ACL actions
  std::optional<SaiAclEntryTraits::Attributes::ActionPacketAction>
      aclActionPacketAction{std::nullopt};
  const auto& act = addedAclEntry->getActionType();
  if (act == cfg::AclActionType::DENY) {
    aclActionPacketAction = SaiAclEntryTraits::Attributes::ActionPacketAction{
        SAI_PACKET_ACTION_DROP};
  }

  // TODO(skhare) At least one field and one action must be specified.
  // Once we add support for all fields and actions, throw error if that is not
  // honored.
  if (!((fieldSrcIpV6.has_value() || fieldDstIpV6.has_value() ||
         fieldL4SrcPort.has_value() || fieldL4DstPort.has_value() ||
         fieldIpProtocol.has_value() || fieldTcpFlags.has_value() ||
         fieldDscp.has_value() || fieldTtl.has_value()) &&
        aclActionPacketAction.has_value())) {
    XLOG(DBG)
        << "Unsupported field/action for aclEntry: addedAclEntry->getID())";
    return AclEntrySaiId{0};
  }

  SaiAclEntryTraits::AdapterHostKey adapterHostKey{
      aclTableId,
      priority,
      fieldSrcIpV6,
      fieldDstIpV6,
      fieldL4SrcPort,
      fieldL4DstPort,
      fieldIpProtocol,
      fieldTcpFlags,
      fieldDscp,
      fieldTtl,
      std::nullopt, // fdb meta
      std::nullopt, // route meta
      std::nullopt, // neighbor meta
      aclActionPacketAction,
      std::nullopt, // setTC
  };
  SaiAclEntryTraits::CreateAttributes attributes{
      aclTableId,
      priority,
      fieldSrcIpV6,
      fieldDstIpV6,
      fieldL4SrcPort,
      fieldL4DstPort,
      fieldIpProtocol,
      fieldTcpFlags,
      fieldDscp,
      fieldTtl,
      std::nullopt, // fdb meta
      std::nullopt, // route meta
      std::nullopt, // neighbor meta
      aclActionPacketAction,
      std::nullopt, // setTC
  };

  auto saiAclEntry = aclEntryStore.setObject(adapterHostKey, attributes);
  auto entryHandle = std::make_unique<SaiAclEntryHandle>();
  entryHandle->aclEntry = saiAclEntry;

  auto [it, inserted] = aclTableHandle->aclTableMembers.emplace(
      addedAclEntry->getID(), std::move(entryHandle));
  CHECK(inserted);

  return it->second->aclEntry->adapterKey();
}

void SaiAclTableManager::removeAclEntry(
    const std::shared_ptr<AclEntry>& removedAclEntry,
    const std::string& aclTableName) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

  // If we attempt to remove entry for a table that does not exist, fail.
  auto aclTableHandle = getAclTableHandle(aclTableName);
  if (!aclTableHandle) {
    throw FbossError(
        "attempted to remove AclEntry to a AclTable that does not exist: ",
        aclTableName);
  }

  // If we attempt to remove entry that does not exist, fail.
  auto itr = aclTableHandle->aclTableMembers.find(removedAclEntry->getID());
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
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

  /*
   * ASIC/SAI implementation typically does not allow modifying an ACL entry.
   * Thus, remove and re-add.
   */
  removeAclEntry(oldAclEntry, aclTableName);
  addAclEntry(newAclEntry, aclTableName);
}

const SaiAclEntryHandle* FOLLY_NULLABLE SaiAclTableManager::getAclEntryHandle(
    const SaiAclTableHandle* aclTableHandle,
    const std::string& aclEntryName) const {
  auto itr = aclTableHandle->aclTableMembers.find(aclEntryName);
  if (itr == aclTableHandle->aclTableMembers.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->aclEntry) {
    XLOG(FATAL) << "invalid null Acl entry for: " << aclEntryName;
  }
  return itr->second.get();
}

} // namespace facebook::fboss
