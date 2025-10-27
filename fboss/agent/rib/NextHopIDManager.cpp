// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/FbossError.h"

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

} // namespace facebook::fboss
