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

struct AclTableGroupFields {
  explicit AclTableGroupFields(cfg::AclStage stage) : stage(stage) {}
  explicit AclTableGroupFields(
      cfg::AclStage stage,
      const std::string& name,
      std::shared_ptr<AclTableMap> aclTableMap)
      : stage(stage), name(name), aclTableMap(aclTableMap) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclTableGroupFields fromFollyDynamic(const folly::dynamic& json);

  cfg::AclStage stage;
  std::string name;
  std::shared_ptr<AclTableMap> aclTableMap;
};

/*
 * AclTableGroup stores state about a group of ACL tables on the switch as well
 * as additional group attribtues.
 */
class AclTableGroup : public NodeBaseT<AclTableGroup, AclTableGroupFields> {
 public:
  explicit AclTableGroup(cfg::AclStage stage);
  static std::shared_ptr<AclTableGroup> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = AclTableGroupFields::fromFollyDynamic(json);
    return std::make_shared<AclTableGroup>(fields);
  }

  static std::shared_ptr<AclTableGroup> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const AclTableGroup& aclTableGroup) const {
    return getFields()->stage == aclTableGroup.getID() &&
        *(getFields()->aclTableMap) == *(aclTableGroup.getAclTableMap()) &&
        getFields()->name == aclTableGroup.getName();
  }

  bool operator!=(const AclTableGroup& aclTableGroup) const {
    return !(*this == aclTableGroup);
  }

  cfg::AclStage getID() const {
    return getFields()->stage;
  }

  const std::string& getName() const {
    return getFields()->name;
  }

  void setName(const std::string& name) {
    writableFields()->name = name;
  }

  std::shared_ptr<AclTableMap> getAclTableMap() const {
    return getFields()->aclTableMap;
  }

  void setAclTableMap(std::shared_ptr<AclTableMap> aclTableMap) {
    writableFields()->aclTableMap = aclTableMap;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
