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

namespace facebook::fboss {

SaiAclTableManager::SaiAclTableManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiAclTableManager::addAclTable() {
  /*
   * TODO(skhare)
   * Add single ACL Table for now (called during SaiSwitch::init()).
   * Later, extend SwitchState to carry AclTable, and then process it to
   * addAclTable.
   */
}

void SaiAclTableManager::removeAclTable() {
  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, and then process it to remove
   * AclTable.
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

void SaiAclTableManager::addAclEntry(
    const std::shared_ptr<AclEntry>& /*addedAclEntry*/) {
  // TODO(skhare) add Acl Entry
}

void SaiAclTableManager::removeAclEntry(
    const std::shared_ptr<AclEntry>& /*removedAclEntry*/) {
  // TODO(skhare) remove Acl Entry
}

void SaiAclTableManager::changedAclEntry(
    const std::shared_ptr<AclEntry>& oldAclEntry,
    const std::shared_ptr<AclEntry>& newAclEntry) {
  /*
   * ASIC/SAI implementation typically does not allow modifying an ACL entry.
   * Thus, remove and re-add.
   */
  removeAclEntry(oldAclEntry);
  addAclEntry(newAclEntry);
}
} // namespace facebook::fboss
