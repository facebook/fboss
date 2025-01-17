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

int ResourceAccountant::computeWeightedEcmpMemberCount(
    const RouteNextHopEntry& fwd,
    const cfg::AsicType& asicType) const {
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      // For TH4, UCMP members take 4x of ECMP members in the same table.
      return 4 * fwd.getNextHopSet().size();
    case cfg::AsicType::ASIC_TYPE_YUBA:
      // Yuba asic natively supports UCMP members with no extra cost.
      return fwd.getNextHopSet().size();
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

int ResourceAccountant::getMemberCountForEcmpGroup(
    const RouteNextHopEntry& fwd) const {
  if (isEcmp(fwd)) {
    return fwd.getNextHopSet().size();
  }
  if (nativeWeightedEcmp_) {
    // Different asic supports native WeightedEcmp in different ways.
    // Therefore we will have asic-specific logic to compute the member count.
    const auto asics = asicTable_->getHwAsics();
    const auto asicType = asics.begin()->second->getAsicType();
    // Ensure that all ASICs have the same type.
    CHECK(std::all_of(
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

  // No need to check for DLB resources unless checkDlbResource_=True
  if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable &&
      checkDlbResource_) {
    if (!checkDlbResource(resourcePercentage)) {
      return false;
    }
  }

  for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
    const auto ecmpGroupLimit = hwAsic->getMaxEcmpGroups();
    const auto ecmpMemberLimit = hwAsic->getMaxEcmpMembers();
    if ((ecmpGroupLimit.has_value() &&
         ecmpGroupRefMap_.size() >
             (ecmpGroupLimit.value() * resourcePercentage) /
                 kHundredPercentage) ||
        (ecmpMemberLimit.has_value() &&
         ecmpMemberUsage_ > (ecmpMemberLimit.value() * resourcePercentage) /
                 kHundredPercentage)) {
      return false;
    }
  }
  return true;
}

bool ResourceAccountant::checkDlbResource(uint32_t resourcePercentage) const {
  for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
    const auto dlbGroupLimit = hwAsic->getMaxDlbEcmpGroups();
    if (dlbGroupLimit.has_value() &&
        ecmpGroupRefMap_.size() >
            (dlbGroupLimit.value() * resourcePercentage) / kHundredPercentage) {
      return false;
    }
  }
  return true;
}

template <typename AddrT>
bool ResourceAccountant::checkAndUpdateEcmpResource(
    const std::shared_ptr<Route<AddrT>>& route,
    bool add) {
  const auto& fwd = route->getForwardInfo();

  // Forwarding to nextHops and more than one nextHop - use ECMP
  if (fwd.getAction() == RouteForwardAction::NEXTHOPS &&
      fwd.getNextHopSet().size() > 1) {
    const auto& nhSet = fwd.getNextHopSet();
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

  auto processRoutesDelta = [&](const auto& routesDelta) {
    DeltaFunctions::forEachRemoved(routesDelta, [&](const auto& delRoute) {
      validRouteUpdate &= checkAndUpdateEcmpResource(delRoute, false /* add */);
      validRouteUpdate &= checkAndUpdateRouteResource(false /* add */);
    });

    DeltaFunctions::forEachChanged(
        routesDelta, [&](const auto& oldRoute, const auto& newRoute) {
          validRouteUpdate &= checkAndUpdateEcmpResource(newRoute, true);
          validRouteUpdate &= checkAndUpdateEcmpResource(oldRoute, false);
        });

    DeltaFunctions::forEachAdded(routesDelta, [&](const auto& newRoute) {
      validRouteUpdate &= checkAndUpdateEcmpResource(newRoute, true /* add */);
      validRouteUpdate &= checkAndUpdateRouteResource(true /* add */);
    });
  };

  for (const auto& routeDelta : delta.getFibsDelta()) {
    processRoutesDelta(routeDelta.getFibDelta<folly::IPAddressV4>());
    processRoutesDelta(routeDelta.getFibDelta<folly::IPAddressV6>());
  }

  // Ensure new state usage does not exceed ecmp_resource_percentage
  validRouteUpdate &= checkEcmpResource(false /* intermediateState */);
  return validRouteUpdate;
}

bool ResourceAccountant::isValidRouteUpdate(const StateDelta& delta) {
  bool validRouteUpdate = routeAndEcmpStateChangedImpl(delta);

  if (FLAGS_dlbResourceCheckEnable && FLAGS_flowletSwitchingEnable &&
      checkDlbResource_ && !validRouteUpdate) {
    XLOG(WARNING)
        << "Invalid route update - exceeding DLB resource limits. New state consumes "
        << ecmpGroupRefMap_.size() << " DLB ECMP groups and "
        << ecmpMemberUsage_ << " members.";
    for (const auto& [switchId, hwAsic] : asicTable_->getHwAsics()) {
      const auto dlbGroupLimit = hwAsic->getMaxDlbEcmpGroups();
      const auto ecmpMemberLimit = hwAsic->getMaxEcmpMembers();
      XLOG(WARNING) << "DLB ECMP resource limits for Switch " << switchId
                    << ": max DLB groups="
                    << (dlbGroupLimit.has_value()
                            ? folly::to<std::string>(dlbGroupLimit.value())
                            : "None")
                    << ", max ECMP members="
                    << (ecmpMemberLimit.has_value()
                            ? folly::to<std::string>(ecmpMemberLimit.value())
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

void ResourceAccountant::enableDlbResourceCheck(bool enable) {
  checkDlbResource_ = enable;
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
std::optional<uint32_t> ResourceAccountant::getMaxNeighborTableSize(
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
bool ResourceAccountant::neighborStateChangedImpl(const StateDelta& delta) {
  bool isValidUpdate = true;

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

          isValidUpdate &= checkNeighborResource<TableT>(
              switchId, entriesMap[switchId], true); // intermediate

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

  // Ensure new state usage does not exceed neighbor_resource_percentage
  for (const auto& [switchId, count] : getNeighborEntriesMap<TableT>()) {
    isValidUpdate &= checkNeighborResource<TableT>(switchId, count, false);
    if (!isValidUpdate) { // log error
      std::string neighbor;
      if constexpr (std::is_same_v<TableT, NdpTable>) {
        neighbor = "Ndp";
      } else if constexpr (std::is_same_v<TableT, ArpTable>) {
        neighbor = "Arp";
      } else { // Invalid resource type
        throw FbossError("Invalid resource type");
      }
      XLOG(ERR) << neighbor
                << " entries are over the limit for switchId: " << switchId;
      XLOG(ERR) << neighbor << " entries : " << count << " exceeds the limit: "
                << getMaxNeighborTableSize<TableT>(
                       switchId, FLAGS_neighbhor_resource_percentage)
                       .value();
    }
  }

  return isValidUpdate;
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
    isValidUpdate &= neighborStateChangedImpl<NdpTable>(delta);
    isValidUpdate &= neighborStateChangedImpl<ArpTable>(delta);
  }

  return isValidUpdate;
}

} // namespace facebook::fboss
