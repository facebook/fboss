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
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiAclTableGroupManager::SaiAclTableGroupManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

AclTableGroupSaiId SaiAclTableGroupManager::addAclTableGroup(
    sai_acl_stage_t aclStage) {
  // If we already store a handle for this this Acl Table group, fail to add a
  // new one.
  auto handle = getAclTableGroupHandle(aclStage);
  if (handle) {
    throw FbossError("attempted to add a duplicate aclTableGroup: ", aclStage);
  }

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  auto& aclTableGroupStore = s->get<SaiAclTableGroupTraits>();
  std::vector<sai_int32_t> bindPointList{SAI_ACL_BIND_POINT_TYPE_SWITCH};
  sai_int32_t type = SAI_ACL_TABLE_GROUP_TYPE_PARALLEL;

  SaiAclTableGroupTraits::AdapterHostKey adapterHostKey{
      aclStage, bindPointList, type};
  SaiAclTableGroupTraits::CreateAttributes attributes{
      aclStage, bindPointList, type};

  auto saiAclTableGroup =
      aclTableGroupStore.setObject(adapterHostKey, attributes);
  auto aclTableGroupHandle = std::make_unique<SaiAclTableGroupHandle>();
  aclTableGroupHandle->aclTableGroup = saiAclTableGroup;
  auto [it, inserted] =
      handles_.emplace(aclStage, std::move(aclTableGroupHandle));
  CHECK(inserted);

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

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  auto& aclTableGroupMemberStore = s->get<SaiAclTableGroupMemberTraits>();

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
    sai_acl_stage_t /*aclStage*/,
    AclTableSaiId /*aclTableSaiId*/,
    const std::string& /*aclTableName*/) {
  /*
   * TODO(skhare)
   * Remove given aclTableSaiId from group for aclStage
   */
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

} // namespace facebook::fboss
