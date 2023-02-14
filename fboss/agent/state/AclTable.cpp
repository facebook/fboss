/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclTable.h"
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/StateUtils.h"

using apache::thrift::TEnumTraits;
using folly::IPAddress;

namespace {
constexpr auto kPriority = "priority";
constexpr auto kName = "name";
constexpr auto kAclMap = "aclMap";
constexpr auto kActionTypes = "actionTypes";
constexpr auto kQualifiers = "qualifiers";
constexpr auto kAcls = "acls";
// Same Priority and name as the default table created */
constexpr auto kAclTablePriority = 0;
constexpr auto kAclTable1 = "AclTable1";
} // namespace

namespace facebook::fboss {

AclTableFields AclTableFields::createDefaultAclTableFieldsFromThrift(
    std::map<std::string, state::AclEntryFields> const& thriftMap) {
  std::shared_ptr<AclMap> aclMap = std::make_shared<AclMap>(thriftMap);
  AclTableFields aclTable(kAclTablePriority, kAclTable1, aclMap);
  return aclTable;
}

AclTable::AclTable(int priority, const std::string& name) {
  set<switch_state_tags::priority>(priority);
  set<switch_state_tags::id>(name);
}

template class ThriftStructNode<AclTable, state::AclTableFields>;

} // namespace facebook::fboss
