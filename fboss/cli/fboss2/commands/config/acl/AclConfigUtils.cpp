/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/acl/AclConfigUtils.h"

#include <fmt/format.h>
#include <algorithm>
#include <stdexcept>

namespace facebook::fboss::acl_utils {

cfg::AclTableGroup& findAclTableGroupOrThrow(
    cfg::SwitchConfig& swConfig,
    const std::string& groupName) {
  if (!swConfig.aclTableGroups() || swConfig.aclTableGroups()->empty()) {
    throw std::runtime_error(
        "No aclTableGroups found in config. "
        "Only field-56 (aclTableGroups) is supported; "
        "the deprecated aclTableGroup (field 45) is not.");
  }

  auto& groups = *swConfig.aclTableGroups();
  auto it = std::find_if(
      groups.begin(), groups.end(), [&](const cfg::AclTableGroup& g) {
        return *g.name() == groupName;
      });
  if (it == groups.end()) {
    throw std::runtime_error(
        fmt::format("AclTableGroup '{}' not found", groupName));
  }
  return *it;
}

} // namespace facebook::fboss::acl_utils
