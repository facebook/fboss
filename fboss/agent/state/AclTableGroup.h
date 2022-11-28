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
#include "fboss/agent/state/AclTableMap.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <optional>
#include <string>
#include <utility>

namespace facebook::fboss {

struct AclTableGroupFields
    : public ThriftyFields<AclTableGroupFields, state::AclTableGroupFields> {
  using ThriftyFields::ThriftyFields;
  explicit AclTableGroupFields(cfg::AclStage stage) {
    writableData().stage() = stage;
  }
  AclTableGroupFields(
      cfg::AclStage stage,
      const std::string& name,
      std::shared_ptr<AclTableMap> aclTableMap) {
    writableData().stage() = stage;
    writableData().name() = name;
    writableData().aclTableMap() = aclTableMap->toThrift();
    aclTableMap_ = aclTableMap;
  }

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclTableGroupFields fromFollyDynamic(const folly::dynamic& json);
  static AclTableGroupFields createDefaultAclTableGroupFields(
      const folly::dynamic& swJson);
  static AclTableGroupFields createDefaultAclTableGroupFieldsFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap);
  state::AclTableGroupFields toThrift() const override {
    return data();
  }
  static AclTableGroupFields fromThrift(
      state::AclTableGroupFields const& aclTableGroupFields) {
    auto fields = AclTableGroupFields(aclTableGroupFields);
    if (auto aclTableMap = aclTableGroupFields.aclTableMap()) {
      fields.aclTableMap_ = AclTableMap::fromThrift(*aclTableMap);
    }
    return AclTableGroupFields(aclTableGroupFields);
  }
  bool operator==(const AclTableGroupFields& other) const {
    return data() == other.data();
  }
  std::shared_ptr<AclTableMap> aclTableMap_;
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

  static std::shared_ptr<AclTableGroup> createDefaultAclTableGroup(
      const folly::dynamic& json) {
    const auto& fields =
        AclTableGroupFields::createDefaultAclTableGroupFields(json);
    return std::make_shared<AclTableGroup>(fields);
  }

  static std::shared_ptr<AclTableGroup> createDefaultAclTableGroupFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap) {
    const auto& fields =
        AclTableGroupFields::createDefaultAclTableGroupFieldsFromThrift(
            thriftMap);
    return std::make_shared<AclTableGroup>(fields);
  }

  static const folly::dynamic& getAclTableGroupName(
      const folly::dynamic& aclTableGroupJson) {
    return AclTableMap::getAclTableMapName(aclTableGroupJson[kEntries]);
  }

  static std::shared_ptr<AclMap> getDefaultAclTableGroup(
      state::AclTableGroupFields const& aclTableGroupFields) {
    if (auto aclTableMap = aclTableGroupFields.aclTableMap()) {
      return AclTableMap::getDefaultAclTableMap(*aclTableMap);
    } else {
      XLOG(ERR) << "AclTableGroup missing from warmboot state file";
      return nullptr;
    }
  }

  static std::shared_ptr<AclTableGroup> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  state::AclTableGroupFields toThrift() const {
    return getFields()->toThrift();
  }
  static std::shared_ptr<AclTableGroup> fromThrift(
      state::AclTableGroupFields const& aclTableGroupFields) {
    auto aclTableGroup = std::make_shared<AclTableGroup>(
        AclTableGroupFields(aclTableGroupFields));
    if (auto aclTableMap = aclTableGroupFields.aclTableMap()) {
      aclTableGroup->writableFields()->aclTableMap_ =
          AclTableMap::fromThrift(*aclTableMap);
    }
    return aclTableGroup;
  }

  bool operator==(const AclTableGroup& other) const {
    return *getFields() == *other.getFields();
  }

  bool operator!=(const AclTableGroup& other) const {
    return !(*this == other);
  }

  cfg::AclStage getID() const {
    return *getFields()->data().stage();
  }

  const std::string& getName() const {
    return *getFields()->data().name();
  }

  void setName(const std::string& name) {
    writableFields()->writableData().name() = name;
  }

  std::shared_ptr<const AclTableMap> getAclTableMap() const {
    return getFields()->aclTableMap_;
  }

  void setAclTableMap(std::shared_ptr<AclTableMap> aclTableMap) {
    writableFields()->aclTableMap_ = aclTableMap;
    writableFields()->writableData().aclTableMap() = aclTableMap->toThrift();
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
