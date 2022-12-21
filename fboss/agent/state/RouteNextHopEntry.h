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

#include <folly/dynamic.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/Thrifty.h"

DECLARE_uint32(ecmp_width);
DECLARE_bool(optimized_ucmp);

namespace facebook::fboss {

class RouteNextHopEntry
    : public ThriftyFields<RouteNextHopEntry, state::RouteNextHopEntry> {
 public:
  using Action = RouteForwardAction;
  using NextHopSet = boost::container::flat_set<NextHop>;
  using AclLookupClass = cfg::AclLookupClass;

  RouteNextHopEntry(
      Action action,
      AdminDistance distance,
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt)
      : RouteNextHopEntry(getRouteNextHopEntryThrift(
            action,
            distance,
            NextHopSet(),
            counterID,
            classID)) {
    CHECK_NE(action, Action::NEXTHOPS);
  }

  RouteNextHopEntry(
      NextHopSet nhopSet,
      AdminDistance distance,
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt);

  RouteNextHopEntry(
      NextHop nhop,
      AdminDistance distance,
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt)
      : RouteNextHopEntry(getRouteNextHopEntryThrift(
            Action::NEXTHOPS,
            distance,
            NextHopSet({nhop}),
            counterID,
            classID)) {}

  explicit RouteNextHopEntry(const state::RouteNextHopEntry& entry);

  RouteNextHopEntry() {}
  RouteNextHopEntry(RouteNextHopEntry&& other) noexcept
      : RouteNextHopEntry(std::move(other.data_)) {}
  RouteNextHopEntry& operator=(RouteNextHopEntry&& other) noexcept {
    data_ = std::move(other.data_);
    return *this;
  }

  AdminDistance getAdminDistance() const {
    return *data().adminDistance();
  }

  Action getAction() const {
    return *data().action();
  }

  NextHopSet getNextHopSet() const;

  const std::optional<RouteCounterID> getCounterID() const {
    if (auto counter = data().counterID()) {
      return *counter;
    }
    return std::nullopt;
  }

  const std::optional<AclLookupClass> getClassID() const {
    if (auto classID = data().classID()) {
      return *classID;
    }
    return std::nullopt;
  }

  NextHopSet normalizedNextHops() const;

  // Get the sum of the weights of all the nexthops in the entry
  NextHopWeight getTotalWeight() const;

  std::string str() const;

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamicLegacy() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteNextHopEntry fromFollyDynamicLegacy(
      const folly::dynamic& entryJson);

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
    writableData() = state::RouteNextHopEntry{};
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

  state::RouteNextHopEntry toThrift() const override;
  static RouteNextHopEntry fromThrift(const state::RouteNextHopEntry& prefix);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);

 private:
  RouteNextHopEntry(RouteNextHopEntry const&) = delete;
  RouteNextHopEntry& operator=(RouteNextHopEntry const&) = delete;

  static state::RouteNextHopEntry getRouteNextHopEntryThrift(
      Action action,
      AdminDistance distance,
      NextHopSet nhopSet = NextHopSet(),
      std::optional<RouteCounterID> counterID = std::nullopt,
      std::optional<AclLookupClass> classID = std::nullopt);
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
