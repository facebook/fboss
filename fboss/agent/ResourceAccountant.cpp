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

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/SwitchState.h"

DEFINE_int32(
    ecmp_resource_percentage,
    75,
    "Percentage of ECMP resources (out of 100) allowed to use before ResourceAccountant rejects the update.");

namespace {
constexpr auto kHundredPercentage = 100;
} // namespace

namespace facebook::fboss {

ResourceAccountant::ResourceAccountant(const HwAsicTable* asicTable)
    : asicTable_(asicTable) {
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
  for (const auto& [_, hwAsic] : asicTable_->getHwAsics()) {
    if (hwAsic->getMaxEcmpGroups().has_value() ||
        hwAsic->getMaxEcmpMembers().has_value()) {
      return true;
    }
  }
  return false;
}

bool ResourceAccountant::stateChangedImpl(const StateDelta& delta) {
  if (!checkRouteUpdate_) {
    return true;
  }
  bool validRouteUpdate = true;

  auto processRoutesDelta = [&](const auto& routesDelta) {
    DeltaFunctions::forEachChanged(
        routesDelta,
        [&](const auto& oldRoute, const auto& newRoute) {
          validRouteUpdate &= checkAndUpdateEcmpResource(newRoute, true);
          validRouteUpdate &= checkAndUpdateEcmpResource(oldRoute, false);
          return LoopAction::CONTINUE;
        },
        [&](const auto& newRoute) {
          validRouteUpdate &= checkAndUpdateEcmpResource(newRoute, true);
          return LoopAction::CONTINUE;
        },
        [&](const auto& delRoute) {
          validRouteUpdate &= checkAndUpdateEcmpResource(delRoute, false);
          return LoopAction::CONTINUE;
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
  bool validRouteUpdate = stateChangedImpl(delta);
  if (!validRouteUpdate) {
    XLOG(WARNING)
        << "Invalid route update - exceeding ECMP resource limits. New state consumes "
        << ecmpMemberUsage_ << " ECMP members and " << ecmpGroupRefMap_.size()
        << " ECMP groups.";
    for (const auto& [switchId, hwAsic] : asicTable_->getHwAsics()) {
      const auto ecmpGroupLimit = hwAsic->getMaxEcmpGroups();
      const auto ecmpMemberLimit = hwAsic->getMaxEcmpMembers();
      XLOG(WARNING) << "ECMP resource limits for Switch " << switchId
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

void ResourceAccountant::stateChanged(const StateDelta& delta) {
  stateChangedImpl(delta);
}

template bool
ResourceAccountant::checkAndUpdateEcmpResource<folly::IPAddressV6>(
    const std::shared_ptr<Route<folly::IPAddressV6>>& route,
    bool add);

template bool
ResourceAccountant::checkAndUpdateEcmpResource<folly::IPAddressV4>(
    const std::shared_ptr<Route<folly::IPAddressV4>>& route,
    bool add);

} // namespace facebook::fboss
