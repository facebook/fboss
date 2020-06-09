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
  CHECK(platform_->getAsic()->isSupported(HwAsic::Feature::ACL));

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
  SaiAclTableGroupTraits::AdapterHostKey attributes{
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
  auto itr = handles_.find(aclStage);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second || !itr->second->aclTableGroup) {
    XLOG(FATAL) << "invalid null Acl table group for: " << aclStage;
  }
  return itr->second.get();
}

void SaiAclTableGroupManager::addTableGroupMember(
    sai_acl_stage_t /*aclStage*/,
    AclTableSaiId /*aclTableSaiId*/) {
  /*
   * TODO(skhare)
   * Add given aclTableSaiId to group for aclStage
   */
}

void SaiAclTableGroupManager::removeTableGroupMember(
    sai_acl_stage_t /*aclStage*/,
    AclTableSaiId /*aclTableSaiId*/) {
  /*
   * TODO(skhare)
   * Remove given aclTableSaiId from group for aclStage
   */
}
} // namespace facebook::fboss
