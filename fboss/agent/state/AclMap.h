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

namespace facebook { namespace fboss {

class AclEntry;
using AclMapTraits = NodeMapTraits<AclEntryID, AclEntry>;

/*
 * A container for the set of entries.
 */
class AclMap : public NodeMapT<AclMap, AclMapTraits> {
 public:
  AclMap();
  ~AclMap() override;

  const std::shared_ptr<AclEntry>& getEntry(AclEntryID id) const {
    LOG(INFO) << id;
    LOG(INFO) << size();
    return getNode(id);
  }
  std::shared_ptr<AclEntry> getEntryIf(AclEntryID id) const {
    return getNodeIf(id);
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

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

}} // facebook::fboss
