/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <boost/container/flat_set.hpp>

#include <folly/json/dynamic.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/Thrifty.h"

DECLARE_uint32(ecmp_width);
DECLARE_bool(optimized_ucmp);
DECLARE_bool(wide_ecmp);

namespace facebook::fboss {

USE_THRIFT_COW(RouteNextHopEntry);

template <>
struct thrift_cow::ThriftStructResolver<state::RouteNextHopEntry> {
  using type = RouteNextHopEntry;
};

class RouteNextHopEntry
    : public thrift_cow::ThriftStructNode<state::RouteNextHopEntry> {
 public:
  using Action = RouteForwardAction;
  using NextHopSet = boost::container::flat_set<NextHop>;
  using AclLookupClass = cfg::AclLookupClass;
  using BaseT = thrift_cow::ThriftStructNode<state::RouteNextHopEntry>;
  using BaseT::BaseT;

  RouteNextHopEntry(
      Action action,
      AdminDistance distance,
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt,
      std::optional<cfg::SwitchingMode> overrideEcmpSwitchingMode =
          std::nullopt,
      std::optional<NextHopSet> overrideNextHops = std::nullopt,
      std::optional<NextHopSetID> normalizedResolvedNextHopSetID = std::nullopt,
      std::optional<NextHopSetID> resolvedNextHopSetID = std::nullopt);
  RouteNextHopEntry(
      NextHopSet nhopSet,
      AdminDistance distance,
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt,
      std::optional<cfg::SwitchingMode> overrideEcmpSwitchingMode =
          std::nullopt,
      std::optional<NextHopSet> overrideNextHops = std::nullopt,
      std::optional<NextHopSetID> normalizedResolvedNextHopSetID = std::nullopt,
      std::optional<NextHopSetID> resolvedNextHopSetID = std::nullopt);

  RouteNextHopEntry(
      NextHop nhop,
      AdminDistance distance,
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt,
      std::optional<cfg::SwitchingMode> overrideEcmpSwitchingMode =
          std::nullopt,
      std::optional<NextHopSet> overrideNextHops = std::nullopt,
      std::optional<NextHopSetID> normalizedResolvedNextHopSetID = std::nullopt,
      std::optional<NextHopSetID> resolvedNextHopSetID = std::nullopt);

  RouteNextHopEntry(RouteNextHopEntry&& other) noexcept {
    this->fromThrift(other.toThrift());
  }
  RouteNextHopEntry& operator=(RouteNextHopEntry&& other) noexcept {
    this->fromThrift(other.toThrift());
    return *this;
  }

  AdminDistance getAdminDistance() const {
    return safe_cref<switch_state_tags::adminDistance>()->cref();
  }

  Action getAction() const {
    return safe_cref<switch_state_tags::action>()->cref();
  }

  NextHopSet getNextHopSet() const;

  const std::optional<RouteCounterID> getCounterID() const {
    if (auto counter = safe_cref<switch_state_tags::counterID>()) {
      return counter->cref();
    }
    return std::nullopt;
  }

  const std::optional<NextHopSetID> getNormalizedResolvedNextHopSetID() const {
    if (auto nhopSetID =
            safe_cref<switch_state_tags::normalizedResolvedNextHopSetID>()) {
      return NextHopSetID(nhopSetID->cref());
    }
    return std::nullopt;
  }

  void setNormalizedResolvedNextHopSetID(
      std::optional<NextHopSetID>& nhopSetID) {
    if (nhopSetID) {
      ref<switch_state_tags::normalizedResolvedNextHopSetID>() =
          static_cast<int64_t>(*nhopSetID);
    } else {
      ref<switch_state_tags::normalizedResolvedNextHopSetID>().reset();
    }
  }

  const std::optional<NextHopSetID> getResolvedNextHopSetID() const {
    if (auto nhopSetID = safe_cref<switch_state_tags::resolvedNextHopSetID>()) {
      return NextHopSetID(nhopSetID->cref());
    }
    return std::nullopt;
  }

  void setResolvedNextHopSetID(std::optional<NextHopSetID>& nhopSetID) {
    if (nhopSetID) {
      ref<switch_state_tags::resolvedNextHopSetID>() =
          static_cast<int64_t>(*nhopSetID);
    } else {
      ref<switch_state_tags::resolvedNextHopSetID>().reset();
    }
  }

  const std::optional<AclLookupClass> getClassID() const {
    if (auto classID = safe_cref<switch_state_tags::classID>()) {
      return classID->cref();
    }
    return std::nullopt;
  }

  const std::optional<cfg::SwitchingMode> getOverrideEcmpSwitchingMode() const {
    if (auto ecmpSwitchingMode =
            safe_cref<switch_state_tags::overrideEcmpSwitchingMode>()) {
      return ecmpSwitchingMode->cref();
    }
    return std::nullopt;
  }
  void setOverrideEcmpSwitchingMode(
      std::optional<cfg::SwitchingMode> switchingMode) {
    ref<switch_state_tags::overrideEcmpSwitchingMode>() = switchingMode;
  }

  const std::optional<NextHopSet> getOverrideNextHops() const;
  void setOverrideNextHops(const std::optional<NextHopSet>& nhops);
  bool hasOverrideSwitchingModeOrNhops() const;
  bool hasOverrideSwitchingMode() const;
  bool hasOverrideNextHops() const;
  NextHopSet normalizedNextHops() const {
    return normalizedNextHopsImpl(false /*ignoreOverride*/);
  }
  NextHopSet nonOverrideNormalizedNextHops() const {
    return normalizedNextHopsImpl(true /*ignoreOverride*/);
  }

  // Get the sum of the weights of all the nexthops in the entry
  NextHopWeight getTotalWeight() const;

  std::string str_DEPRACATED() const;

  std::string str() const;

  // Methods to manipulate this object
  bool isDrop() const {
    return getAction() == Action::DROP;
  }
  bool isToCPU() const {
    return getAction() == Action::TO_CPU;
  }
  bool isSame(const RouteNextHopEntry& entry) const {
    return entry.getAdminDistance() == getAdminDistance();
  }

  static bool isAction(const state::RouteNextHopEntry& entry, Action action) {
    return *entry.action() == action;
  }

  // Reset the NextHopSet
  void reset() {
    this->fromThrift(state::RouteNextHopEntry{});
  }

  bool isValid(bool forMplsRoute = false) const;

  static RouteNextHopEntry from(
      const facebook::fboss::UnicastRoute& route,
      AdminDistance defaultAdminDistance,
      std::optional<RouteCounterID> counterID,
      std::optional<AclLookupClass> classID);
  static RouteNextHopEntry from(
      const facebook::fboss::MplsRoute& route,
      AdminDistance defaultAdminDistance,
      std::optional<RouteCounterID> counterID,
      std::optional<AclLookupClass> classID);
  static facebook::fboss::RouteNextHopEntry createDrop(
      AdminDistance adminDistance = AdminDistance::STATIC_ROUTE);
  static facebook::fboss::RouteNextHopEntry createToCpu(
      AdminDistance adminDistance = AdminDistance::STATIC_ROUTE);
  static facebook::fboss::RouteNextHopEntry fromStaticRoute(
      const cfg::StaticRouteWithNextHops& route);
  static facebook::fboss::RouteNextHopEntry fromStaticIp2MplsRoute(
      const cfg::StaticIp2MplsRoute& route);
  static facebook::fboss::RouteNextHopEntry fromStaticMplsRoute(
      const cfg::StaticMplsRouteWithNextHops& route);
  static bool isUcmp(const NextHopSet& nhopSet);
  static void normalizeNextHopWeightsToMaxPaths(
      std::vector<uint64_t>& nhWeights,
      uint64_t normalizedPathCount);

 private:
  NextHopSet normalizedNextHopsImpl(bool ignoreOverride) const;
  static state::RouteNextHopEntry getRouteNextHopEntryThrift(
      Action action,
      AdminDistance distance,
      NextHopSet nhopSet,
      std::optional<RouteCounterID> counterID,
      std::optional<AclLookupClass> classID,
      std::optional<cfg::SwitchingMode> overrideEcmpSwitchingMode,
      const std::optional<NextHopSet>& overridNextHops,
      const std::optional<NextHopSetID>& normalizedResolvedNextHopSetID,
      const std::optional<NextHopSetID>& resolvedNextHopSetID);
  void normalize(
      std::vector<NextHopWeight>& scaledWeights,
      NextHopWeight totalWeight) const;
};

/**
 * Comparision operators
 */
bool operator==(const RouteNextHopEntry& a, const RouteNextHopEntry& b);
bool operator<(const RouteNextHopEntry& a, const RouteNextHopEntry& b);

void toAppend(const RouteNextHopEntry& entry, std::string* result);
std::ostream& operator<<(std::ostream& os, const RouteNextHopEntry& entry);

void toAppend(const RouteNextHopEntry::NextHopSet& nhops, std::string* result);
std::ostream& operator<<(
    std::ostream& os,
    const RouteNextHopEntry::NextHopSet& nhops);
NextHopWeight totalWeight(const RouteNextHopEntry::NextHopSet& nhops);

using RouteNextHopSet = RouteNextHopEntry::NextHopSet;

namespace util {

/**
 * Convert thrift representation of nexthops to RouteNextHops.
 */
RouteNextHopSet toRouteNextHopSet(
    std::vector<NextHopThrift> const& nhts,
    bool allowV6NonLinkLocal = false);

/**
 * Convert RouteNextHops to thrift representaion of nexthops
 */
std::vector<NextHopThrift> fromRouteNextHopSet(RouteNextHopSet const& nhs);

UnicastRoute toUnicastRoute(
    const folly::CIDRNetwork& nw,
    const RouteNextHopEntry& nhopEntry);
} // namespace util

} // namespace facebook::fboss
