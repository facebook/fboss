/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/acl/rule/AclRuleAttrs.h"

#include <fmt/format.h>
#include <algorithm>
#include <stdexcept>

namespace facebook::fboss {

std::pair<cfg::AclTable*, std::string> findAclTable(
    cfg::SwitchConfig& swConfig,
    const std::string& tableName) {
  if (!swConfig.aclTableGroups() || swConfig.aclTableGroups()->empty()) {
    throw std::runtime_error(
        "No aclTableGroups found in config. "
        "Only field-56 (aclTableGroups) is supported; "
        "the deprecated aclTableGroup (field 45) is not.");
  }

  for (auto& group : *swConfig.aclTableGroups()) {
    auto& tables = *group.aclTables();
    auto tit =
        std::find_if(tables.begin(), tables.end(), [&](const cfg::AclTable& t) {
          return *t.name() == tableName;
        });
    if (tit != tables.end()) {
      return {&*tit, *group.name()};
    }
  }
  throw std::runtime_error(
      fmt::format("AclTable '{}' not found in any AclTableGroup", tableName));
}

} // namespace facebook::fboss
