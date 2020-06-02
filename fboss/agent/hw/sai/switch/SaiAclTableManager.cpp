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

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiAclTableManager::SaiAclTableManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiAclTableManager::addAclTable(const std::string& /*aclTableName*/) {
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
