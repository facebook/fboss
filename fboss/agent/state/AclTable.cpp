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

folly::dynamic AclTableFields::toFollyDynamic() const {
  folly::dynamic aclTable = folly::dynamic::object;
  aclTable[kPriority] = *data().priority();
  aclTable[kName] = *data().id();
  if (aclMap_) {
    aclTable[kAclMap] = aclMap_->toFollyDynamic();
  }

  aclTable[kActionTypes] = folly::dynamic::array;
  for (const auto& actionType : *data().actionTypes()) {
    aclTable[kActionTypes].push_back(static_cast<int>(actionType));
  }

  aclTable[kQualifiers] = folly::dynamic::array;
  for (const auto& qualifier : *data().qualifiers()) {
    aclTable[kQualifiers].push_back(static_cast<int>(qualifier));
  }

  return aclTable;
}

AclTableFields AclTableFields::fromFollyDynamic(
    const folly::dynamic& aclTableJson) {
  std::shared_ptr<AclMap> aclMap;
  if (aclTableJson.find(kAclMap) != aclTableJson.items().end()) {
    aclMap = AclMap::fromFollyDynamic(aclTableJson[kAclMap]);
  }
  AclTableFields aclTable(
      aclTableJson[kPriority].asInt(), aclTableJson[kName].asString(), aclMap);

  if (aclTableJson.find(kActionTypes) != aclTableJson.items().end()) {
    for (const auto& entry : aclTableJson[kActionTypes]) {
      auto actionType = cfg::AclTableActionType(entry.asInt());
      aclTable.writableData().actionTypes()->push_back(actionType);
    }
  }

  if (aclTableJson.find(kQualifiers) != aclTableJson.items().end()) {
    for (const auto& entry : aclTableJson[kQualifiers]) {
      auto qualifier = cfg::AclTableQualifier(entry.asInt());
      aclTable.writableData().qualifiers()->push_back(qualifier);
    }
  }

  return aclTable;
}

/* Create Default ACL table similar to the one created in Sai code today.
 * Leave the ACL Table and qualifiers empty to be populated during Delta
 * processing */
AclTableFields AclTableFields::createDefaultAclTableFields(
    const folly::dynamic& swJson) {
  std::shared_ptr<AclMap> aclMap;

  aclMap = AclMap::fromFollyDynamic(swJson);
  AclTableFields aclTable(kAclTablePriority, kAclTable1, aclMap);

  return aclTable;
}

AclTableFields AclTableFields::createDefaultAclTableFieldsFromThrift(
    std::map<std::string, state::AclEntryFields> const& thriftMap) {
  std::shared_ptr<AclMap> aclMap = std::make_shared<AclMap>(thriftMap);
  AclTableFields aclTable(kAclTablePriority, kAclTable1, aclMap);
  return aclTable;
}

AclTable::AclTable(int priority, const std::string& name)
    : NodeBaseT(priority, name) {}

template class NodeBaseT<AclTable, AclTableFields>;

} // namespace facebook::fboss
