// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gtest/gtest.h>
#include <cstdint>
#include <unordered_map>
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

// A set of NextHopIDs (used as key for NextHopIDSetID mapping)
using NextHopIDSet = std::set<NextHopID>;

} // namespace facebook::fboss

// Hash function for NextHopIDSet to be used in unordered_map
namespace std {
template <>
struct hash<facebook::fboss::NextHopIDSet> {
  size_t operator()(const facebook::fboss::NextHopIDSet& idSet) const;
};
} // namespace std

namespace facebook::fboss {

/**
 * NextHopIDManager is responsible for generating and managing unique IDs
 * for NextHops and NextHopSets. It maintains reference counts for each
 * assigned ID to track usage.
 */
class NextHopIDManager {
 public:
  NextHopIDManager() = default;

  // Get or allocate a NextHopID for the given NextHop
  // Returns pair of (NextHopID, bool) where bool is true if new ID was
  // allocated
  std::pair<NextHopID, bool> getOrAllocateNextHopID(const NextHop& nextHop);

  // Get or allocate a NextHopSetID for the given set of NextHopIDs
  // Returns pair of (NextHopSetID, bool) where bool is true if new ID was
  // allocated
  std::pair<NextHopSetID, bool> getOrAllocateNextHopSetID(
      const NextHopIDSet& nextHopIDSet);

  // Decrement reference count for a NextHop and deallocate if count reaches 0
  // Returns true if deallocated, false otherwise
  bool decrOrDeallocateNextHop(const NextHop& nextHop);

  // Decrement reference count for a set of NextHopIDs and deallocate if count
  // reaches 0. Returns true if deallocated, false otherwise
  bool decrOrDeallocateNextHopIDSet(const NextHopIDSet& nextHopIDSet);

  // Get the reverse lookup map from NextHopID to NextHop
  const std::unordered_map<NextHopID, NextHop>& getIdToNextHop() const {
    return idToNextHop_;
  }

  // Get the reverse lookup map from NextHopSetID to NextHopIDSet
  const std::unordered_map<NextHopSetID, NextHopIDSet>& getIdToNextHopIdSet()
      const {
    return idToNextHopIdSet_;
  }

  // Retrieves or allocate NextHopSetID for a RouteNextHopSet
  NextHopSetID getOrAllocRouteNextHopSetID(const RouteNextHopSet& nextHopSet);

  // Decrements or deallcoates NextHopSetID for a RouteNextHopSet
  bool decrOrDeallocRouteNextHopSetID(NextHopSetID nextHopSetID);

  // Update from old RouteNextHopSet to new RouteNextHopSet
  // Decrements old, allocates/increments new
  NextHopSetID updateRouteNextHopSetID(
      NextHopSetID oldNextHopSetID,
      const RouteNextHopSet& newNextHopSet);

 private:
  // Structure to hold ID and reference count for NextHops
  // Count will be 0 when initialised
  struct NextHopIDInfo {
    NextHopID id;
    uint32_t count = 0;

    NextHopIDInfo(NextHopID id_, uint32_t count_) : id(id_), count(count_) {}
  };

  // Structure to hold ID and reference count for NextHopSets
  struct NextHopSetIDInfo {
    NextHopSetID id;
    uint32_t count;

    NextHopSetIDInfo(NextHopSetID id_, uint32_t count_)
        : id(id_), count(count_) {}
  };

  // Counter for generating NextHop IDs, starting from 1
  // allocate 0 - (2^63 -1) IDs for NextHops.
  NextHopID nextAvailableNextHopID_{1};

  // Counter for generating NextHopSet IDs, starting from 2^63
  // Allocate 2^63 - (2^64 - 1) IDs for NextHopIDSets.
  NextHopSetID nextAvailableNextHopSetID_{1ULL << 63};

  // Mapping from NextHop to its ID and reference count
  std::unordered_map<NextHop, NextHopIDInfo> nextHopToIDInfo_;

  // Mapping from set of NextHopIDs to its NextHopSetID and reference count
  std::unordered_map<NextHopIDSet, NextHopSetIDInfo> nextHopIdSetToIDInfo_;

  // Reverse lookup maps for clients to convert IDs back to NextHops
  std::unordered_map<NextHopID, NextHop> idToNextHop_;

  // Map from NextHopSetID to set of NextHopIDs
  std::unordered_map<NextHopSetID, NextHopIDSet> idToNextHopIdSet_;

  // Get the ref count for a given NextHop
  uint32_t getNextHopRefCount(const NextHop& nextHop);

  // Get the ref count for a given set of NextHopIDs
  uint32_t getNextHopIDSetRefCount(const NextHopIDSet& nextHopIDSet);

  // Get the NextHopSetID for a given NextHopIDSet
  // Returns std::nullopt if the set doesn't exist
  std::optional<NextHopSetID> getNextHopSetID(
      const NextHopIDSet& nextHopIDSet) const;

  // Get the NextHopID for a given NextHop (lookup only, no allocation)
  // Returns std::nullopt if the NextHop doesn't exist
  std::optional<NextHopID> getNextHopID(const NextHop& nextHop) const;

  FRIEND_TEST(NextHopIDManagerTest, getOrAllocateNextHopID);
  FRIEND_TEST(NextHopIDManagerTest, getOrAllocateNextHopSetID);
  FRIEND_TEST(NextHopIDManagerTest, getOrAllocateNextHopSetIDOrderIndependence);
  FRIEND_TEST(NextHopIDManagerTest, decrOrDeallocateNextHop);
  FRIEND_TEST(NextHopIDManagerTest, decrOrDeallocateNextHopIDSet);
  FRIEND_TEST(NextHopIDManagerTest, getOrAllocRouteNextHopSetIDWithEmptySet);
  FRIEND_TEST(
      NextHopIDManagerTest,
      getOrAllocRouteNextHopSetIDSubSetSuperSetNextHops);
  FRIEND_TEST(NextHopIDManagerTest, delOrDecrRouteNextHopSetID);
  FRIEND_TEST(NextHopIDManagerTest, updateRouteNextHopSetID);
};

} // namespace facebook::fboss
