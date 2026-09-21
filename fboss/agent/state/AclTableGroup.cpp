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

#include "fboss/agent/gen-cpp2/switch_config_constants.h"

namespace facebook::fboss {

AclTableGroup::AclTableGroup(cfg::AclStage stage) {
  set<switch_state_tags::stage>(stage);
}

std::shared_ptr<AclTableGroup>
AclTableGroup::createDefaultAclTableGroupFromThrift(
    const std::map<std::string, state::AclEntryFields>& aclMap) {
  auto aclTableMap = AclTableMap::createDefaultAclTableMapFromThrift(aclMap);
  state::AclTableGroupFields data{};
  data.stage() = cfg::AclStage::INGRESS;
  data.name() = cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE_GROUP();
  data.aclTableMap() = aclTableMap->toThrift();
  return std::make_shared<AclTableGroup>(data);
}

template struct ThriftStructNode<AclTableGroup, state::AclTableGroupFields>;
} // namespace facebook::fboss
