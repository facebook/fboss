/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ResourceAccountant.h"
#include "fboss/agent/AgentFeatures.h"

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
constexpr auto kHundredPercentage = 100;
} // namespace

namespace facebook::fboss {

ResourceAccountant::ResourceAccountant(
    const HwAsicTable* asicTable,
    const SwitchIdScopeResolver* scopeResolver)
    : asicTable_(asicTable), scopeResolver_(scopeResolver) {
  CHECK_EQ(
      asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER),
      asicTable->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER));
  nativeWeightedEcmp_ = asicTable->isFeatureSupportedOnAllAsic(
      HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER);
  checkRouteUpdate_ = shouldCheckRouteUpdate();
}

bool ResourceAccountant::isEcmp(const RouteNextHopEntry& fwd) const {
  for (const auto& nhop : fwd.normalizedNextHops()) {
    if (nhop.weight() && nhop.weight() > 1) {
      return false;
    }
  }
  return true;
}

size_t ResourceAccountant::computeWeightedEcmpMemberCount(
    const RouteNextHopEntry& fwd,
    const cfg::AsicType& asicType) const {
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      // For TH4, UCMP members take 4x of ECMP members in the same table.
      return 4 * fwd.normalizedNextHops().size();
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      // For TH5, UCMP members take 4x of ECMP members in the same table.
      return 4 * fwd.normalizedNextHops().size();
    case cfg::AsicType::ASIC_TYPE_YUBA:
      // Yuba asic natively supports UCMP members with no extra cost.
      return fwd.normalizedNextHops().size();
    default:
      XLOG(
          WARNING,
          "Unsupported ASIC type for Ucmp member resource computation. Assuming UCMP member usage is computed by ECMP replication");
      auto totalWeight = 0;
      for (const auto& nhop : fwd.normalizedNextHops()) {
        totalWeight += nhop.weight() ? nhop.weight() : 1;
      }
      return totalWeight;
  }
}

size_t ResourceAccountant::getMemberCountForEcmpGroup(
    const RouteNextHopEntry& fwd) const {
  if (isEcmp(fwd)) {
    return fwd.normalizedNextHops().size();
  }
  if (nativeWeightedEcmp_) {
    // Different asic supports native WeightedEcmp in different ways.
    // Therefore we will have asic-specific logic to compute the member count.
    const auto asics = asicTable_->getHwAsics();
    const auto asicType = asics.begin()->second->getAsicType();
    // Ensure that all ASICs have the same type.
    CHECK(
        std::all_of(
            asics.begin(), asics.end(), [&asicType](const auto& idAndAsic) {
              return idAndAsic.second->getAsicType() == asicType;
            }));
    return computeWeightedEcmpMemberCount(fwd, asicType);
  }
  // No native weighted ECMP support. Members are replicated to support
  // weighted ECMP.
  auto totalWeight = 0;
  for (const auto& nhop : fwd.normalizedNextHops()) {
    totalWeight += nhop.weight() ? nhop.weight() : 1;
  }
  return totalWeight;
}

bool ResourceAccountant::checkEcmpResource(bool intermediateState) const {
  // There are two checks needed for ECMP resource:
  // 1) Post each route add/update, check if intermediate state exceeds HW
  // limit. 2) Post entire state update, check if total usage is lower than
  // ecmp_resource_percentage.
  uint32_t resourcePercentage =
      intermediateState ? kHundredPercentage : FLAGS_ecmp_resource_percentage;

  for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
    const auto ecmpGroupLimit = hwAsic->getMaxEcmpGroups();
    const auto ecmpMemberLimit = hwAsic->getMaxEcmpMembers();
    std::optional<int> ecmpGroupEnforcedLimit, ecmpMemberEnforcedLimit;
    if (ecmpGroupLimit.has_value()) {
      ecmpGroupEnforcedLimit =
          (ecmpGroupLimit.value() * resourcePercentage / kHundredPercentage);
    }
    if (ecmpMemberLimit.has_value()) {
      ecmpMemberEnforcedLimit =
          (ecmpMemberLimit.value() * resourcePercentage / kHundredPercentage);
    }

    if (ecmpGroupEnforcedLimit.has_value() &&
        ecmpGroupRefMap_.size() > *ecmpGroupEnforcedLimit) {
      XLOG(DBG2) << " Ecmp group limit exceeded. Ecmp demand from this update: "
                 << ecmpGroupRefMap_.size()
                 << " ASIC limit: " << *ecmpGroupEnforcedLimit;
      return false;
    }
    if (ecmpMemberEnforcedLimit.has_value() &&
        ecmpMemberUsage_ > *ecmpMemberEnforcedLimit) {
      XLOG(DBG2)
          << " Ecmp member limit exceeded. Ecmp demand from this update: "
          << ecmpMemberUsage_ << " ASIC Limit: " << *ecmpMemberEnforcedLimit;
      return false;
    }
  }
  return true;
}

bool ResourceAccountant::checkArsResource(bool intermediateState) const {
  if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable) {
    uint32_t resourcePercentage =
        intermediateState ? kHundredPercentage : FLAGS_ars_resource_percentage;

    for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
      const auto arsGroupLimit = hwAsic->getMaxArsGroups();
      if (arsGroupLimit.has_value() &&
          arsEcmpGroupRefMap_.size() >
              (arsGroupLimit.value() * resourcePercentage) /
                  kHundredPercentage) {
        return false;
      }
    }
  }
  return true;
}

template <typename AddrT>
bool ResourceAccountant::checkAndUpdateGenericEcmpResource(
    const std::shared_ptr<Route<AddrT>>& route,
    bool add) {
  const auto& fwd = route->getForwardInfo();

  const auto nhSet = fwd.normalizedNextHops();
  // Forwarding to nextHops and more than one nextHop - use ECMP
  if (fwd.getAction() == RouteForwardAction::NEXTHOPS && nhSet.size() > 1) {
    if (auto it = ecmpGroupRefMap_.find(nhSet); it != ecmpGroupRefMap_.end()) {
      it->second = it->second + (add ? 1 : -1);
      CHECK(it->second >= 0);
      if (!add && it->second == 0) {
        ecmpGroupRefMap_.erase(it);
        ecmpMemberUsage_ -= getMemberCountForEcmpGroup(fwd);
      }
      return true;
    }
    // ECMP group does not exists in hw - Check if any usage exceeds ASIC
    // limit
    CHECK(add);
    ecmpGroupRefMap_[nhSet] = 1;
    ecmpMemberUsage_ += getMemberCountForEcmpGroup(fwd);
    return checkEcmpResource(true /* intermediateState */);
  }
  return true;
}

template <typename AddrT>
bool ResourceAccountant::checkAndUpdateArsEcmpResource(
    const std::shared_ptr<Route<AddrT>>& route,
    bool add) {
  if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable) {
    const auto& fwd = route->getForwardInfo();
    const auto nhSet = fwd.normalizedNextHops();
    // Forwarding to nextHops and more than one nextHop - use ECMP
    if (fwd.getAction() == RouteForwardAction::NEXTHOPS && nhSet.size() > 1) {
      // If ERM were disabled, then arsEcmpGroupRefMap_ and ecmpGroupRefMap_
      // will be identical since primary and backup groups are
      // indistinguishable.
      //
      // No need to check backup groups since they don't use dynamic groups
      if (FLAGS_enable_ecmp_resource_manager &&
          fwd.getOverrideEcmpSwitchingMode().has_value()) {
        return true;
      }
      if (auto it = arsEcmpGroupRefMap_.find(nhSet);
          it != arsEcmpGroupRefMap_.end()) {
        it->second = it->second + (add ? 1 : -1);
        CHECK(it->second >= 0);
        if (!add && it->second == 0) {
          arsEcmpGroupRefMap_.erase(it);
        }
        return true;
      }
      // ECMP group does not exists in hw - Check if any usage exceeds ASIC
      // limit
      CHECK(add);
      arsEcmpGroupRefMap_[nhSet] = 1;
      return checkArsResource(true /* intermediateState */);
    }
  }
  return true;
}

template <typename AddrT>
bool ResourceAccountant::checkAndUpdateEcmpResource(
    const std::shared_ptr<Route<AddrT>>& route,
    bool add) {
  bool valid = true;
  valid &= checkAndUpdateGenericEcmpResource(route, add);
  valid &= checkAndUpdateArsEcmpResource(route, add);
  return valid;
}

bool ResourceAccountant::shouldCheckRouteUpdate() const {
  if (!FLAGS_enable_route_resource_protection) {
    return false;
  }
  for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
    if (hwAsic->getMaxEcmpGroups().has_value() ||
        hwAsic->getMaxEcmpMembers().has_value() ||
        hwAsic->getMaxRoutes().has_value()) {
      return true;
    }
  }
  return false;
}

bool ResourceAccountant::checkAndUpdateRouteResource(bool add) {
  // Staring with the simpliest computation - treat all routes the same.
  // We will graually evolve this to be more accurate (e.g. v4 /32, v4 </32,
  // v6/64 etc.).
  if (add) {
    routeUsage_++;
    for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
      const auto routeLimit = hwAsic->getMaxRoutes();
      if (routeLimit.has_value() && routeUsage_ > routeLimit.value()) {
        return false;
      }
    }
    return true;
  }
  routeUsage_--;
  return true;
}

bool ResourceAccountant::routeAndEcmpStateChangedImpl(const StateDelta& delta) {
  if (!checkRouteUpdate_ || !FLAGS_enable_route_resource_protection) {
    return true;
  }
  bool validRouteUpdate = true;

  processFibsDeltaInHwSwitchOrder(
      delta,
      [&](RouterID /*rid*/, const auto& oldRoute, const auto& newRoute) {
        if (!oldRoute->isResolved() && !newRoute->isResolved()) {
          return;
        }
        if (oldRoute->isResolved() && !newRoute->isResolved()) {
          validRouteUpdate &=
              checkAndUpdateEcmpResource(oldRoute, false /* add */);
          validRouteUpdate &= checkAndUpdateRouteResource(false /* add */);
          return;
        }
        if (!oldRoute->isResolved() && newRoute->isResolved()) {
          validRouteUpdate &=
              checkAndUpdateEcmpResource(newRoute, true /* add */);
          validRouteUpdate &= checkAndUpdateRouteResource(true /* add */);
          return;
        }
        // Both old and new are resolved
        CHECK(oldRoute->isResolved() && newRoute->isResolved());
        validRouteUpdate &= checkAndUpdateEcmpResource(newRoute, true);
        validRouteUpdate &= checkAndUpdateEcmpResource(oldRoute, false);
      },
      [&](RouterID /*rid*/, const auto& newRoute) {
        if (newRoute->isResolved()) {
          validRouteUpdate &=
              checkAndUpdateEcmpResource(newRoute, true /* add */);
          validRouteUpdate &= checkAndUpdateRouteResource(true /* add */);
        }
      },
      [&](RouterID /*rid*/, const auto& delRoute) {
        if (delRoute->isResolved()) {
          validRouteUpdate &=
              checkAndUpdateEcmpResource(delRoute, false /* add */);
          validRouteUpdate &= checkAndUpdateRouteResource(false /* add */);
        }
      }

  );

  // Ensure new state usage does not exceed ecmp_resource_percentage
  validRouteUpdate &= checkEcmpResource(false /* intermediateState */);
  validRouteUpdate &= checkArsResource(false /* intermediateState */);
  return validRouteUpdate;
}

bool ResourceAccountant::isValidRouteUpdate(const StateDelta& delta) {
  bool validRouteUpdate = routeAndEcmpStateChangedImpl(delta);

  if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable &&
      !validRouteUpdate) {
    XLOG(WARNING)
        << "Invalid route update - exceeding DLB resource limits. New state consumes "
        << arsEcmpGroupRefMap_.size() << " DLB ECMP groups";
    for (const auto& [switchId, hwAsic] : asicTable_->getHwAsics()) {
      const auto dlbGroupLimit = hwAsic->getMaxArsGroups();
      XLOG(WARNING) << "DLB ECMP resource limits for Switch " << switchId
                    << ": max DLB groups="
                    << (dlbGroupLimit.has_value()
                            ? folly::to<std::string>(dlbGroupLimit.value())
                            : "None");
    }
    return validRouteUpdate;
  }

  if (!validRouteUpdate) {
    XLOG(WARNING)
        << "Invalid route update - exceeding route or ECMP resource limits. New state consumes "
        << routeUsage_ << " routes, " << ecmpMemberUsage_
        << " ECMP members and " << ecmpGroupRefMap_.size() << " ECMP groups.";
    for (const auto& [switchId, hwAsic] : asicTable_->getHwAsics()) {
      const auto ecmpGroupLimit = hwAsic->getMaxEcmpGroups();
      const auto ecmpMemberLimit = hwAsic->getMaxEcmpMembers();
      const auto routeLimit = hwAsic->getMaxRoutes();
      XLOG(WARNING) << "ECMP resource limits for Switch " << switchId
                    << ": max routes="
                    << (routeLimit.has_value()
                            ? folly::to<std::string>(routeLimit.value())
                            : "None")
                    << ": max ECMP groups="
                    << (ecmpGroupLimit.has_value()
                            ? folly::to<std::string>(ecmpGroupLimit.value())
                            : "None")
                    << ", max ECMP members="
                    << (ecmpMemberLimit.has_value()
                            ? folly::to<std::string>(ecmpMemberLimit.value())
                            : "None");
    }
  }
  return validRouteUpdate;
}

template bool
ResourceAccountant::checkAndUpdateEcmpResource<folly::IPAddressV6>(
    const std::shared_ptr<Route<folly::IPAddressV6>>& route,
    bool add);

template bool
ResourceAccountant::checkAndUpdateEcmpResource<folly::IPAddressV4>(
    const std::shared_ptr<Route<folly::IPAddressV4>>& route,
    bool add);

// calculate new update for l2 entries from the delta
// check l2Entries_ in the switchState
// return true if the l2Entries_ are within the limit
bool ResourceAccountant::l2StateChangedImpl(const StateDelta& delta) {
  auto processDelta = [&](const auto& deltaMac) {
    DeltaFunctions::forEachChanged(
        deltaMac,
        [&](const auto& /*oldMac*/, const auto& /*newMac*/) {
          return LoopAction::CONTINUE;
        },
        [&](const auto& /*newMac*/) {
          l2Entries_++;
          return LoopAction::CONTINUE;
        },
        [&](const auto& /*deletedMac*/) {
          l2Entries_--;
          return LoopAction::CONTINUE;
        });
  };

  for (auto& deltaVlan : delta.getVlansDelta()) {
    processDelta(deltaVlan.getMacDelta());
  }
  if (l2Entries_ > FLAGS_max_l2_entries) {
    XLOG(ERR) << "Total l2 entries in new switchState: " << l2Entries_
              << " exceeds the limit: " << FLAGS_max_l2_entries;
    return false;
  }
  return true;
}

// Neighbor table resoure accounting

// get switchId from neighbor entry
SwitchID ResourceAccountant::getSwitchIdFromNeighborEntry(
    std::shared_ptr<SwitchState> newState,
    const auto& nbrEntry) {
  const auto& interfaceMap = newState->getInterfaces();
  InterfaceID interfaceId = nbrEntry->getIntfID();
  std::shared_ptr<Interface> intf = interfaceMap->getNodeIf(interfaceId);
  if (!intf) {
    throw FbossError("No interface found for interfaceId: ", interfaceId);
  }
  SwitchID switchId = scopeResolver_->scope(intf, newState).switchId();

  return switchId;
}

// check if the resource count per ASIC is set
// return false if the resource count per ASIC is not set
template <typename TableT>
bool ResourceAccountant::shouldCheckNeighborUpdate(SwitchID switchId) {
  return getMaxNeighborTableSize<TableT>(switchId, kHundredPercentage)
      .has_value();
}

// get total neighbor table size from ASIC
template <typename TableT>
std::optional<uint32_t> ResourceAccountant::getMaxAsicNeighborTableSize(
    SwitchID switchId,
    uint8_t resourcePercentage) {
  uint32_t size = 0;
  auto hwAsic = asicTable_->getHwAsicIf(SwitchID(switchId));

  if constexpr (std::is_same_v<TableT, NdpTable>) {
    size = hwAsic->getMaxNdpTableSize().has_value()
        ? hwAsic->getMaxNdpTableSize().value()
        : 0;
  } else if constexpr (std::is_same_v<TableT, ArpTable>) {
    size = hwAsic->getMaxArpTableSize().has_value()
        ? hwAsic->getMaxArpTableSize().value()
        : 0;
  } else { // Invalid resource type
    throw FbossError("Invalid resource type");
  }
  if (size == 0) {
    return std::nullopt;
  }
  return (size * resourcePercentage) / kHundredPercentage;
}

// get total unified neighbor table size from ASIC
// unified table is shared by both ARP and NDP tables
std::optional<uint32_t> ResourceAccountant::getMaxAsicUnifiedNeighborTableSize(
    const SwitchID& switchId,
    uint8_t resourcePercentage) {
  uint32_t size = 0;
  auto hwAsic = asicTable_->getHwAsicIf(SwitchID(switchId));

  size = hwAsic->getMaxUnifiedNeighborTableSize().has_value()
      ? hwAsic->getMaxUnifiedNeighborTableSize().value()
      : 0;
  if (size == 0) {
    return std::nullopt;
  }
  return (size * resourcePercentage) / kHundredPercentage;
}

// get max neighbor table size suported by resourceAccountant
template <typename TableT>
uint32_t ResourceAccountant::getMaxConfiguredNeighborTableSize() {
  if constexpr (std::is_same_v<TableT, NdpTable>) {
    return FLAGS_max_ndp_entries;
  } else if constexpr (std::is_same_v<TableT, ArpTable>) {
    return FLAGS_max_arp_entries;
  }
  throw FbossError("Invalid resource type");
}

// check if the neighbor resource is available for the update as per limits
template <typename TableT>
bool ResourceAccountant::checkNeighborResource(
    SwitchID switchId,
    uint32_t count,
    bool intermediateState) {
  // There are two checks needed for neighbor resource:
  // 1) Post each neighbor add update, check if intermediate
  //  state exceeds HW limit.
  // 2) Post entire state update, check if total usage is lower than
  //  neighbor_resource_percentage.

  // No need to check for neighbor resource if the max resource count per ASIC
  // is not set
  if (!shouldCheckNeighborUpdate<TableT>(switchId)) {
    return true;
  }

  uint8_t resourcePercentage = intermediateState
      ? kHundredPercentage
      : FLAGS_neighbhor_resource_percentage;

  uint32_t maxCapacity =
      getMaxNeighborTableSize<TableT>(switchId, resourcePercentage).value();

  return count <= maxCapacity;
}

template <typename TableT>
std::unordered_map<SwitchID, uint32_t>&
ResourceAccountant::getNeighborEntriesMap() {
  if constexpr (std::is_same_v<TableT, NdpTable>) {
    return ndpEntriesMap_;
  } else if constexpr (std::is_same_v<TableT, ArpTable>) {
    return arpEntriesMap_;
  } else { // Invalid resource type
    throw FbossError("Invalid resource type");
  }
}
// calculate new update for neighbor entries from the delta
template <typename TableT>
void ResourceAccountant::neighborStateChangedImpl(const StateDelta& delta) {
  auto processDelta = [&](const auto& deltaNbr, auto& entriesMap) {
    DeltaFunctions::forEachChanged(
        deltaNbr,
        [&](const auto& /*old*/, const auto& /*new*/) {
          return LoopAction::CONTINUE;
        },
        [&](const auto& newNbr) {
          auto switchId =
              getSwitchIdFromNeighborEntry(delta.newState(), newNbr);
          entriesMap[switchId]++;
          return LoopAction::CONTINUE;
        },
        [&](const auto& deleted) {
          auto switchId =
              getSwitchIdFromNeighborEntry(delta.newState(), deleted);
          entriesMap[switchId]--;
          return LoopAction::CONTINUE;
        });
  };

  if (FLAGS_intf_nbr_tables) {
    for (auto& intfDelta : delta.getIntfsDelta()) {
      processDelta(
          intfDelta.getNeighborDelta<TableT>(),
          getNeighborEntriesMap<TableT>());
    }
  } else {
    for (auto& vlanDelta : delta.getVlansDelta()) {
      processDelta(
          vlanDelta.getNeighborDelta<TableT>(),
          getNeighborEntriesMap<TableT>());
    }
  }
}

bool ResourceAccountant::checkNeighborResource() {
  std::set<SwitchID> allSwitchIds;
  for (const auto& [switchId, _] : ndpEntriesMap_) {
    allSwitchIds.insert(switchId);
  }
  for (const auto& [switchId, _] : arpEntriesMap_) {
    allSwitchIds.insert(switchId);
  }

  // Check each switch ID
  for (const auto& switchId : allSwitchIds) {
    auto ndpIt = ndpEntriesMap_.find(switchId);
    auto arpIt = arpEntriesMap_.find(switchId);
    uint32_t ndpCount = (ndpIt != ndpEntriesMap_.end()) ? ndpIt->second : 0;
    uint32_t arpCount = (arpIt != arpEntriesMap_.end()) ? arpIt->second : 0;

    // Check NDP limits
    if (ndpCount > 0) {
      auto maxNdpSize = getMaxConfiguredNeighborTableSize<NdpTable>();
      if (FLAGS_enforce_resource_hw_limits) {
        auto asicNdpSize =
            getMaxAsicNeighborTableSize<NdpTable>(switchId, kHundredPercentage);
        if (asicNdpSize.has_value()) {
          maxNdpSize = std::min(asicNdpSize.value(), maxNdpSize);
        }
      }
      if (ndpCount > maxNdpSize) {
        XLOG(ERR) << "Total NDP entries in new switchState: " << ndpCount
                  << " exceeds the limit: " << maxNdpSize
                  << " for switchId: " << switchId;
        return false;
      }
    }

    // Check ARP limits
    if (arpCount > 0) {
      auto maxArpSize = getMaxConfiguredNeighborTableSize<ArpTable>();
      if (FLAGS_enforce_resource_hw_limits) {
        auto asicArpSize =
            getMaxAsicNeighborTableSize<ArpTable>(switchId, kHundredPercentage);
        if (asicArpSize.has_value()) {
          maxArpSize = std::min(asicArpSize.value(), maxArpSize);
        }
      }
      if (arpCount > maxArpSize) {
        XLOG(ERR) << "Total ARP entries in new switchState: " << arpCount
                  << " exceeds the limit: " << maxArpSize
                  << " for switchId: " << switchId;
        return false;
      }
    }

    // Check unified table limits if enabled and available
    if (FLAGS_enforce_resource_hw_limits &&
        getMaxAsicUnifiedNeighborTableSize(switchId, kHundredPercentage)
            .has_value()) {
      uint32_t unifiedCount = ndpCount + arpCount;
      uint32_t maxUnifiedSize =
          getMaxAsicUnifiedNeighborTableSize(switchId, kHundredPercentage)
              .value();
      if (unifiedCount > maxUnifiedSize) {
        XLOG(ERR) << "Total unified neighbor entries in new switchState: "
                  << unifiedCount << " exceeds the limit: " << maxUnifiedSize
                  << " for switchId: " << switchId;
        return false;
      }
    }
  }

  return true;
}

// stateChanged is called when the ResourceAccountant needs to be updated
void ResourceAccountant::stateChanged(const StateDelta& delta) {
  routeAndEcmpStateChangedImpl(delta);

  if (FLAGS_enable_hw_update_protection) {
    l2StateChangedImpl(delta);
    neighborStateChangedImpl<NdpTable>(delta);
    neighborStateChangedImpl<ArpTable>(delta);
  }
}

// check if the resource is available for the update as per fboss limits
bool ResourceAccountant::isValidUpdate(const StateDelta& delta) {
  bool isValidUpdate = isValidRouteUpdate(delta);

  if (FLAGS_enable_hw_update_protection) {
    isValidUpdate &= l2StateChangedImpl(delta);
    neighborStateChangedImpl<NdpTable>(delta);
    neighborStateChangedImpl<ArpTable>(delta);
    isValidUpdate &= checkNeighborResource();
  }

  return isValidUpdate;
}

} // namespace facebook::fboss
