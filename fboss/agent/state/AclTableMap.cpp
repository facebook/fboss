/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/AclMap.h"

#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/state/SwitchState.h"

namespace {
constexpr auto kAclTable1 = "AclTable1";
}

namespace facebook::fboss {

AclTableMap::AclTableMap() {}

AclTableMap::~AclTableMap() {}

std::map<std::string, state::AclTableFields> AclTableMap::toThrift() const {
  std::map<std::string, state::AclTableFields> thriftMap;
  for (const auto& [key, value] : this->getAllNodes()) {
    thriftMap[key] = value->toThrift();
  }
  return thriftMap;
}

std::shared_ptr<AclTableMap> AclTableMap::fromThrift(
    std::map<std::string, state::AclTableFields> const& thriftMap) {
  auto aclTableMap = std::make_shared<AclTableMap>();
  for (const auto& [key, value] : thriftMap) {
    auto node = std::make_shared<AclTable>();
    node->fromThrift(value);
    aclTableMap->addNode(std::move(node));
  }
  return aclTableMap;
}

std::shared_ptr<AclTableMap> AclTableMap::createDefaultAclTableMap(
    const folly::dynamic& swJson) {
  auto aclTableMap = std::make_shared<AclTableMap>();
  aclTableMap->addNode(AclTable::createDefaultAclTable(swJson));
  return aclTableMap;
}

std::shared_ptr<AclTableMap> AclTableMap::createDefaultAclTableMapFromThrift(
    std::map<std::string, state::AclEntryFields> const& thriftMap) {
  auto aclTableMap = std::make_shared<AclTableMap>();
  aclTableMap->addNode(AclTable::createDefaultAclTableFromThrift(thriftMap));
  return aclTableMap;
}

std::shared_ptr<AclMap> AclTableMap::getDefaultAclTableMap(
    std::map<std::string, state::AclTableFields> const& thriftMap) {
  if (thriftMap.find(kAclTable1) != thriftMap.end()) {
    auto aclTable = thriftMap.at(kAclTable1);
    return AclTable::getDefaultAclTable(aclTable);
  } else {
    XLOG(ERR) << "AclTableMap missing from warmboot state file";
    return nullptr;
  }
}
FBOSS_INSTANTIATE_NODE_MAP(AclTableMap, AclTableMapTraits);

} // namespace facebook::fboss
