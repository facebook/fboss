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
} // namespace

namespace facebook::fboss {

folly::dynamic AclTableFields::toFollyDynamic() const {
  // TODO(saranicholas): write aclMap field to output json
  folly::dynamic aclTable = folly::dynamic::object;
  aclTable[kPriority] = priority;
  aclTable[kName] = name;
  return aclTable;
}

AclTableFields AclTableFields::fromFollyDynamic(
    const folly::dynamic& aclTableJson) {
  // TODO(saranicholas): read aclMap field from input json
  AclTableFields aclTable(
      aclTableJson[kPriority].asInt(), aclTableJson[kName].asString());

  return aclTable;
}

AclTable::AclTable(int priority, const std::string& name)
    : NodeBaseT(priority, name) {}

template class NodeBaseT<AclTable, AclTableFields>;

} // namespace facebook::fboss
