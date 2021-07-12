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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace facebook::fboss {

struct AclTableFields {
  explicit AclTableFields(int priority, const std::string& name)
      : priority(priority), name(name) {}
  explicit AclTableFields(
      int priority,
      const std::string& name,
      std::shared_ptr<AclMap> aclMap)
      : priority(priority), name(name), aclMap(aclMap) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclTableFields fromFollyDynamic(const folly::dynamic& json);
  int priority;
  std::string name;
  std::shared_ptr<AclMap> aclMap;
};

/*
 * AclTable stores state about a table of access control entries on the switch,
 * where the table contains a list of ACLs, a table name, and a priority number.
 */
class AclTable : public NodeBaseT<AclTable, AclTableFields> {
 public:
  explicit AclTable(int priority, const std::string& name);
  static std::shared_ptr<AclTable> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = AclTableFields::fromFollyDynamic(json);
    return std::make_shared<AclTable>(fields);
  }

  static std::shared_ptr<AclTable> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const AclTable& aclTable) const {
    return getFields()->priority == aclTable.getPriority() &&
        getFields()->name == aclTable.getID() &&
        *(getFields()->aclMap) == *(aclTable.getAclMap());
  }

  bool operator!=(const AclTable& aclTable) const {
    return !(*this == aclTable);
  }

  int getPriority() const {
    return getFields()->priority;
  }

  void setPriority(const int priority) {
    writableFields()->priority = priority;
  }

  const std::string& getID() const {
    return getFields()->name;
  }

  std::shared_ptr<AclMap> getAclMap() const {
    return getFields()->aclMap;
  }

  void setAclMap(std::shared_ptr<AclMap> aclMap) {
    writableFields()->aclMap = aclMap;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
