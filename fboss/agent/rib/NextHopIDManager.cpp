// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/Route.h"
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

NextHopIDManager::NextHopToIDIter NextHopIDManager::getOrAllocateNextHopID(
    const NextHop& nextHop) {
  auto it = nextHopToID_.find(nextHop);
  if (it != nextHopToID_.end()) {
    idToNextHop_.at(it->second).refCount++;
    XLOG(DBG3) << "[NextHop ID Manager] refCount++ nhId=" << it->second
               << " nh=" << nextHop.addr().str();
    return it;
  }

  NextHopID newID = nextAvailableNextHopID_;
  nextAvailableNextHopID_ = NextHopID(nextAvailableNextHopID_ + 1);

  CHECK(static_cast<int64_t>(newID) > 0 && newID < kNextHopSetIDStart)
      << "Next Hop ID is in the range of [0, 2^62 - 1], the id space has been exhausted! It does not support wrap around!";

  auto [nhItr, nhInserted] = nextHopToID_.emplace(nextHop, newID);
  CHECK(nhInserted);
  auto [idItr, idInserted] =
      idToNextHop_.emplace(newID, NextHopEntry(nextHop, 1));
  CHECK(idInserted);
  XLOG(DBG3) << "[NextHop ID Manager] allocated nhId=" << newID
             << " nh=" << nextHop.addr().str();
  return nhItr;
}

NextHopIDManager::NextHopIdSetIter NextHopIDManager::getOrAllocateNextHopSetID(
    const NextHopIDSet& nextHopIDSet) {
  auto it = nextHopIdSetToIDInfo_.find(nextHopIDSet);
  if (it != nextHopIdSetToIDInfo_.end()) {
    it->second.count++;
    XLOG(DBG3) << "[NextHop ID Manager] refCount++ setId=" << it->second.id
               << " setSize=" << nextHopIDSet.size();
    return it;
  }

  NextHopSetID newID = nextAvailableNextHopSetID_;
  nextAvailableNextHopSetID_ = NextHopSetID(nextAvailableNextHopSetID_ + 1);

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
  XLOG(DBG3) << "[NextHop ID Manager] allocated setId=" << newID
             << " setSize=" << nextHopIDSet.size();
  return idSetInfoItr;
}

uint32_t NextHopIDManager::getNextHopRefCount(const NextHop& nextHop) {
  auto it = nextHopToID_.find(nextHop);
  if (it != nextHopToID_.end()) {
    auto idIt = idToNextHop_.find(it->second);
    if (idIt != idToNextHop_.end()) {
      return idIt->second.refCount;
    }
    throw FbossError(
        "NextHopID ",
        it->second,
        " in nextHopToIDInfo_ but missing from idToNextHop_");
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
  auto it = nextHopToID_.find(nextHop);
  if (it == nextHopToID_.end()) {
    throw FbossError(
        "Cannot decrement reference count or deallocate for non-existent NextHop");
  }
  return decrOrDeallocateNextHopByID(it->second);
}

bool NextHopIDManager::decrOrDeallocateNextHopByID(const NextHopID& nextHopID) {
  auto idIt = idToNextHop_.find(nextHopID);
  if (idIt == idToNextHop_.end()) {
    throw FbossError(
        "Cannot decrement reference count or deallocate for non-existent NextHopID ",
        nextHopID);
  }
  CHECK_GT(idIt->second.refCount, 0);
  idIt->second.refCount--;
  if (idIt->second.refCount == 0) {
    XLOG(DBG3) << "[NextHop ID Manager] deallocated nhId=" << nextHopID
               << " nh=" << idIt->second.nextHop.addr().str();
    auto erasedNh = nextHopToID_.erase(idIt->second.nextHop);
    CHECK_EQ(erasedNh, 1);
    idToNextHop_.erase(idIt);
    return true;
  }
  XLOG(DBG3) << "[NextHop ID Manager] refCount-- nhId=" << nextHopID;
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
    XLOG(DBG3) << "[NextHop ID Manager] deallocated setId=" << it->second.id;
    auto erasedIdMap = idToNextHopIdSet_.erase(it->second.id);
    CHECK_EQ(erasedIdMap, 1);
    auto erasedIdInfo = nextHopIdSetToIDInfo_.erase(it->first);
    CHECK_EQ(erasedIdInfo, 1);
    return true;
  }
  XLOG(DBG3) << "[NextHop ID Manager] refCount-- setId=" << it->second.id;
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
  auto it = nextHopToID_.find(nextHop);
  if (it != nextHopToID_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<NextHopSetID> NextHopIDManager::lookupRouteNextHopSetID(
    const RouteNextHopSet& nextHopSet) const {
  // Build the NextHopIDSet by looking up each NextHop's ID
  NextHopIDSet nextHopIDSet;
  for (const auto& nextHop : nextHopSet) {
    auto nhId = getNextHopID(nextHop);
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
    auto nhId = nhIter->second;
    nextHopIDSet.insert(nhId);
    if (idToNextHop_.at(nhId).refCount == 1) {
      result.addedNextHopIds.push_back(nhId);
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
  // Uses ID-based lookup to avoid hashing the NextHop object
  for (const auto& nextHopID : nextHopIDSet) {
    auto derefNextHop = decrOrDeallocateNextHopByID(nextHopID);
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
  nextHopToID_.clear();
  idToNextHop_.clear();
  nextHopIdSetToIDInfo_.clear();
  idToNextHopIdSet_.clear();
  nameToNextHopSet_.clear();
  nameToNextHopSetID_.clear();
  nameToRoutes_.clear();
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

  if (hasRoutesForNamedNhg(name)) {
    throw FbossError(
        "Cannot delete named next-hop group '",
        name,
        "' because it is referenced by routes");
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
    nextHopVec.push_back(nextHopIt->second.nextHop);
  }
  return RouteNextHopSet(nextHopVec.begin(), nextHopVec.end());
}

bool NextHopIDManager::nextHopSetContainsAddr(
    NextHopSetID nextHopSetID,
    const folly::IPAddress& ip) const {
  auto nextHopIdSetIt = idToNextHopIdSet_.find(nextHopSetID);
  if (nextHopIdSetIt == idToNextHopIdSet_.end()) {
    return false;
  }
  for (const auto& nextHopID : nextHopIdSetIt->second) {
    auto nextHopIt = idToNextHop_.find(nextHopID);
    CHECK(nextHopIt != idToNextHop_.end())
        << "NextHopId " << nextHopID
        << " not found in idToNextHop_ for NextHopSetID " << nextHopSetID;
    if (nextHopIt->second.nextHop.addr() == ip) {
      return true;
    }
  }
  return false;
}

namespace {

// Verifies that the idToNextHopMap and idToNextHopIdSetMap are identical
// across all switchId -> fibInfo entries. In production, these maps are synced
// from the global RIB and must be the same on every switch.
bool assertNextHopIdMapsSame(
    const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap) {
  if (!fibsInfoMap || fibsInfoMap->size() <= 1) {
    return true;
  }

  std::shared_ptr<IdToNextHopMap> referenceNhopMap;
  std::shared_ptr<IdToNextHopIdSetMap> referenceNhopIdSetMap;

  for (const auto& [switchId, fibInfo] : std::as_const(*fibsInfoMap)) {
    auto nhopMap = fibInfo->getIdToNextHopMap();
    auto nhopIdSetMap = fibInfo->getIdToNextHopIdSetMap();
    if (!nhopMap || !nhopIdSetMap) {
      continue;
    }
    if (!referenceNhopMap) {
      referenceNhopMap = nhopMap;
      referenceNhopIdSetMap = nhopIdSetMap;
      continue;
    }
    if (nhopMap->toThrift() != referenceNhopMap->toThrift() ||
        nhopIdSetMap->toThrift() != referenceNhopIdSetMap->toThrift()) {
      XLOG(ERR) << "NextHopIdMaps differ across switches";
      return false;
    }
  }
  return true;
}

} // namespace

void NextHopIDManager::processNextHopSetIdForReconstruction(
    NextHopSetID setId,
    const std::shared_ptr<IdToNextHopMap>& id2NhopMap,
    const std::shared_ptr<IdToNextHopIdSetMap>& id2NhopIdSetMap,
    ReconstructionContext& ctx) {
  // Lookup NextHopIDSet from FibInfo's idToNextHopIdSetMap
  auto nextHopIdSetNode = id2NhopIdSetMap->getNextHopIdSet(
      static_cast<state::NextHopSetIdType>(setId));
  CHECK(nextHopIdSetNode);
  // Build NextHopIDSet and process each NextHop. Iterate directly over the
  // node; elements are wrapped, so unwrap with .toThrift().
  NextHopIDSet nextHopIDSet;
  for (const auto& elem : std::as_const(*nextHopIdSetNode)) {
    NextHopID nextHopID((*elem).toThrift());
    nextHopIDSet.insert(nextHopID);

    // Lookup NextHop from FibInfo's idToNextHopMap
    auto nhNode =
        id2NhopMap->getNextHopIf(static_cast<state::NextHopIdType>(nextHopID));
    if (!nhNode) {
      throw FbossError(
          "Inconsistent state: NextHopID ",
          nextHopID,
          " not found in FibInfo's idToNextHopMap for SetID ",
          setId);
    }

    auto nextHop =
        util::fromThrift(nhNode->toThrift(), true /* allowV6NonLinkLocal */);

    // Update NextHop maps and refcounts
    auto nhIt = nextHopToID_.find(nextHop);
    if (nhIt == nextHopToID_.end()) {
      nextHopToID_.emplace(nextHop, nextHopID);
      idToNextHop_.emplace(nextHopID, NextHopEntry(nextHop, 1));
    } else {
      idToNextHop_.at(nhIt->second).refCount++;
    }
    ctx.maxNextHopId = std::max(ctx.maxNextHopId, nextHopID);
  }

  // Update NextHopIDSet maps and refcounts
  auto setInfoIt = nextHopIdSetToIDInfo_.find(nextHopIDSet);
  if (setInfoIt == nextHopIdSetToIDInfo_.end()) {
    nextHopIdSetToIDInfo_.emplace(nextHopIDSet, NextHopSetIDInfo(setId, 1));
    idToNextHopIdSet_[setId] = nextHopIDSet;
  } else {
    setInfoIt->second.count++;
  }
  ctx.maxNextHopSetId = std::max(ctx.maxNextHopSetId, setId);
}

void NextHopIDManager::reconstructFibPass(
    const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap,
    ReconstructionContext& ctx) {
  if (!fibsInfoMap || fibsInfoMap->empty()) {
    return;
  }

  // Track named next-hop groups that we've already processed. We only need to
  // process them once since they're the same across switches.
  std::unordered_set<std::string> processedNamedGroups;

  for (const auto& [switchId, fibInfo] : std::as_const(*fibsInfoMap)) {
    auto id2NhopMapInFib = fibInfo->getIdToNextHopMap();
    auto id2NhopIdSetMapInFib = fibInfo->getIdToNextHopIdSetMap();
    auto fibsMap = fibInfo->getfibsMap();

    if (!fibsMap || !id2NhopMapInFib || !id2NhopIdSetMapInFib) {
      continue;
    }

    // FibInfo's id maps contain all allocated nexthop IDs including those for
    // MySid entries, and are consistent across switches. Cache the first
    // switch's copy for the MySid / MPLS / unresolved-RIB passes.
    if (!ctx.fibId2NhopMap) {
      ctx.fibId2NhopMap = id2NhopMapInFib;
      ctx.fibId2NhopIdSetMap = id2NhopIdSetMapInFib;
    }

    // process routes from a FIB
    auto processRoutes = [&](const auto& fib, RouterID rid) {
      for (const auto& [prefix, route] : std::as_const(*fib)) {
        const auto& fwdInfo = route->getForwardInfo();

        // Process resolvedNextHopSetID
        if (auto setIdOpt = fwdInfo.getResolvedNextHopSetID()) {
          processNextHopSetIdForReconstruction(
              NextHopSetID(*setIdOpt),
              id2NhopMapInFib,
              id2NhopIdSetMapInFib,
              ctx);
        }

        // Process normalizedResolvedNextHopSetID
        if (auto normalizedSetIdOpt =
                fwdInfo.getNormalizedResolvedNextHopSetID()) {
          processNextHopSetIdForReconstruction(
              NextHopSetID(*normalizedSetIdOpt),
              id2NhopMapInFib,
              id2NhopIdSetMapInFib,
              ctx);
        }

        // Reconstruct named NHG reverse mapping from per-client entries,
        // and rebuild refcounts for any per-client clientNextHopSetID.
        const auto& nhopsMulti = route->getEntryForClients();
        for (const auto& entry : nhopsMulti) {
          auto nhgName = entry.second->getNamedNextHopGroup();
          if (nhgName.has_value()) {
            auto cidr = route->prefix().toCidrNetwork();
            nameToRoutes_[*nhgName].emplace(rid, cidr);
          }
          if (auto clientSetIdOpt = entry.second->getClientNextHopSetID()) {
            processNextHopSetIdForReconstruction(
                NextHopSetID(*clientSetIdOpt),
                id2NhopMapInFib,
                id2NhopIdSetMapInFib,
                ctx);
          }
        }
      }
    };

    // Iterate over all VRFs in this FibInfo
    for (const auto& [routerId, fibContainer] : std::as_const(*fibsMap)) {
      RouterID rid(routerId);
      processRoutes(fibContainer->getFibV4(), rid);
      processRoutes(fibContainer->getFibV6(), rid);
    }

    // Reconstruct named next-hop groups from FibInfo. Only process each name
    // once (they should be the same across switches).
    auto nameToSetIdMap =
        fibInfo->safe_cref<switch_state_tags::nameToNextHopSetId>();
    if (nameToSetIdMap) {
      for (const auto& [name, setIdNode] : std::as_const(*nameToSetIdMap)) {
        if (processedNamedGroups.count(name) > 0) {
          continue;
        }
        processedNamedGroups.insert(name);

        NextHopSetID setId = NextHopSetID(setIdNode->toThrift());

        // Increment reference counts via processNextHopSetIdForReconstruction
        // so that deallocateNamedNextHopGroup() can properly decrement them.
        processNextHopSetIdForReconstruction(
            setId, id2NhopMapInFib, id2NhopIdSetMapInFib, ctx);

        auto nextHopIdSetIt = idToNextHopIdSet_.find(setId);
        CHECK(nextHopIdSetIt != idToNextHopIdSet_.end())
            << "NextHopSetId " << setId
            << " not found in idToNextHopIdSet_ after processNhopSetId";

        std::vector<NextHop> nextHopVec;
        nextHopVec.reserve(nextHopIdSetIt->second.size());
        for (const auto& nextHopID : nextHopIdSetIt->second) {
          auto nextHopIt = idToNextHop_.find(nextHopID);
          CHECK(nextHopIt != idToNextHop_.end())
              << "NextHopId " << nextHopID
              << " not found in idToNextHop_ after processNhopSetId";
          nextHopVec.push_back(nextHopIt->second.nextHop);
        }
        RouteNextHopSet nextHopSet(nextHopVec.begin(), nextHopVec.end());

        nameToNextHopSet_[name] = nextHopSet;
        nameToNextHopSetID_[name] = setId;
      }
    }
  }
}

void NextHopIDManager::reconstructMySidPass(
    const std::shared_ptr<MultiSwitchMySidMap>& mySidMap,
    ReconstructionContext& ctx) {
  // MySid pass: register/bump refcounts for MySid entries' nexthop set IDs.
  // FibInfo contains all MySid nexthop IDs in its id maps, but those sets may
  // not be referenced by any routes. processNextHopSetIdForReconstruction
  // handles both new registration and refcount-bumping for already-registered
  // sets.
  if (!mySidMap || !ctx.fibId2NhopMap || !ctx.fibId2NhopIdSetMap) {
    return;
  }
  for (const auto& miter : std::as_const(*mySidMap)) {
    for (const auto& [key, mySid] : std::as_const(*miter.second)) {
      if (auto setIdOpt = mySid->getResolvedNextHopsId()) {
        processNextHopSetIdForReconstruction(
            *setIdOpt, ctx.fibId2NhopMap, ctx.fibId2NhopIdSetMap, ctx);
      }
      if (auto setIdOpt = mySid->getUnresolveNextHopsId()) {
        processNextHopSetIdForReconstruction(
            *setIdOpt, ctx.fibId2NhopMap, ctx.fibId2NhopIdSetMap, ctx);
      }
    }
  }
}

void NextHopIDManager::reconstructMplsFibPass(
    const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFib,
    ReconstructionContext& ctx) {
  // MPLS FIB pass: resolved setIDs + per-client clientNextHopSetIDs.
  if (!labelFib || !ctx.fibId2NhopMap || !ctx.fibId2NhopIdSetMap) {
    return;
  }
  for (const auto& [switchId, labelFibInner] : std::as_const(*labelFib)) {
    for (const auto& [label, route] : std::as_const(*labelFibInner)) {
      const auto& fwdInfo = route->getForwardInfo();
      if (auto setIdOpt = fwdInfo.getResolvedNextHopSetID()) {
        processNextHopSetIdForReconstruction(
            NextHopSetID(*setIdOpt),
            ctx.fibId2NhopMap,
            ctx.fibId2NhopIdSetMap,
            ctx);
      }
      if (auto normalizedSetIdOpt =
              fwdInfo.getNormalizedResolvedNextHopSetID()) {
        processNextHopSetIdForReconstruction(
            NextHopSetID(*normalizedSetIdOpt),
            ctx.fibId2NhopMap,
            ctx.fibId2NhopIdSetMap,
            ctx);
      }
      for (const auto& [_clientId, entry] :
           std::as_const(route->getEntryForClients())) {
        if (auto clientSetIdOpt = entry->getClientNextHopSetID()) {
          processNextHopSetIdForReconstruction(
              NextHopSetID(*clientSetIdOpt),
              ctx.fibId2NhopMap,
              ctx.fibId2NhopIdSetMap,
              ctx);
        }
      }
    }
  }
}

void NextHopIDManager::reconstructUnresolvedRibPass(
    const RibRouteTables* ribTables,
    ReconstructionContext& ctx) {
  // Unresolved-RIB pass: per-client setIDs invisible to the FIB walk.
  if (!ribTables || !ctx.fibId2NhopMap || !ctx.fibId2NhopIdSetMap) {
    return;
  }
  // We use unsafeGetUnlocked() because the caller already holds the
  // synchronizedRouteTables_ wlock — re-locking would self-deadlock.
  //
  // The obvious-looking alternative — drop the unsafe access and pass
  // lockedRouteTables->routerIDToRouteTable straight in as a parameter
  // — falls apart for two reasons. First, RouterIDToRouteTable /
  // VrfRouteTable / RouteTables are private nested types in
  // RibRouteTables, so naming them in this header's signature would
  // force them all public and require moving them to a shared header.
  // Second, the moment NextHopIDManager.h mentions a RibRouteTables
  // type it has to include RoutingInformationBase.h, which already
  // includes this header back (RibRouteTables owns a NextHopIDManager
  // member) — circular include.
  //
  // Rather than reorganize that type hierarchy for one refcount walk,
  // we stuck with unsafeGetUnlocked + a friend declaration on
  // RibRouteTables. Minimal blast radius, no public surface change.
  const auto& routeTables =
      ribTables->synchronizedRouteTables_.unsafeGetUnlocked();
  auto bumpClientSetIdsOnRoute = [&](const auto& route) {
    if (route->isResolved()) {
      return;
    }
    for (const auto& [_clientId, entry] :
         std::as_const(route->getEntryForClients())) {
      if (auto setIdOpt = entry->getClientNextHopSetID()) {
        processNextHopSetIdForReconstruction(
            NextHopSetID(*setIdOpt),
            ctx.fibId2NhopMap,
            ctx.fibId2NhopIdSetMap,
            ctx);
      }
    }
  };
  // v4/v6 NetworkToRouteMap is a RadixTree — iterator yields nodes
  // dereferenced via .value().
  auto processUnresolvedIpRoutes = [&](const auto& routes) {
    for (auto ritr = routes.begin(); ritr != routes.end(); ++ritr) {
      bumpClientSetIdsOnRoute(ritr->value());
    }
  };
  // LabelToRouteMap is an unordered_map — range-for yields
  // pair<LabelID, shared_ptr<Route<LabelID>>>.
  auto processUnresolvedMplsRoutes = [&](const auto& routes) {
    for (const auto& [_label, route] : routes) {
      bumpClientSetIdsOnRoute(route);
    }
  };
  for (const auto& [_vrf, vrfTable] : routeTables.routerIDToRouteTable) {
    processUnresolvedIpRoutes(vrfTable.v4NetworkToRoute);
    processUnresolvedIpRoutes(vrfTable.v6NetworkToRoute);
    // Mirror every other MPLS-touching path in the RIB (FibUpdater,
    // ConfigApplier, rollback, fromThrift): only walk labelToRoute when
    // FLAGS_mpls_rib is on, otherwise it may carry stale entries no
    // decrement path will ever match.
    if (FLAGS_mpls_rib) {
      processUnresolvedMplsRoutes(vrfTable.labelToRoute);
    }
  }
}

void NextHopIDManager::reconstructFromSwitchStateMaps(
    const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap,
    const std::shared_ptr<MultiSwitchMySidMap>& mySidMap,
    const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFib,
    const RibRouteTables* ribTables) {
  DCHECK(assertNextHopIdMapsSame(fibsInfoMap));
  clearNhopIdManagerState();

  ReconstructionContext ctx;

  // The FIB pass also caches the (cross-switch-identical) id maps into ctx for
  // the remaining passes, whose sets may not be referenced by any FIB route.
  reconstructFibPass(fibsInfoMap, ctx);
  reconstructMySidPass(mySidMap, ctx);
  reconstructMplsFibPass(labelFib, ctx);
  reconstructUnresolvedRibPass(ribTables, ctx);

  // Set next available IDs
  nextAvailableNextHopID_ = NextHopID(ctx.maxNextHopId + 1);
  nextAvailableNextHopSetID_ = NextHopSetID(ctx.maxNextHopSetId + 1);

  XLOG(INFO) << "[NextHop ID Manager] reconstruction done:"
             << " nhopCount=" << idToNextHop_.size()
             << " setCount=" << idToNextHopIdSet_.size()
             << " nextNhopId=" << nextAvailableNextHopID_
             << " nextSetId=" << nextAvailableNextHopSetID_;
  if (XLOG_IS_ON(DBG3)) {
    for (const auto& [id, entry] : idToNextHop_) {
      XLOG(DBG3) << "[NextHop ID Manager] reconstructed nhId=" << id
                 << " nh=" << entry.nextHop.addr().str();
    }
    for (const auto& [setId, nhopIdSet] : idToNextHopIdSet_) {
      std::string ids;
      for (const auto& nhId : nhopIdSet) {
        if (!ids.empty()) {
          ids += ",";
        }
        ids += folly::to<std::string>(static_cast<int64_t>(nhId));
      }
      XLOG(DBG3) << "[NextHop ID Manager] reconstructed setId=" << setId
                 << " nhIds={" << ids << "}";
    }
  }
}

void NextHopIDManager::addRouteForNamedNhg(
    const std::string& name,
    const RouterID& rid,
    const folly::CIDRNetwork& prefix) {
  nameToRoutes_[name].emplace(rid, prefix);
}

void NextHopIDManager::removeRouteForNamedNhg(
    const std::string& name,
    const RouterID& rid,
    const folly::CIDRNetwork& prefix) {
  auto it = nameToRoutes_.find(name);
  if (it != nameToRoutes_.end()) {
    it->second.erase({rid, prefix});
    if (it->second.empty()) {
      nameToRoutes_.erase(it);
    }
  }
}

const NextHopIDManager::RouteSet& NextHopIDManager::getRoutesForNamedNhg(
    const std::string& name) const {
  static const RouteSet kEmpty;
  auto it = nameToRoutes_.find(name);
  if (it != nameToRoutes_.end()) {
    return it->second;
  }
  return kEmpty;
}

bool NextHopIDManager::hasRoutesForNamedNhg(const std::string& name) const {
  auto it = nameToRoutes_.find(name);
  return it != nameToRoutes_.end() && !it->second.empty();
}

} // namespace facebook::fboss
