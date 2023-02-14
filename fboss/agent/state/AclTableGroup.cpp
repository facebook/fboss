/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclTableGroup.h"
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclTable.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/StateUtils.h"

using apache::thrift::TEnumTraits;
using folly::IPAddress;

namespace {
constexpr auto kAclStage = "aclStage";
constexpr auto kName = "name";
constexpr auto kAclTableGroupName = "ingress-ACL-Table-Group";
} // namespace

namespace facebook::fboss {

AclTableGroupFields
AclTableGroupFields::createDefaultAclTableGroupFieldsFromThrift(
    std::map<std::string, state::AclEntryFields> const& thriftMap) {
  auto aclTableMap = AclTableMap::createDefaultAclTableMapFromThrift(thriftMap);
  AclTableGroupFields aclTableGroup(
      cfg::AclStage::INGRESS, kAclTableGroupName, aclTableMap);

  return aclTableGroup;
}

AclTableGroup::AclTableGroup(cfg::AclStage stage) {
  set<switch_state_tags::stage>(stage);
}

template class ThriftStructNode<AclTableGroup, state::AclTableGroupFields>;
} // namespace facebook::fboss
