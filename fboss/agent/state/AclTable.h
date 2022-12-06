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
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace facebook::fboss {

struct AclTableFields
    : public ThriftyFields<AclTableFields, state::AclTableFields> {
  using ThriftyFields::ThriftyFields;
  explicit AclTableFields(int priority, const std::string& name) {
    writableData().id() = name;
    writableData().priority() = priority;
  }
  AclTableFields(
      int priority,
      const std::string& name,
      std::shared_ptr<AclMap> aclMap) {
    writableData().id() = name;
    writableData().priority() = priority;
    writableData().aclMap() = aclMap->toThrift();
    aclMap_ = aclMap;
  }

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclTableFields fromFollyDynamic(const folly::dynamic& json);
  static AclTableFields createDefaultAclTableFields(const folly::dynamic& json);
  static AclTableFields createDefaultAclTableFieldsFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap);

  state::AclTableFields toThrift() const override {
    return data();
  }
  static AclTableFields fromThrift(
      state::AclTableFields const& aclTableFields) {
    AclTableFields fields = AclTableFields(aclTableFields);
    if (auto aclMap = aclTableFields.aclMap()) {
      fields.aclMap_ = std::make_shared<AclMap>(*aclMap);
    }
    return fields;
  }
  bool operator==(const AclTableFields& other) const {
    return data() == other.data();
  }

  std::shared_ptr<AclMap> aclMap_;
};

USE_THRIFT_COW(AclTable);
RESOLVE_STRUCT_MEMBER(AclTable, switch_state_tags::aclMap, AclMap)

/*
 * AclTable stores state about a table of access control entries on the switch,
 * where the table contains a list of ACLs, a table name, and a priority number.
 */
class AclTable : public ThriftStructNode<AclTable, state::AclTableFields> {
 public:
  using Base = ThriftStructNode<AclTable, state::AclTableFields>;
  explicit AclTable(int priority, const std::string& name);
  static std::shared_ptr<AclTable> fromFollyDynamic(
      const folly::dynamic& json) {
    auto fields = AclTableFields::fromFollyDynamic(json);
    return std::make_shared<AclTable>(fields.toThrift());
  }

  static std::shared_ptr<AclTable> createDefaultAclTable(
      const folly::dynamic& json) {
    auto fields = AclTableFields::createDefaultAclTableFields(json);
    return std::make_shared<AclTable>(fields.toThrift());
  }

  static std::shared_ptr<AclTable> createDefaultAclTableFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap) {
    auto fields =
        AclTableFields::createDefaultAclTableFieldsFromThrift(thriftMap);
    return std::make_shared<AclTable>(fields.toThrift());
  }

  static const folly::dynamic& getAclTableName(
      const folly::dynamic& aclTableJson) {
    return AclMap::getAclMapName(aclTableJson[kEntries]);
  }

  static std::shared_ptr<AclMap> getDefaultAclTable(
      state::AclTableFields const& aclTableFields) {
    if (auto aclMap = aclTableFields.aclMap()) {
      return std::make_shared<AclMap>(*aclMap);
    } else {
      XLOG(ERR) << "AclTable missing from warmboot state file";
      return nullptr;
    }
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = AclTableFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
  }

  int getPriority() const {
    return cref<switch_state_tags::priority>()->cref();
  }

  void setPriority(const int priority) {
    set<switch_state_tags::priority>(priority);
  }

  const std::string& getID() const {
    return cref<switch_state_tags::id>()->cref();
  }

  auto getAclMap() const {
    return safe_cref<switch_state_tags::aclMap>();
  }

  void setAclMap(std::shared_ptr<AclMap> aclMap) {
    ref<switch_state_tags::aclMap>() = aclMap;
  }

  // THRIFT_COPY
  std::vector<cfg::AclTableActionType> getActionTypes() const {
    return cref<switch_state_tags::actionTypes>()->toThrift();
  }

  void setActionTypes(const std::vector<cfg::AclTableActionType>& actionTypes) {
    set<switch_state_tags::actionTypes>(actionTypes);
  }

  // THRIFT_COPY
  std::vector<cfg::AclTableQualifier> getQualifiers() const {
    return cref<switch_state_tags::qualifiers>()->toThrift();
  }

  void setQualifiers(const std::vector<cfg::AclTableQualifier>& qualifiers) {
    set<switch_state_tags::qualifiers>(qualifiers);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
