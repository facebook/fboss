// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
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

std::optional<NextHopSetID> NextHopIDManager::lookupRouteNextHopSetID(
    const RouteNextHopSet& nextHopSet) const {
  // Build the NextHopIDSet by looking up each NextHop's ID
  NextHopIDSet nextHopIDSet;
  for (const auto& nextHop : nextHopSet) {
    auto nhId = getNextHopID(nextHop);
    DCHECK(nhId) << "NextHop ID not found in NextHopIDManager";
    if (!nhId.has_value()) {
      // NextHop not found, return nullopt
      return std::nullopt;
    }
    nextHopIDSet.insert(*nhId);
  }

  // Lookup the NextHopSetID
  return getNextHopSetID(nextHopIDSet);
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
      XLOG(DBG3) << "NextHopID " << nextHopID << " deallocated";
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

void NextHopIDManager::clearNhopIdManagerState() {
  nextHopToIDInfo_.clear();
  idToNextHop_.clear();
  nextHopIdSetToIDInfo_.clear();
  idToNextHopIdSet_.clear();
  nameToNextHopSet_.clear();
  nameToNextHopSetID_.clear();
}

// Allocate or update a named next-hop group.
// This API is called when a thrift API request comes in to add/update a named
// next-hop group, independent of whether any route is using it.
// Note: getOrAllocRouteNextHopSetID will reuse existing NextHopSetID if the
// same nexthops have already been allocated (e.g., by routes), ensuring named
// groups and routes share the same NextHopSetID when they have identical
// nexthops.
NextHopIDManager::NamedNextHopGroupAllocationResult
NextHopIDManager::allocateNamedNextHopGroup(
    const std::string& name,
    const RouteNextHopSet& nextHopSet) {
  NamedNextHopGroupAllocationResult result;
  result.name = name;

  // Check if this name already exists
  auto existingIt = nameToNextHopSet_.find(name);
  if (existingIt != nameToNextHopSet_.end()) {
    // Group already exists - delegate to updateNamedNextHopGroup
    auto updateResult = updateNamedNextHopGroup(name, nextHopSet);
    result.allocation = std::move(updateResult.allocation);
    result.isNew = false;
  } else {
    // New named next-hop group
    // Note: getOrAllocRouteNextHopSetID will reuse existing NextHopSetID
    // if the same nexthops have already been allocated (e.g., by routes).
    // This ensures named groups and routes share the same NextHopSetID
    // when they reference the same set of nexthops.
    result.allocation = getOrAllocRouteNextHopSetID(nextHopSet);
    result.isNew = true;

    // Store the mappings
    nameToNextHopSet_[name] = nextHopSet;
    nameToNextHopSetID_[name] = result.allocation.nextHopIdSetIter->second.id;
  }

  return result;
}

NextHopIDManager::NamedNextHopGroupUpdateResult
NextHopIDManager::updateNamedNextHopGroup(
    const std::string& name,
    const RouteNextHopSet& newNextHopSet) {
  NamedNextHopGroupUpdateResult result;
  result.name = name;

  // Check if this name exists
  auto existingIt = nameToNextHopSet_.find(name);
  if (existingIt == nameToNextHopSet_.end()) {
    throw FbossError("Cannot update non-existent named next-hop group: ", name);
  }

  auto oldSetIdIt = nameToNextHopSetID_.find(name);
  CHECK(oldSetIdIt != nameToNextHopSetID_.end())
      << "Named next-hop group " << name
      << " exists in nameToNextHopSet_ but not in nameToNextHopSetID_";

  // Deallocate old nexthops
  result.deallocation = decrOrDeallocRouteNextHopSetID(oldSetIdIt->second);

  // Allocate new nexthops
  result.allocation = getOrAllocRouteNextHopSetID(newNextHopSet);

  // Update the mappings
  nameToNextHopSet_[name] = newNextHopSet;
  nameToNextHopSetID_[name] = result.allocation.nextHopIdSetIter->second.id;

  return result;
}

NextHopIDManager::NextHopDeallocationResult
NextHopIDManager::deallocateNamedNextHopGroup(const std::string& name) {
  // Check if this name exists
  auto existingIt = nameToNextHopSet_.find(name);
  if (existingIt == nameToNextHopSet_.end()) {
    throw FbossError(
        "Cannot deallocate non-existent named next-hop group: ", name);
  }

  auto setIdIt = nameToNextHopSetID_.find(name);
  CHECK(setIdIt != nameToNextHopSetID_.end())
      << "Named next-hop group " << name
      << " exists in nameToNextHopSet_ but not in nameToNextHopSetID_";

  // Deallocate the nexthops
  auto result = decrOrDeallocRouteNextHopSetID(setIdIt->second);

  // Remove the name mappings
  nameToNextHopSet_.erase(existingIt);
  nameToNextHopSetID_.erase(setIdIt);

  return result;
}

std::optional<NextHopSetID> NextHopIDManager::getNextHopSetIDForName(
    const std::string& name) const {
  auto it = nameToNextHopSetID_.find(name);
  if (it != nameToNextHopSetID_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<RouteNextHopSet> NextHopIDManager::getNextHopsForName(
    const std::string& name) const {
  auto it = nameToNextHopSet_.find(name);
  if (it != nameToNextHopSet_.end()) {
    return it->second;
  }
  return std::nullopt;
}

RouteNextHopSet NextHopIDManager::getNextHops(NextHopSetID nextHopSetID) const {
  auto nextHopsOpt = getNextHopsIf(nextHopSetID);
  if (!nextHopsOpt) {
    throw FbossError("NextHopSetID ", nextHopSetID, " not found");
  }
  return *nextHopsOpt;
}

std::optional<RouteNextHopSet> NextHopIDManager::getNextHopsIf(
    NextHopSetID nextHopSetID) const {
  auto nextHopIdSetIt = idToNextHopIdSet_.find(nextHopSetID);
  if (nextHopIdSetIt == idToNextHopIdSet_.end()) {
    return std::nullopt;
  }

  std::vector<NextHop> nextHopVec;
  nextHopVec.reserve(nextHopIdSetIt->second.size());
  for (const auto& nextHopID : nextHopIdSetIt->second) {
    auto nextHopIt = idToNextHop_.find(nextHopID);
    CHECK(nextHopIt != idToNextHop_.end())
        << "NextHopId " << nextHopID
        << " not found in idToNextHop_ for NextHopSetID " << nextHopSetID;
    nextHopVec.push_back(nextHopIt->second);
  }
  return RouteNextHopSet(nextHopVec.begin(), nextHopVec.end());
}

void NextHopIDManager::reconstructFromFib(
    const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap) {
  clearNhopIdManagerState();

  if (!fibsInfoMap || fibsInfoMap->empty()) {
    return;
  }

  NextHopID maxNextHopId = NextHopID(kNextHopIDStart - 1);
  NextHopSetID maxNextHopSetId = NextHopSetID(kNextHopSetIDStart - 1);

  // Track named next-hop groups that we've already processed
  // We only need to process them once since they're the same across switches
  std::unordered_set<std::string> processedNamedGroups;

  // Iterate through each FIBInfo on all switches
  for (const auto& [switchId, fibInfo] : std::as_const(*fibsInfoMap)) {
    auto id2NhopMapInFib = fibInfo->getIdToNextHopMap();
    auto id2NhopIdSetMapInFib = fibInfo->getIdToNextHopIdSetMap();
    auto fibsMap = fibInfo->getfibsMap();

    if (!fibsMap || !id2NhopMapInFib || !id2NhopIdSetMapInFib) {
      continue;
    }

    // function to process a single SetID
    // resolvedNextHopSetID, normalizedResolvedNextHopID, and any future ID
    // types
    auto processNhopSetId = [&](NextHopSetID setId) {
      // Lookup NextHopIDSet from FibInfo's idToNextHopIdSetMap
      auto nextHopIdSetNode = id2NhopIdSetMapInFib->getNextHopIdSet(
          static_cast<state::NextHopSetIdType>(setId));
      CHECK(nextHopIdSetNode);
      // Build NextHopIDSet and process each NextHop
      // Iterate directly over the node; elements are wrapped, so unwrap with
      // .toThrift()
      NextHopIDSet nextHopIDSet;
      for (const auto& elem : std::as_const(*nextHopIdSetNode)) {
        NextHopID nextHopID((*elem).toThrift());
        nextHopIDSet.insert(nextHopID);

        // Lookup NextHop from FibInfo's idToNextHopMap
        auto nhNode = id2NhopMapInFib->getNextHopIf(
            static_cast<state::NextHopIdType>(nextHopID));
        if (!nhNode) {
          throw FbossError(
              "Inconsistent state: NextHopID ",
              nextHopID,
              " not found in FibInfo's idToNextHopMap for SetID ",
              setId);
        }

        auto nextHop = util::fromThrift(
            nhNode->toThrift(), true /* allowV6NonLinkLocal */);

        // Update NextHop maps and refcounts
        auto nhInfoIt = nextHopToIDInfo_.find(nextHop);
        if (nhInfoIt == nextHopToIDInfo_.end()) {
          nextHopToIDInfo_.emplace(nextHop, NextHopIDInfo(nextHopID, 1));
          idToNextHop_[nextHopID] = nextHop;
        } else {
          nhInfoIt->second.count++;
        }
        maxNextHopId = std::max(maxNextHopId, nextHopID);
      }

      // Update NextHopIDSet maps and refcounts
      auto setInfoIt = nextHopIdSetToIDInfo_.find(nextHopIDSet);
      if (setInfoIt == nextHopIdSetToIDInfo_.end()) {
        nextHopIdSetToIDInfo_.emplace(nextHopIDSet, NextHopSetIDInfo(setId, 1));
        idToNextHopIdSet_[setId] = nextHopIDSet;
      } else {
        setInfoIt->second.count++;
      }
      maxNextHopSetId = std::max(maxNextHopSetId, setId);
    };

    // process routes from a FIB
    auto processRoutes = [&](const auto& fib) {
      for (const auto& [prefix, route] : std::as_const(*fib)) {
        const auto& fwdInfo = route->getForwardInfo();

        // Process resolvedNextHopSetID
        if (auto setIdOpt = fwdInfo.getResolvedNextHopSetID()) {
          processNhopSetId(NextHopSetID(*setIdOpt));
        }

        // Process normalizedResolvedNextHopSetID
        if (auto normalizedSetIdOpt =
                fwdInfo.getNormalizedResolvedNextHopSetID()) {
          processNhopSetId(NextHopSetID(*normalizedSetIdOpt));
        }
      }
    };

    // Iterate over all VRFs in this FibInfo
    for (const auto& [routerId, fibContainer] : std::as_const(*fibsMap)) {
      processRoutes(fibContainer->getFibV4());
      processRoutes(fibContainer->getFibV6());
    }

    // Reconstruct named next-hop groups from FibInfo
    // Only process each name once (they should be the same across switches)
    // This must come after processNhopSetId is defined so we can use it
    // to properly increment reference counts for named groups
    auto nameToSetIdMap =
        fibInfo->safe_cref<switch_state_tags::nameToNextHopSetId>();
    if (nameToSetIdMap) {
      for (const auto& [name, setIdNode] : std::as_const(*nameToSetIdMap)) {
        if (processedNamedGroups.count(name) > 0) {
          continue;
        }
        processedNamedGroups.insert(name);

        NextHopSetID setId = NextHopSetID(setIdNode->toThrift());

        // Increment reference counts via processNhopSetId so that
        // deallocateNamedNextHopGroup() can properly decrement them.
        // This also populates idToNextHop_ and idToNextHopIdSet_ maps.
        processNhopSetId(setId);

        // Now we can get the NextHopIDSet directly from the ID manager's
        // idToNextHopIdSet_ map which was populated by processNhopSetId
        auto nextHopIdSetIt = idToNextHopIdSet_.find(setId);
        CHECK(nextHopIdSetIt != idToNextHopIdSet_.end())
            << "NextHopSetId " << setId
            << " not found in idToNextHopIdSet_ after processNhopSetId";

        // Build RouteNextHopSet from the NextHopIDSet using idToNextHop_ map
        // Collect nexthops in a vector first to avoid O(n^2) single-element
        // insertions into flat_set
        std::vector<NextHop> nextHopVec;
        nextHopVec.reserve(nextHopIdSetIt->second.size());
        for (const auto& nextHopID : nextHopIdSetIt->second) {
          auto nextHopIt = idToNextHop_.find(nextHopID);
          CHECK(nextHopIt != idToNextHop_.end())
              << "NextHopId " << nextHopID
              << " not found in idToNextHop_ after processNhopSetId";
          nextHopVec.push_back(nextHopIt->second);
        }
        RouteNextHopSet nextHopSet(nextHopVec.begin(), nextHopVec.end());

        // Store the name to nexthop mapping
        nameToNextHopSet_[name] = nextHopSet;
        nameToNextHopSetID_[name] = setId;
      }
    }
  }

  // Set next available IDs
  nextAvailableNextHopID_ = NextHopID(maxNextHopId + 1);
  nextAvailableNextHopSetID_ = NextHopSetID(maxNextHopSetId + 1);
}

} // namespace facebook::fboss
