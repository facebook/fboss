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
  explicit AclTableGroupFields(const std::string& name) : name(name) {}
  explicit AclTableGroupFields(
      const std::string& name,
      std::shared_ptr<AclTableMap> aclTableMap)
      : name(name), aclTableMap(aclTableMap) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclTableGroupFields fromFollyDynamic(const folly::dynamic& json);
  std::string name;
  std::shared_ptr<AclTableMap> aclTableMap;
};

/*
 * AclTableGroup stores state about a group of ACL tables on the switch as well
 * as additional group attribtues.
 */
class AclTableGroup : public NodeBaseT<AclTableGroup, AclTableGroupFields> {
 public:
  explicit AclTableGroup(const std::string& name);
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
    return getFields()->name == aclTableGroup.getID() &&
        getFields()->aclTableMap == aclTableGroup.getAclTableMap();
  }

  const std::string& getID() const {
    return getFields()->name;
  }

  std::shared_ptr<AclTableMap> getAclTableMap() const {
    return getFields()->aclTableMap;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
