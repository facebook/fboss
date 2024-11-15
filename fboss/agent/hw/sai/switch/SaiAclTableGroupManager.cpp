/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiAclTableGroupManager::SaiAclTableGroupManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

sai_acl_stage_t SaiAclTableGroupManager::cfgAclStageToSaiAclStage(
    cfg::AclStage aclStage) {
  switch (aclStage) {
    case cfg::AclStage::INGRESS:
      return SAI_ACL_STAGE_INGRESS;
    case cfg::AclStage::INGRESS_MACSEC:
      return SAI_ACL_STAGE_INGRESS_MACSEC;
    case cfg::AclStage::EGRESS_MACSEC:
      return SAI_ACL_STAGE_EGRESS_MACSEC;
    case cfg::AclStage::EGRESS:
      return SAI_ACL_STAGE_EGRESS;
  }

  // should return in one of the cases
  throw FbossError("Unsupported ACL stage");
}

void SaiAclTableGroupManager::removeAclTableGroup(
    const std::shared_ptr<AclTableGroup>& /*removedAclTableGroup*/) {
  // TODO(skhare) add support
}

void SaiAclTableGroupManager::changedAclTableGroup(
    const std::shared_ptr<AclTableGroup>& /*oldAclTableGroup*/,
    const std::shared_ptr<AclTableGroup>& /*newAclTableGroup*/) {
  // TODO(skhare) add support
}

AclTableGroupSaiId SaiAclTableGroupManager::addAclTableGroup(
    const std::shared_ptr<AclTableGroup>& addedAclTableGroup) {
  sai_acl_stage_t saiAclStage =
      cfgAclStageToSaiAclStage(addedAclTableGroup->getID());

  // If we already store a handle for this this Acl Table group, fail to add a
  // new one.
  auto handle = getAclTableGroupHandle(saiAclStage);
  if (handle) {
    throw FbossError(
        "attempted to add a duplicate aclTableGroup: ", saiAclStage);
  }

  auto& aclTableGroupStore = saiStore_->get<SaiAclTableGroupTraits>();
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_SWITCH};
  sai_int32_t type = SAI_ACL_TABLE_GROUP_TYPE_PARALLEL;

  SaiAclTableGroupTraits::AdapterHostKey adapterHostKey{
      saiAclStage, bindPointList, type};
  SaiAclTableGroupTraits::CreateAttributes attributes{
      saiAclStage, bindPointList, type};

  auto saiAclTableGroup =
      aclTableGroupStore.setObject(adapterHostKey, attributes);
  auto aclTableGroupHandle = std::make_unique<SaiAclTableGroupHandle>();
  aclTableGroupHandle->aclTableGroup = saiAclTableGroup;
  auto [it, inserted] =
      handles_.emplace(saiAclStage, std::move(aclTableGroupHandle));
  CHECK(inserted);

  managerTable_->switchManager().setIngressAcl();

  return it->second->aclTableGroup->adapterKey();
}

const SaiAclTableGroupHandle* FOLLY_NULLABLE
SaiAclTableGroupManager::getAclTableGroupHandle(
    sai_acl_stage_t aclStage) const {
  return getAclTableGroupHandleImpl(aclStage);
}

SaiAclTableGroupHandle* FOLLY_NULLABLE
SaiAclTableGroupManager::getAclTableGroupHandle(sai_acl_stage_t aclStage) {
  return getAclTableGroupHandleImpl(aclStage);
}

SaiAclTableGroupHandle* FOLLY_NULLABLE
SaiAclTableGroupManager::getAclTableGroupHandleImpl(
    sai_acl_stage_t aclStage) const {
  auto itr = handles_.find(aclStage);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->aclTableGroup) {
    XLOG(FATAL) << "invalid null Acl table group for: " << aclStage;
  }
  return itr->second.get();
}

AclTableGroupMemberSaiId SaiAclTableGroupManager::addAclTableGroupMember(
    sai_acl_stage_t aclStage,
    AclTableSaiId aclTableSaiId,
    const std::string& aclTableName) {
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL_TABLE_GROUP));

  // If we attempt to add member to a group that does not exist, fail.
  auto aclTableGroupHandle = getAclTableGroupHandle(aclStage);
  if (!aclTableGroupHandle) {
    throw FbossError(
        "attempted to add AclTable to a group that does not exist: ", aclStage);
  }

  // If we already store a handle for this this Acl Table group member, fail to
  // add new one.
  auto aclTableGroupMemberHandle =
      getAclTableGroupMemberHandle(aclTableGroupHandle, aclTableName);
  if (aclTableGroupMemberHandle) {
    throw FbossError(
        "attempted to add a duplicate aclTableGroup member: ", aclTableName);
  }

  auto& aclTableGroupMemberStore =
      saiStore_->get<SaiAclTableGroupMemberTraits>();

  SaiAclTableGroupMemberTraits::Attributes::TableGroupId
      aclTableGroupIdAttribute{
          aclTableGroupHandle->aclTableGroup->adapterKey()};
  SaiAclTableGroupMemberTraits::Attributes::TableId aclTableIdAttribute{
      aclTableSaiId};

  /*
   * TODO(skhare)
   * Priority is valid for SEQUENTIAL ACL table groups only, while we only use
   * PARALLEL ACL groups today.
   * But Priority field is mandatory as per SAI spec, so set it to some value.
   * In future, if we support SEQUENTIAL ACL groups, read MINIMUM and MAXIMUM
   * PRIORITY Switch attribute and use as appropriate below.
   */
  SaiAclTableGroupMemberTraits::Attributes::Priority priority{
      SAI_SWITCH_ATTR_ACL_TABLE_MINIMUM_PRIORITY};

  SaiAclTableGroupMemberTraits::AdapterHostKey adapterHostKey{
      aclTableGroupIdAttribute, aclTableIdAttribute, priority};
  SaiAclTableGroupMemberTraits::CreateAttributes attributes{
      aclTableGroupIdAttribute, aclTableIdAttribute, priority};
  auto saiAclTableGroupMember =
      aclTableGroupMemberStore.setObject(adapterHostKey, attributes);

  auto groupMemberHandle = std::make_unique<SaiAclTableGroupMemberHandle>();
  groupMemberHandle->aclTableGroupMember = saiAclTableGroupMember;

  auto [it, inserted] = aclTableGroupHandle->aclTableGroupMembers.emplace(
      aclTableName, std::move(groupMemberHandle));
  CHECK(inserted);

  return it->second->aclTableGroupMember->adapterKey();
}

void SaiAclTableGroupManager::removeAclTableGroupMember(
    sai_acl_stage_t aclStage,
    const std::string& aclTableName) {
  // If we attempt to remove member from a group that does not exist, fail.
  auto aclTableGroupHandle = getAclTableGroupHandle(aclStage);
  if (!aclTableGroupHandle) {
    throw FbossError(
        "attempted to remove AclTable from a group that does not exist: ",
        aclStage);
  }

  // If we don't store a handle for this this Acl Table group member, fail to
  // remove.
  auto aclTableGroupMemberHandle =
      getAclTableGroupMemberHandle(aclTableGroupHandle, aclTableName);
  if (!aclTableGroupMemberHandle) {
    throw FbossError(
        "attempted to remove non-existing aclTableGroup member: ",
        aclTableName);
  }

  aclTableGroupHandle->aclTableGroupMembers.erase(aclTableName);
}

const SaiAclTableGroupMemberHandle* FOLLY_NULLABLE
SaiAclTableGroupManager::getAclTableGroupMemberHandle(
    const SaiAclTableGroupHandle* aclTableGroupHandle,
    const std::string& aclTableName) const {
  auto itr = aclTableGroupHandle->aclTableGroupMembers.find(aclTableName);
  if (itr == aclTableGroupHandle->aclTableGroupMembers.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->aclTableGroupMember) {
    XLOG(FATAL) << "invalid null Acl table group member for: " << aclTableName;
  }
  return itr->second.get();
}

std::shared_ptr<AclTableGroup>
SaiAclTableGroupManager::reconstructAclTableGroup(
    cfg::AclStage /*stage*/,
    const std::string& /*name*/) const {
  throw FbossError(
      "reconstructAclTableGroup not implemented in SaiAclTableGroupManager");
}
} // namespace facebook::fboss
