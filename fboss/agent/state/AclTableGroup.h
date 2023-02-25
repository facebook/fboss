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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <optional>
#include <string>
#include <utility>

namespace facebook::fboss {

USE_THRIFT_COW(AclTableGroup);
RESOLVE_STRUCT_MEMBER(
    AclTableGroup,
    switch_state_tags::aclTableMap,
    AclTableMap)

/*
 * AclTableGroup stores state about a group of ACL tables on the switch as well
 * as additional group attribtues.
 */
class AclTableGroup
    : public ThriftStructNode<AclTableGroup, state::AclTableGroupFields> {
 public:
  using BaseT = ThriftStructNode<AclTableGroup, state::AclTableGroupFields>;

  explicit AclTableGroup(cfg::AclStage stage);

  static std::shared_ptr<AclTableGroup> createDefaultAclTableGroupFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap);

  static std::shared_ptr<AclMap> getDefaultAclTableGroup(
      state::AclTableGroupFields const& aclTableGroupFields) {
    if (auto aclTableMap = aclTableGroupFields.aclTableMap()) {
      return AclTableMap::getDefaultAclTableMap(*aclTableMap);
    } else {
      XLOG(ERR) << "AclTableGroup missing from warmboot state file";
      return nullptr;
    }
  }

  cfg::AclStage getID() const {
    return cref<switch_state_tags::stage>()->cref();
  }

  const std::string& getName() const {
    return cref<switch_state_tags::name>()->cref();
  }

  void setName(const std::string& name) {
    set<switch_state_tags::name>(name);
  }

  std::shared_ptr<const AclTableMap> getAclTableMap() const {
    return get<switch_state_tags::aclTableMap>();
  }

  void setAclTableMap(std::shared_ptr<AclTableMap> aclTableMap) {
    ref<switch_state_tags::aclTableMap>() = std::move(aclTableMap);
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
