// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/OverrideSetIdManager.h"

#include <glog/logging.h>

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

OverrideSetIdManager::MembersToIdIter OverrideSetIdManager::getOrAllocate(
    const NextHopIDSet& members) {
  if (members.empty()) {
    throw FbossError("Cannot allocate an override SetID for an empty set");
  }
  auto it = membersToId_.find(members);
  if (it != membersToId_.end()) {
    return it;
  }

  NextHopSetID newID{nextAvailable_++};
  CHECK(isOverrideSetID(newID))
      << "Override NextHopSetID space exhausted, no wrap around: " << newID;
  auto [membersItr, membersInserted] = membersToId_.emplace(members, newID);
  CHECK(membersInserted);
  auto [idItr, idInserted] = idToMembers_.emplace(newID, members);
  CHECK(idInserted);
  return membersItr;
}

} // namespace facebook::fboss
