// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include <boost/functional/hash.hpp>

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

std::pair<NextHopID, bool> NextHopIDManager::getOrAllocateNextHopID(
    const NextHop& nextHop) {
  auto it = nextHopToIDInfo_.find(nextHop);
  if (it != nextHopToIDInfo_.end()) {
    // Existing NextHop found, increment reference count and return existing ID
    it->second.count++;
    return {it->second.id, false};
  }

  // New NextHop, allocate a new ID
  auto newID = nextAvailableNextHopID_;
  nextAvailableNextHopID_ = NextHopID(nextAvailableNextHopID_ + 1);

  // Check if the NextHopID is within the range [0, 2^63-1]
  // This is the range assigned to NextHopID
  CHECK(static_cast<uint64_t>(newID) < (1ULL << 63))
      << "Next Hop ID is in the range of [0, 2^63 - 1], the id space has been exhausted! It does not support wrap around!";

  auto [idInfoitr, idInfoinserted] =
      nextHopToIDInfo_.emplace(nextHop, NextHopIDInfo(newID, 1));
  CHECK(idInfoinserted);
  auto [idMapItr, idMapInserted] = idToNextHop_.insert({newID, nextHop});
  CHECK(idMapInserted);
  return {newID, true};
}

std::pair<NextHopSetID, bool> NextHopIDManager::getOrAllocateNextHopSetID(
    const NextHopIDSet& nextHopIDSet) {
  auto it = nextHopIdSetToIDInfo_.find(nextHopIDSet);
  if (it != nextHopIdSetToIDInfo_.end()) {
    // Existing NextHopIDSet found, increment reference count and return
    // existing ID
    it->second.count++;
    return {it->second.id, false};
  }

  // New NextHopIDSet, allocate a new ID
  auto newID = nextAvailableNextHopSetID_;
  nextAvailableNextHopSetID_ = NextHopSetID(nextAvailableNextHopSetID_ + 1);

  // Check if the NextHopSetID is within the range [2^63, 2^64-1]
  // This is the range assigned to NextHopSetID
  CHECK(static_cast<uint64_t>(newID) >= (1ULL << 63))
      << "Next Hop Set ID is in the range of [2^63, 2^64-1], the id space has been exhausted! It does not support wrap around!";
  auto [idSetInfoItr, idSetInfoInserted] =
      nextHopIdSetToIDInfo_.emplace(nextHopIDSet, NextHopSetIDInfo(newID, 1));
  CHECK(idSetInfoInserted);
  auto [idSetMapItr, idSetMapInserted] =
      idToNextHopIdSet_.insert({newID, nextHopIDSet});
  CHECK(idSetMapInserted);
  return {newID, true};
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

NextHopSetID NextHopIDManager::getOrAllocRouteNextHopSetID(
    const RouteNextHopSet& nextHopSet) {
  // Get the NextHopIDs first for each NextHop in the RouteNextHopSet
  NextHopIDSet nextHopIDSet;
  for (const auto& nextHop : nextHopSet) {
    auto [nextHopID, nhopIDAllocated] = getOrAllocateNextHopID(nextHop);
    nextHopIDSet.insert(nextHopID);
  }

  // Get the NextHopSetID for the NextHopIDSet
  auto [nextHopSetID, setIdAllocated] = getOrAllocateNextHopSetID(nextHopIDSet);
  return nextHopSetID;
}

bool NextHopIDManager::decrOrDeallocRouteNextHopSetID(
    NextHopSetID nextHopSetID) {
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

    auto nhopDeallocated = decrOrDeallocateNextHop(nextHopIt->second);
    if (nhopDeallocated) {
      XLOG(DBG3) << "NextHop " << nextHopIt->second << " deallocated";
    }
  }
  // Decrement the reference count for the NextHopSetID
  bool nhopSetIDDeallocated = decrOrDeallocateNextHopIDSet(nextHopIDSet);
  if (nhopSetIDDeallocated) {
    XLOG(DBG3) << "NextHopSetID " << nextHopSetID << " deallocated";
  }

  return nhopSetIDDeallocated;
}

NextHopSetID NextHopIDManager::updateRouteNextHopSetID(
    NextHopSetID nextHopSetID,
    const RouteNextHopSet& newNextHopSet) {
  // Call delete route to decrement the reference count for the old
  decrOrDeallocRouteNextHopSetID(nextHopSetID);

  // Call getRouteNextHopSetID to get the NextHopSetID for the new
  // NextHopSet
  return getOrAllocRouteNextHopSetID(newNextHopSet);
}

} // namespace facebook::fboss
