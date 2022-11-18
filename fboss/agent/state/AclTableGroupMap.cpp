/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclTableGroupMap.h"

#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

AclTableGroupMap::AclTableGroupMap() {}

AclTableGroupMap::~AclTableGroupMap() {}

std::map<cfg::AclStage, state::AclTableGroupFields> AclTableGroupMap::toThrift()
    const {
  std::map<cfg::AclStage, state::AclTableGroupFields> thriftMap;
  for (const auto& [key, value] : this->getAllNodes()) {
    thriftMap[key] = value->toThrift();
  }
  return thriftMap;
}

std::shared_ptr<AclTableGroupMap> AclTableGroupMap::fromThrift(
    std::map<cfg::AclStage, state::AclTableGroupFields> const& thriftMap) {
  auto aclTableGroupMap = std::make_shared<AclTableGroupMap>();
  for (const auto& [key, value] : thriftMap) {
    aclTableGroupMap->addNode(AclTableGroup::fromThrift(value));
  }
  return aclTableGroupMap;
}

std::shared_ptr<AclTableGroupMap>
AclTableGroupMap::createDefaultAclTableGroupMap(const folly::dynamic& swJson) {
  auto aclTableGroupMap = std::make_shared<AclTableGroupMap>();
  aclTableGroupMap->addNode(AclTableGroup::createDefaultAclTableGroup(swJson));

  return aclTableGroupMap;
}

std::shared_ptr<AclTableGroupMap>
AclTableGroupMap::createDefaultAclTableGroupMapFromThrift(
    std::map<std::string, state::AclEntryFields> const& thriftMap) {
  auto aclTableGroupMap = std::make_shared<AclTableGroupMap>();
  aclTableGroupMap->addNode(
      AclTableGroup::createDefaultAclTableGroupFromThrift(thriftMap));

  return aclTableGroupMap;
}

std::shared_ptr<AclMap> AclTableGroupMap::getDefaultAclTableGroupMap(
    std::map<cfg::AclStage, state::AclTableGroupFields> const& thriftMap) {
  if (thriftMap.find(cfg::AclStage::INGRESS) != thriftMap.end()) {
    auto& aclTableGroup = thriftMap.at(cfg::AclStage::INGRESS);
    return AclTableGroup::getDefaultAclTableGroup(aclTableGroup);
  } else {
    return nullptr;
  }
}

FBOSS_INSTANTIATE_NODE_MAP(AclTableGroupMap, AclTableGroupMapTraits);

} // namespace facebook::fboss
