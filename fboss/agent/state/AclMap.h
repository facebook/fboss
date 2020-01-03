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

#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using AclMapTraits = NodeMapTraits<std::string, AclEntry>;
/*
 * A container for the set of entries.
 */
class AclMap : public NodeMapT<AclMap, AclMapTraits> {
 public:
  AclMap();
  ~AclMap() override;

  const std::shared_ptr<AclEntry>& getEntry(const std::string& name) const {
    return getNode(name);
  }
  std::shared_ptr<AclEntry> getEntryIf(const std::string& name) const {
    return getNodeIf(name);
  }

  size_t numEntries() const {
    return size();
  }

  AclMap* modify(std::shared_ptr<SwitchState>* state);

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

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
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

  static KeyType getKey(const std::shared_ptr<Node>& entry) {
    return entry->getPriority();
  }
};
class PrioAclMap : public NodeMapT<PrioAclMap, PrioAclMapTraits> {
 public:
  PrioAclMap() {}
  ~PrioAclMap() override {}

  void addAcls(const std::shared_ptr<AclMap>& acls) {
    for (const auto& aclEntry : *acls) {
      addNode(aclEntry);
    }
  }

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

using AclMapDelta = NodeMapDelta<
    PrioAclMap,
    DeltaValue<PrioAclMap::Node>,
    MapUniquePointerTraits<PrioAclMap>>;

} // namespace facebook::fboss
