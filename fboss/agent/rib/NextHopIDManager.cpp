// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include <boost/functional/hash.hpp>

#include <limits>

namespace std {
size_t hash<facebook::fboss::NextHopIDSet>::operator()(
    const facebook::fboss::NextHopIDSet& idSet) const {
  size_t seed = 0;
  for (const auto& id : idSet) {
    boost::hash_combine(seed, static_cast<uint64_t>(id));
  }
  return seed;
}
} // namespace std

namespace facebook::fboss {

NextHopIDManager::NextHopInfoIter NextHopIDManager::getOrAllocateNextHopID(
    const NextHop& nextHop) {
  auto it = nextHopToIDInfo_.find(nextHop);
  if (it != nextHopToIDInfo_.end()) {
    // Existing NextHop found, increment reference count and return iterator
    it->second.count++;
    return it;
  }

  // New NextHop, allocate a new ID
  NextHopID newID = nextAvailableNextHopID_;
  nextAvailableNextHopID_ = NextHopID(nextAvailableNextHopID_ + 1);

  // Check if the NextHopID is within the range [0, 2^62-1]
  // This is the range assigned to NextHopID
  CHECK(static_cast<int64_t>(newID) > 0 && newID < kNextHopSetIDStart)
      << "Next Hop ID is in the range of [0, 2^62 - 1], the id space has been exhausted! It does not support wrap around!";

  auto [idInfoItr, idInfoInserted] =
      nextHopToIDInfo_.emplace(nextHop, NextHopIDInfo(newID, 1));
  CHECK(idInfoInserted);
  auto [idMapItr, idMapInserted] = idToNextHop_.insert({newID, nextHop});
  CHECK(idMapInserted);
  return idInfoItr;
}

NextHopIDManager::NextHopIdSetIter NextHopIDManager::getOrAllocateNextHopSetID(
    const NextHopIDSet& nextHopIDSet) {
  auto it = nextHopIdSetToIDInfo_.find(nextHopIDSet);
  if (it != nextHopIdSetToIDInfo_.end()) {
    // Existing NextHopIDSet found, increment reference count and return
    // iterator
    it->second.count++;
    return it;
  }

  // New NextHopIDSet, allocate a new ID
  NextHopSetID newID = nextAvailableNextHopSetID_;
  nextAvailableNextHopSetID_ = NextHopSetID(nextAvailableNextHopSetID_ + 1);

  // Check if the NextHopSetID is within the range [2^62, INT64_MAX]
  // This is the range assigned to NextHopSetID
  CHECK(
      static_cast<int64_t>(newID) >= kNextHopSetIDStart &&
      static_cast<int64_t>(newID) < std::numeric_limits<int64_t>::max())
      << "Next Hop Set ID is in the range of [2^62, 2^64-1], the id space has been exhausted! It does not support wrap around!";
  auto [idSetInfoItr, idSetInfoInserted] =
      nextHopIdSetToIDInfo_.emplace(nextHopIDSet, NextHopSetIDInfo(newID, 1));
  CHECK(idSetInfoInserted);
  auto [idSetMapItr, idSetMapInserted] =
      idToNextHopIdSet_.insert({newID, nextHopIDSet});
  CHECK(idSetMapInserted);
  return idSetInfoItr;
}

uint32_t NextHopIDManager::getNextHopRefCount(const NextHop& nextHop) {
  auto it = nextHopToIDInfo_.find(nextHop);
  if (it != nextHopToIDInfo_.end()) {
    return it->second.count;
  }
  return 0;
}

uint32_t NextHopIDManager::getNextHopIDSetRefCount(
    const NextHopIDSet& nextHopIdSet) {
  auto it = nextHopIdSetToIDInfo_.find(nextHopIdSet);
  if (it != nextHopIdSetToIDInfo_.end()) {
    return it->second.count;
  }
  return 0;
}

bool NextHopIDManager::decrOrDeallocateNextHop(const NextHop& nextHop) {
  auto it = nextHopToIDInfo_.find(nextHop);
  if (it == nextHopToIDInfo_.end()) {
    throw FbossError(
        "Cannot decrement reference count or deallocate for non-existent NextHop");
  }
  CHECK_GT(it->second.count, 0);
  it->second.count--;
  if (it->second.count == 0) {
    // Reference count reached 0, deallocate
    auto erasedIdMap = idToNextHop_.erase(it->second.id);
    CHECK_EQ(erasedIdMap, 1);
    auto erasedIdInfo = nextHopToIDInfo_.erase(it->first);
    CHECK_EQ(erasedIdInfo, 1);
    return true;
  }
  return false;
}

bool NextHopIDManager::decrOrDeallocateNextHopIDSet(
    const NextHopIDSet& nextHopIDSet) {
  auto it = nextHopIdSetToIDInfo_.find(nextHopIDSet);
  if (it == nextHopIdSetToIDInfo_.end()) {
    throw FbossError(
        "Cannot decrement reference count or deallocate for non-existent NextHopIDSet");
  }
  CHECK_GT(it->second.count, 0);
  it->second.count--;
  if (it->second.count == 0) {
    // Reference count reached 0, deallocate
    auto erasedIdMap = idToNextHopIdSet_.erase(it->second.id);
    CHECK_EQ(erasedIdMap, 1);
    auto erasedIdInfo = nextHopIdSetToIDInfo_.erase(it->first);
    CHECK_EQ(erasedIdInfo, 1);
    return true;
  }
  return false;
}

std::optional<NextHopSetID> NextHopIDManager::getNextHopSetID(
    const NextHopIDSet& nextHopIDSet) const {
  auto it = nextHopIdSetToIDInfo_.find(nextHopIDSet);
  if (it != nextHopIdSetToIDInfo_.end()) {
    return it->second.id;
  }
  return std::nullopt;
}

std::optional<NextHopID> NextHopIDManager::getNextHopID(
    const NextHop& nextHop) const {
  auto it = nextHopToIDInfo_.find(nextHop);
  if (it != nextHopToIDInfo_.end()) {
    return it->second.id;
  }
  return std::nullopt;
}

NextHopIDManager::NextHopAllocationResult
NextHopIDManager::getOrAllocRouteNextHopSetID(
    const RouteNextHopSet& nextHopSet) {
  NextHopAllocationResult result;

  // Get the NextHopIDs first for each NextHop in the RouteNextHopSet
  NextHopIDSet nextHopIDSet;
  for (const auto& nextHop : nextHopSet) {
    auto nhIter = getOrAllocateNextHopID(nextHop);
    nextHopIDSet.insert(nhIter->second.id);
    // Check if this was a new allocation (count == 1 means newly allocated)
    if (nhIter->second.count == 1) {
      // Track newly allocated NextHopID for the caller to update FibInfo
      // Caller can retrieve NextHop from idToNextHop_ map using this ID
      result.addedNextHopIds.push_back(nhIter->second.id);
    }
  }

  // Get the const iterator for the NextHopIDSet
  result.nextHopIdSetIter = getOrAllocateNextHopSetID(nextHopIDSet);
  return result;
}

NextHopIDManager::NextHopDeallocationResult
NextHopIDManager::decrOrDeallocRouteNextHopSetID(NextHopSetID nextHopSetID) {
  NextHopDeallocationResult result;

  // Lookup the NextHopIDSet from the given NextHopSetID
  auto it = idToNextHopIdSet_.find(nextHopSetID);
  if (it == idToNextHopIdSet_.end()) {
    throw FbossError("Can not delete a non existent NextHopSetID");
  }

  const NextHopIDSet& nextHopIDSet = it->second;

  // Decrement the reference count for each NextHopID in the NextHopIDSet
  for (const auto& nextHopID : nextHopIDSet) {
    auto nextHopIt = idToNextHop_.find(nextHopID);
    CHECK(nextHopIt != idToNextHop_.end())
        << "NextHopID " << nextHopID << " in NextHopIDSet doesn't exist!!";

    auto derefNextHop = decrOrDeallocateNextHop(nextHopIt->second);
    if (derefNextHop) {
      XLOG(DBG3) << "NextHop " << nextHopIt->second << " deallocated";
      // Track deallocated NextHops for the caller to update FibInfo
      result.removedNextHopIds.push_back(nextHopID);
    }
  }
  // Decrement the reference count for the NextHopSetID
  bool derefNextHopIDSet = decrOrDeallocateNextHopIDSet(nextHopIDSet);
  if (derefNextHopIDSet) {
    XLOG(DBG3) << "NextHopSetID " << nextHopSetID << " deallocated";
    // Track deallocated set for the caller to update FibInfo
    result.removedSetId = nextHopSetID;
  }

  return result;
}

NextHopIDManager::NextHopUpdateResult NextHopIDManager::updateRouteNextHopSetID(
    NextHopSetID nextHopSetID,
    const RouteNextHopSet& newNextHopSet) {
  NextHopUpdateResult result;

  // Call delete route to decrement the reference count for the old
  result.deallocation = decrOrDeallocRouteNextHopSetID(nextHopSetID);

  // Call getRouteNextHopSetID to get the NextHopSetID for the new
  // NextHopSet
  result.allocation = getOrAllocRouteNextHopSetID(newNextHopSet);

  return result;
}

} // namespace facebook::fboss
