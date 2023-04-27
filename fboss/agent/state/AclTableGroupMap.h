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

#include <memory>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/NodeMap.h"

namespace facebook::fboss {

using AclTableGroupMapLegacyTraits =
    NodeMapTraits<cfg::AclStage, AclTableGroup>;

using AclTableGroupMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::enumeration,
    apache::thrift::type_class::structure>;
using AclTableGroupMapThriftType =
    std::map<cfg::AclStage, state::AclTableGroupFields>;

class AclTableGroupMap;
using AclTableGroupMapTraits = ThriftMapNodeTraits<
    AclTableGroupMap,
    AclTableGroupMapTypeClass,
    AclTableGroupMapThriftType,
    AclTableGroup>;

/*
 * A container for the set of tables.
 */
class AclTableGroupMap
    : public ThriftMapNode<AclTableGroupMap, AclTableGroupMapTraits> {
 public:
  using BaseT = ThriftMapNode<AclTableGroupMap, AclTableGroupMapTraits>;
  using Traits = AclTableGroupMapTraits;
  using BaseT::modify;

  AclTableGroupMap();
  virtual ~AclTableGroupMap() override;

  static std::shared_ptr<AclTableGroupMap>
  createDefaultAclTableGroupMapFromThrift(
      std::map<std::string, state::AclEntryFields> const& thriftMap);

  static std::shared_ptr<AclMap> getDefaultAclTableGroupMap(
      std::map<cfg::AclStage, state::AclTableGroupFields> const& thriftMap);

  const std::shared_ptr<AclTableGroup>& getAclTableGroup(
      cfg::AclStage aclStage) const {
    return getNode(aclStage);
  }
  std::shared_ptr<AclTableGroup> getAclTableGroupIf(
      cfg::AclStage aclStage) const {
    return getNodeIf(aclStage);
  }

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void addAclTableGroup(const std::shared_ptr<AclTableGroup>& aclTableGroup) {
    addNode(aclTableGroup);
  }
  void removeAclTableGroup(cfg::AclStage aclStage) {
    removeAclTableGroup(getNode(aclStage));
  }
  void removeAclTableGroup(
      const std::shared_ptr<AclTableGroup>& aclTableGroup) {
    removeNode(aclTableGroup);
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using MultiAclTableGroupMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, AclTableGroupMapTypeClass>;
using MultiAclTableGroupMapThriftType =
    std::map<std::string, AclTableGroupMapThriftType>;

class MultiAclTableGroupMap;

using MultiAclTableGroupMapTraits = ThriftMultiMapNodeTraits<
    MultiAclTableGroupMap,
    MultiAclTableGroupMapTypeClass,
    MultiAclTableGroupMapThriftType,
    AclTableGroupMap>;

class HwSwitchMatcher;

class MultiAclTableGroupMap : public ThriftMultiMapNode<
                                  MultiAclTableGroupMap,
                                  MultiAclTableGroupMapTraits> {
 public:
  using Traits = MultiAclTableGroupMapTraits;
  using BaseT =
      ThriftMultiMapNode<MultiAclTableGroupMap, MultiAclTableGroupMapTraits>;
  using BaseT::modify;

  MultiAclTableGroupMap() {}
  virtual ~MultiAclTableGroupMap() {}

  // ACL map in absence of multi-acl-table support
  std::shared_ptr<AclMap> getAclMap() const;
  // from legacy ACL map thrift
  std::shared_ptr<MultiAclTableGroupMap> fromAclMap(
      const std::map<std::string, state::AclEntryFields>& aclMap);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
