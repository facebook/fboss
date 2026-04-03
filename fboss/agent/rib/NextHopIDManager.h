// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gtest/gtest_prod.h>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/MySidMap.h"
#include "fboss/agent/state/NextHopIdMaps.h"
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
  // Structure to hold ID and reference count for NextHops
  struct NextHopIDInfo {
    NextHopID id;
    uint32_t count = 0;

    NextHopIDInfo(NextHopID id_, uint32_t count_) : id(id_), count(count_) {}
  };

  // Structure to hold ID and reference count for NextHopSets
  // Must be public as it's exposed through NextHopIdSetIter
  struct NextHopSetIDInfo {
    NextHopSetID id;
    uint32_t count;

    NextHopSetIDInfo(NextHopSetID id_, uint32_t count_)
        : id(id_), count(count_) {}
  };

  // Type alias for the NextHop to NextHopIDInfo map iterator
  using NextHopInfoIter =
      std::unordered_map<NextHop, NextHopIDInfo>::const_iterator;

  // Type alias for the NextHopIDSet to NextHopSetIDInfo map iterator
  using NextHopIdSetIter =
      std::unordered_map<NextHopIDSet, NextHopSetIDInfo>::const_iterator;

  // Result struct for NextHop allocation operations
  // Contains information about what was allocated so callers can update FibInfo
  // incrementally
  struct NextHopAllocationResult {
    // Const iterator to the NextHopIDSet -> NextHopSetIDInfo entry
    NextHopIdSetIter nextHopIdSetIter;
    // List of newly allocated NextHopIDs (caller can retrieve NextHop from
    // idToNextHop map using these IDs)
    std::vector<NextHopID> addedNextHopIds;
  };

  // Result struct for NextHop deallocation operations
  // Contains information about what was deallocated so callers can update
  // FibInfo incrementally
  struct NextHopDeallocationResult {
    // List of NextHopIDs that were deallocated (refcount reached 0)
    std::vector<NextHopID> removedNextHopIds;
    // Only populated if the set was deallocated (refcount reached 0)
    std::optional<NextHopSetID> removedSetId;
  };

  // Result struct for NextHop update operations
  // Contains both allocation and deallocation information
  struct NextHopUpdateResult {
    NextHopAllocationResult allocation;
    NextHopDeallocationResult deallocation;
  };

  // Result struct for named next-hop group allocation
  struct NamedNextHopGroupAllocationResult {
    NextHopAllocationResult allocation;
    // Name of the next-hop group
    std::string name;
    // Whether this is a new group or an update to existing
    bool isNew{false};
  };

  // Result struct for named next-hop group update
  struct NamedNextHopGroupUpdateResult {
    NextHopAllocationResult allocation;
    NextHopDeallocationResult deallocation;
    std::string name;
  };

  NextHopIDManager() = default;

  // Get or allocate a NextHopID for the given NextHop
  // Returns const iterator to the NextHop -> NextHopIDInfo entry
  NextHopInfoIter getOrAllocateNextHopID(const NextHop& nextHop);

  // Get or allocate a NextHopSetID for the given set of NextHopIDs
  // Returns const iterator to the NextHopIDSet -> NextHopSetIDInfo entry
  NextHopIdSetIter getOrAllocateNextHopSetID(const NextHopIDSet& nextHopIDSet);

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
  // Returns NextHopAllocationResult containing the const iterator to the
  // NextHopIDSet entry and any newly allocated NextHopIDs
  NextHopAllocationResult getOrAllocRouteNextHopSetID(
      const RouteNextHopSet& nextHopSet);

  // Decrements or deallocates NextHopSetID for a RouteNextHopSet
  // Returns NextHopDeallocationResult containing any deallocated NextHops
  // and optionally the removed setId (if refcount reached 0)
  NextHopDeallocationResult decrOrDeallocRouteNextHopSetID(
      NextHopSetID nextHopSetID);

  // Lookup the NextHopSetID for a given RouteNextHopSet
  // Returns std::nullopt if the set doesn't exist
  std::optional<NextHopSetID> lookupRouteNextHopSetID(
      const RouteNextHopSet& nextHopSet) const;

  // Update from old RouteNextHopSet to new RouteNextHopSet
  // Decrements old, allocates/increments new
  // Returns NextHopUpdateResult containing both allocation and deallocation
  // info
  NextHopUpdateResult updateRouteNextHopSetID(
      NextHopSetID oldNextHopSetID,
      const RouteNextHopSet& newNextHopSet);

  // Named next-hop group management
  // Allocate or update a named next-hop group with the given nexthops
  // The name to nexthops mapping is maintained internally
  // Returns allocation result and whether it was a new group
  NamedNextHopGroupAllocationResult allocateNamedNextHopGroup(
      const std::string& name,
      const RouteNextHopSet& nextHopSet);

  // Update an existing named next-hop group with new nexthops
  // Deallocates old nexthops and allocates new ones
  // Throws if the group doesn't exist
  NamedNextHopGroupUpdateResult updateNamedNextHopGroup(
      const std::string& name,
      const RouteNextHopSet& newNextHopSet);

  // Deallocate a named next-hop group
  // Removes the name to nexthops mapping and deallocates the nexthops
  // Returns deallocation result
  // Throws if the group doesn't exist
  NextHopDeallocationResult deallocateNamedNextHopGroup(
      const std::string& name);

  // Get the NextHopSetID for a named next-hop group
  // Returns std::nullopt if the group doesn't exist
  std::optional<NextHopSetID> getNextHopSetIDForName(
      const std::string& name) const;

  // Get the nexthops for a named next-hop group
  // Returns std::nullopt if the group doesn't exist
  std::optional<RouteNextHopSet> getNextHopsForName(
      const std::string& name) const;

  // Get the RouteNextHopSet for a given NextHopSetID
  // Throws FbossError if the NextHopSetID is not found
  RouteNextHopSet getNextHops(NextHopSetID nextHopSetID) const;

  // Get the RouteNextHopSet for a given NextHopSetID
  // Returns std::nullopt if the NextHopSetID is not found
  std::optional<RouteNextHopSet> getNextHopsIf(NextHopSetID nextHopSetID) const;

  // Get all named next-hop groups
  const std::unordered_map<std::string, RouteNextHopSet>&
  getNameToNextHopSetMap() const {
    return nameToNextHopSet_;
  }

  // Check if a named next-hop group exists
  bool hasNamedNextHopGroup(const std::string& name) const {
    return nameToNextHopSet_.find(name) != nameToNextHopSet_.end();
  }

  /**
   * Reconstruct the NextHopIDManager for two main scenarios:
   *
   * 1. Warm Boot:
   *    When the RIB is reconstructed back from the FIB, we need to reconstruct
   *    the NextHopIDManager.
   *
   * 2. Rollback:
   *    When the RIB's NextHopIDManager has to be reconstructed from the
   *    previous FIB state. This is when route updates fail to apply.
   *
   * For each FibInfo in MultiSwitchFibInfoMap:
   *   1. Get idToNextHopMap and idToNextHopIdSetMap from this FibInfo
   *   2. For each route in FibV4/FibV6:
   *      a. Collect all NextHopSetIDs from route (resolvedNextHopSetID, and
   *         future: normalizedResolvedNextHopID)
   *      b. For each SetID, lookup NextHopIDSet from this FibInfo's
   *         idToNextHopIdSetMap
   *      c. For each NextHopID in the set, lookup NextHop from this FibInfo's
   *         idToNextHopMap
   *      d. Update internal maps and refcounts
   *
   * We have to reconstruct the IDmanager from all the FibsInfo present on all
   * switches. NextHopID manager is common across all switches, but the NextHop
   * ID maps in the switch state are specific to each switch.
   */
  void reconstructFromSwitchStateMaps(
      const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap,
      const std::shared_ptr<MultiSwitchMySidMap>& mySidMap);

 private:
  static constexpr int64_t kNextHopIDStart = 1;
  static constexpr int64_t kNextHopSetIDStart = 1LL << 62;

  // Counter for generating NextHop IDs, starting from 1
  // allocate 0 - (2^62 -1) IDs for NextHops.
  NextHopID nextAvailableNextHopID_{kNextHopIDStart};

  // Counter for generating NextHopSet IDs, starting from 2^62
  // Allocate 2^62 - INT64_MAX IDs for NextHopIDSets.
  NextHopSetID nextAvailableNextHopSetID_{kNextHopSetIDStart};

  // Mapping from NextHop to its ID and reference count
  std::unordered_map<NextHop, NextHopIDInfo> nextHopToIDInfo_;

  // Mapping from set of NextHopIDs to its NextHopSetID and reference count
  std::unordered_map<NextHopIDSet, NextHopSetIDInfo> nextHopIdSetToIDInfo_;

  // Reverse lookup maps for clients to convert IDs back to NextHops
  std::unordered_map<NextHopID, NextHop> idToNextHop_;

  // Map from NextHopSetID to set of NextHopIDs
  std::unordered_map<NextHopSetID, NextHopIDSet> idToNextHopIdSet_;

  // Named next-hop group mappings
  // Map from name to RouteNextHopSet (the actual nexthops)
  std::unordered_map<std::string, RouteNextHopSet> nameToNextHopSet_;
  // Map from name to NextHopSetID for quick lookup
  std::unordered_map<std::string, NextHopSetID> nameToNextHopSetID_;

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

  // Clear all NextHop and NextHopIDSet mappings
  void clearNhopIdManagerState();

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
  FRIEND_TEST(NextHopIDManagerTest, reconstructFromSwitchStateMaps);
  FRIEND_TEST(NextHopIDManagerTest, reconstructFromSwitchStateMapsMultiSwitch);
  FRIEND_TEST(
      NextHopIDManagerTest,
      reconstructFromSwitchStateMaps_MySidResolvedNextHopsId);
  FRIEND_TEST(
      NextHopIDManagerTest,
      reconstructFromSwitchStateMaps_MySidBothNextHopIds);
  FRIEND_TEST(NextHopIDManagerTest, allocateNamedNextHopGroup);
  FRIEND_TEST(NextHopIDManagerTest, updateNamedNextHopGroup);
  FRIEND_TEST(NextHopIDManagerTest, deallocateNamedNextHopGroup);
  FRIEND_TEST(NextHopIDManagerTest, namedNextHopGroupWarmBoot);
  FRIEND_TEST(NextHopIDManagerTest, namedNextHopGroupSharesSetIdWithRoutes);
  FRIEND_TEST(NextHopIDManagerTest, routeReusesNamedNextHopGroupSetId);
  FRIEND_TEST(
      RibMySidUpdaterTest,
      nhopRefCountBumped_afterResolvingNhopWithIntfId);
  FRIEND_TEST(
      RibMySidUpdaterTest,
      twoEntriesSameNhops_resolvedSetIdSharedAndRefCountIsTwo);
};

} // namespace facebook::fboss
