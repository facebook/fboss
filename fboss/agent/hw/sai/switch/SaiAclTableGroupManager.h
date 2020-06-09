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

using SaiAclTableGroup = SaiObject<SaiAclTableGroupTraits>;
using SaiAclTableGroupMember = SaiObject<SaiAclTableGroupMemberTraits>;

struct SaiAclTableGroupMemberHandle {
  std::shared_ptr<SaiAclTableGroupMember> aclTableGroupMember;
};

struct SaiAclTableGroupHandle {
  std::shared_ptr<SaiAclTableGroup> aclTableGroup;
  // SAI ACl Table Group member name (ACL Table name) to corresponding handle
  folly::F14FastMap<std::string, std::unique_ptr<SaiAclTableGroupMemberHandle>>
      aclTableGroupMembers;
};

class SaiAclTableGroupManager {
 public:
  SaiAclTableGroupManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  AclTableGroupSaiId addAclTableGroup(sai_acl_stage_t aclStage);

  AclTableGroupMemberSaiId addAclTableGroupMember(
      sai_acl_stage_t aclStage,
      AclTableSaiId aclTableSaiId,
      const std::string& aclTableName);
  void removeAclTableGroupMember(
      sai_acl_stage_t aclStage,
      AclTableSaiId aclTableSaiId,
      const std::string& aclTableName);

  const SaiAclTableGroupHandle* FOLLY_NULLABLE
  getAclTableGroupHandle(sai_acl_stage_t aclStage) const;
  SaiAclTableGroupHandle* FOLLY_NULLABLE
  getAclTableGroupHandle(sai_acl_stage_t aclStage);

  const SaiAclTableGroupMemberHandle* FOLLY_NULLABLE
  getAclTableGroupMemberHandle(
      const SaiAclTableGroupHandle* aclTableGroupHandle,
      const std::string& aclTableName) const;

 private:
  SaiAclTableGroupHandle* FOLLY_NULLABLE
  getAclTableGroupHandleImpl(sai_acl_stage_t aclStage) const;

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  // SAI ACL Table Group to corresponding Handle
  /*
   * TODO(skhare)
   * Extend SwitchState to carry AclTable, then use AclStage in the
   * SwitchState's AclTable as the key.
   */
  using SaiAclTableGroupHandles = folly::
      F14FastMap<sai_acl_stage_t, std::unique_ptr<SaiAclTableGroupHandle>>;
  SaiAclTableGroupHandles handles_;
};

} // namespace facebook::fboss
