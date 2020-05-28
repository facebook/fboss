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
  void addAclTable();
  void removeAclTable();
  void changedAclTable();

  void addAclEntry(const std::shared_ptr<AclEntry>& addedAclEntry);
  void removeAclEntry(const std::shared_ptr<AclEntry>& removedAclEntry);
  void changedAclEntry(
      const std::shared_ptr<AclEntry>& oldAclEntry,
      const std::shared_ptr<AclEntry>& newAclEntry);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
