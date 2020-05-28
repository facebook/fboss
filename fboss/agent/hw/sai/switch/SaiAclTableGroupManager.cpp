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

namespace facebook::fboss {

SaiAclTableGroupManager::SaiAclTableGroupManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiAclTableGroupManager::addAclTableGroup(sai_acl_stage_t /*aclStage*/) {
  /*
   * TODO(skhare)
   * Add ACL Table group for aclStage.
   */
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
