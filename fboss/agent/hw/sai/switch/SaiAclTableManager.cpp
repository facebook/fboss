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
    : managerTable_(managerTable), platform_(platform) {}

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

const SaiAclTableHandle* SaiAclTableManager::getAclTableHandle(
    const std::string& aclTableName) const {
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

void SaiAclTableManager::addAclEntry(
    const std::shared_ptr<AclEntry>& /*addedAclEntry*/) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

  // TODO(skhare) add Acl Entry
}

void SaiAclTableManager::removeAclEntry(
    const std::shared_ptr<AclEntry>& /*removedAclEntry*/) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

  // TODO(skhare) remove Acl Entry
}

void SaiAclTableManager::changedAclEntry(
    const std::shared_ptr<AclEntry>& oldAclEntry,
    const std::shared_ptr<AclEntry>& newAclEntry) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

  /*
   * ASIC/SAI implementation typically does not allow modifying an ACL entry.
   * Thus, remove and re-add.
   */
  removeAclEntry(oldAclEntry);
  addAclEntry(newAclEntry);
}
} // namespace facebook::fboss
