/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/AclTestUtils.h"
#include <memory>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/SwitchState.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss::utility {

std::string kDefaultAclTable() {
  return "AclTable1";
}

int getAclTableIndex(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName) {
  if (!cfg->aclTableGroup()) {
    throw FbossError(
        "Multiple acl tables flag enabled but config leaves aclTableGroup empty");
  }
  if (!tableName.has_value()) {
    throw FbossError(
        "Multiple acl tables flag enabled but no acl table name provided for add/delAcl()");
  }
  int tableIndex;
  std::vector<cfg::AclTable> aclTables = *cfg->aclTableGroup()->aclTables();
  std::vector<cfg::AclTable>::iterator it = std::find_if(
      aclTables.begin(), aclTables.end(), [&](cfg::AclTable const& aclTable) {
        return *aclTable.name() == tableName.value();
      });
  if (it != aclTables.end()) {
    tableIndex = std::distance(aclTables.begin(), it);
  } else {
    throw FbossError(
        "Table with name ", tableName.value(), " does not exist in config");
  }
  return tableIndex;
}

cfg::AclEntry* addAclEntry(
    cfg::SwitchConfig* cfg,
    cfg::AclEntry& acl,
    const std::optional<std::string>& tableName) {
  if (FLAGS_enable_acl_table_group) {
    auto aclTableName =
        tableName.has_value() ? tableName.value() : kDefaultAclTable();
    int tableNumber = getAclTableIndex(cfg, aclTableName);
    CHECK(cfg->aclTableGroup().has_value());
    cfg->aclTableGroup()->aclTables()[tableNumber].aclEntries()->push_back(acl);
    return &cfg->aclTableGroup()->aclTables()[tableNumber].aclEntries()->back();
  } else {
    cfg->acls()->push_back(acl);
    return &cfg->acls()->back();
  }
}

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType,
    const std::optional<std::string>& tableName) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  *acl.actionType() = aclActionType;

  return addAclEntry(cfg, acl, tableName);
}

} // namespace facebook::fboss::utility
