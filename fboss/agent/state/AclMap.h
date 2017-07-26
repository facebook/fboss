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

#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/AclEntry.h"

namespace facebook { namespace fboss {

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

}} // facebook::fboss
