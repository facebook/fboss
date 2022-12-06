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

  static std::shared_ptr<AclTable> createDefaultAclTable(
      const folly::dynamic& json) {
    const auto& fields = AclTableFields::createDefaultAclTableFields(json);
    return std::make_shared<AclTable>(fields);
  }

  static std::shared_ptr<AclTable> createDefaultAclTableFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap) {
    const auto& fields =
        AclTableFields::createDefaultAclTableFieldsFromThrift(thriftMap);
    return std::make_shared<AclTable>(fields);
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

  static std::shared_ptr<AclTable> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  state::AclTableFields toThrift() const {
    return getFields()->toThrift();
  }
  static std::shared_ptr<AclTable> fromThrift(
      state::AclTableFields const& aclTableFields) {
    auto aclTable = std::make_shared<AclTable>(AclTableFields(aclTableFields));
    if (auto aclMap = aclTableFields.aclMap()) {
      aclTable->writableFields()->aclMap_ = std::make_shared<AclMap>(*aclMap);
    }
    return aclTable;
  }
  bool operator==(const AclTable& other) const {
    return *getFields() == *other.getFields();
  }

  bool operator!=(const AclTable& aclTable) const {
    return !(*this == aclTable);
  }

  int getPriority() const {
    return *getFields()->data().priority();
  }

  void setPriority(const int priority) {
    writableFields()->writableData().priority() = priority;
  }

  const std::string& getID() const {
    return *getFields()->data().id();
  }

  std::shared_ptr<const AclMap> getAclMap() const {
    return getFields()->aclMap_;
  }

  void setAclMap(std::shared_ptr<AclMap> aclMap) {
    writableFields()->aclMap_ = aclMap;
    writableFields()->writableData().aclMap() = aclMap->toThrift();
  }

  std::vector<cfg::AclTableActionType> getActionTypes() const {
    return *getFields()->data().actionTypes();
  }

  void setActionTypes(const std::vector<cfg::AclTableActionType>& actionTypes) {
    writableFields()->writableData().actionTypes() = actionTypes;
  }

  std::vector<cfg::AclTableQualifier> getQualifiers() const {
    return *getFields()->data().qualifiers();
  }

  void setQualifiers(const std::vector<cfg::AclTableQualifier>& qualifiers) {
    writableFields()->writableData().qualifiers() = qualifiers;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
