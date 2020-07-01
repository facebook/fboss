/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiAclTable = SaiObject<SaiAclTableTraits>;
using SaiAclEntry = SaiObject<SaiAclEntryTraits>;

struct SaiAclEntryHandle {
  std::shared_ptr<SaiAclEntry> aclEntry;
};

struct SaiAclTableHandle {
  std::shared_ptr<SaiAclTable> aclTable;
  // SAI ACl Entry name to corresponding handle
  folly::F14FastMap<std::string, std::unique_ptr<SaiAclEntryHandle>>
      aclTableMembers;
};

class SaiAclTableManager {
 public:
  SaiAclTableManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, and then pass and process following
   * data type for {add, remove, changed}AclTable:
   * const std:shared_ptr<AclTable>&.
   */
  AclTableSaiId addAclTable(const std::string& aclTableName);
  void removeAclTable();
  void changedAclTable();

  const SaiAclTableHandle* getAclTableHandle(
      const std::string& aclTableName) const;

  void addAclEntry(const std::shared_ptr<AclEntry>& addedAclEntry);
  void removeAclEntry(const std::shared_ptr<AclEntry>& removedAclEntry);
  void changedAclEntry(
      const std::shared_ptr<AclEntry>& oldAclEntry,
      const std::shared_ptr<AclEntry>& newAclEntry);

 private:
  SaiAclTableHandle* FOLLY_NULLABLE
  getAclTableHandleImpl(const std::string& aclTableName) const;

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  // SAI ACL Table name to corresponding Handle
  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, then use name from AclTable as key.
   */
  using SaiAclTableHandles =
      folly::F14FastMap<std::string, std::unique_ptr<SaiAclTableHandle>>;
  SaiAclTableHandles handles_;

  const sai_uint32_t aclEntryMinimumPriority_;
  const sai_uint32_t aclEntryMaximumPriority_;
};

} // namespace facebook::fboss
