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
    if (aclTableMap) {
      writableData().aclTableMap() = aclTableMap->toThrift();
    }
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
      fields.aclTableMap_ = std::make_shared<AclTableMap>(*aclTableMap);
    }
    return fields;
  }
  bool operator==(const AclTableGroupFields& other) const {
    return data() == other.data();
  }
  std::shared_ptr<AclTableMap> aclTableMap_;
};

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
  static std::shared_ptr<AclTableGroup> fromFollyDynamic(
      const folly::dynamic& json) {
    auto fields = AclTableGroupFields::fromFollyDynamic(json);
    return std::make_shared<AclTableGroup>(fields.toThrift());
  }

  static std::shared_ptr<AclTableGroup> createDefaultAclTableGroup(
      const folly::dynamic& json) {
    auto fields = AclTableGroupFields::createDefaultAclTableGroupFields(json);
    return std::make_shared<AclTableGroup>(fields.toThrift());
  }

  static std::shared_ptr<AclTableGroup> createDefaultAclTableGroupFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap) {
    auto fields =
        AclTableGroupFields::createDefaultAclTableGroupFieldsFromThrift(
            thriftMap);
    return std::make_shared<AclTableGroup>(fields.toThrift());
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

  folly::dynamic toFollyDynamic() const override {
    auto fields = AclTableGroupFields::fromThrift(toThrift());
    return fields.toFollyDynamic();
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
