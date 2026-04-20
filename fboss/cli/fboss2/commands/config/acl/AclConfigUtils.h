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

#include <string>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

/*
 * Shared lookup helper for the `config acl table-group` and `config acl
 * table` subcommand handlers. Both need to resolve an AclTableGroup by name
 * out of sw.aclTableGroups() before mutating it (stage for table-group,
 * priority on one of its aclTables for table); that lookup, and the guard
 * against the unset/deprecated aclTableGroup field, is identical between the
 * two, so it lives here rather than being duplicated in each handler.
 */
namespace facebook::fboss::acl_utils {

/*
 * Returns a mutable reference to the AclTableGroup named `groupName` in
 * swConfig.aclTableGroups() (field 56; the deprecated aclTableGroup, field
 * 45, is not supported). Throws std::runtime_error if aclTableGroups is
 * unset/empty, or if no group with that name exists.
 */
cfg::AclTableGroup& findAclTableGroupOrThrow(
    cfg::SwitchConfig& swConfig,
    const std::string& groupName);

} // namespace facebook::fboss::acl_utils
