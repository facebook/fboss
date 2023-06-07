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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using AclMapLegacyTraits = NodeMapTraits<std::string, AclEntry>;

using AclMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using AclMapThriftType = std::map<std::string, state::AclEntryFields>;

class AclMap;
using AclMapTraits =
    ThriftMapNodeTraits<AclMap, AclMapTypeClass, AclMapThriftType, AclEntry>;

/*
 * A container for the set of entries.
 */
class AclMap : public ThriftMapNode<AclMap, AclMapTraits> {
 public:
  using Base = ThriftMapNode<AclMap, AclMapTraits>;
  using Traits = AclMapTraits;

  AclMap();
  ~AclMap() override;

  bool operator==(const AclMap& aclMap) const {
    if (numEntries() != aclMap.numEntries()) {
      return false;
    }
    for (auto const& iter : *this) {
      const auto& entry = iter.second;
      if (!aclMap.getEntryIf(entry->getID()) ||
          *(aclMap.getEntry(entry->getID())) != *entry) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const AclMap& aclMap) const {
    return !(*this == aclMap);
  }

  const std::shared_ptr<AclEntry>& getEntry(const std::string& name) const {
    return getNode(name);
  }
  std::shared_ptr<AclEntry> getEntryIf(const std::string& name) const {
    return getNodeIf(name);
  }

  size_t numEntries() const {
    return size();
  }

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void addEntry(const std::shared_ptr<AclEntry>& aclEntry) {
    addNode(aclEntry);
  }
  void removeEntry(const std::string& aclName) {
    removeEntry(getNode(aclName));
  }
  void removeEntry(const std::shared_ptr<AclEntry>& aclEntry) {
    removeNode(aclEntry);
  }

  static const folly::dynamic& getAclMapName(
      const folly::dynamic& aclTableJson) {
    /*
     * Entries data structure is a vector of maps. For our case, its a vector of
     * size 1 and the first entry will contain the AclMap with the acl entries
     * we are interested in. So return the first element
     */
    return aclTableJson[0][kAclMap];
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

/**
 * This container is only used when stateChangedImpl() asks for aclsDelta
 * In S/W, we use `AclMap` which use acl name as key. However, SwitchState
 * update might fail because forEachChanged() will iterate deltaValue based on
 * the order of key. In such case, we update changed acls based on the acl name
 * order. i.e. old acls: [D, C, B] and new acls: [D, A, C, B], we will create
 * A because A is the smallest string in the acl list. But since old C hasn't
 * removed yet, it will try to add A with the same priority with the existing C.
 */
struct PrioAclMapTraits {
  using KeyType = int;
  using Node = AclEntry;
  using ExtraFields = NodeMapNoExtraFields;
  using NodeContainer =
      boost::container::flat_map<KeyType, std::shared_ptr<Node>>;

  static KeyType getKey(const std::shared_ptr<Node>& entry) {
    return entry->getPriority();
  }
};

class PrioAclMap : public NodeMapT<PrioAclMap, PrioAclMapTraits> {
 public:
  PrioAclMap() {}
  ~PrioAclMap() override {}

  void addAcls(const std::shared_ptr<const AclMap>& acls) {
    for (const auto& iter : *acls) {
      addNode(iter.second);
    }
  }

  std::set<cfg::AclTableQualifier> requiredQualifiers() const;

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

using AclMapDelta = NodeMapDelta<
    PrioAclMap,
    DeltaValue<PrioAclMap::Node>,
    MapUniquePointerTraits<PrioAclMap>>;

using MultiSwitchAclMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, AclMapTypeClass>;
using MultiSwitchAclMapThriftType = std::map<std::string, AclMapThriftType>;

class MultiSwitchAclMap;

using MultiSwitchAclMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchAclMap,
    MultiSwitchAclMapTypeClass,
    MultiSwitchAclMapThriftType,
    AclMap>;

class HwSwitchMatcher;

class MultiSwitchAclMap : public ThriftMultiSwitchMapNode<
                              MultiSwitchAclMap,
                              MultiSwitchAclMapTraits> {
 public:
  using Traits = MultiSwitchAclMapTraits;
  using BaseT =
      ThriftMultiSwitchMapNode<MultiSwitchAclMap, MultiSwitchAclMapTraits>;
  using BaseT::modify;

  MultiSwitchAclMap() = default;
  virtual ~MultiSwitchAclMap() = default;

  MultiSwitchAclMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
