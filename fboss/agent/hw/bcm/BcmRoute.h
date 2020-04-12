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

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {

class BcmSwitch;
class BcmHost;
class BcmMultiPathNextHop;

/**
 * BcmRoute represents a L3 route object.
 */
class BcmRoute {
 public:
  BcmRoute(
      BcmSwitch* hw,
      bcm_vrf_t vrf,
      const folly::IPAddress& addr,
      uint8_t len,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);
  ~BcmRoute();
  void program(
      const RouteNextHopEntry& fwd,
      std::optional<cfg::AclLookupClass> classID);
  static bool deleteLpmRoute(
      int unit,
      bcm_vrf_t vrf,
      const folly::IPAddress& prefix,
      uint8_t prefixLength);
  static void initL3RouteFromArgs(
      bcm_l3_route_t* rt,
      bcm_vrf_t vrf,
      const folly::IPAddress& prefix,
      uint8_t prefixLength);

  bcm_if_t getEgressId() const {
    return egressId_;
  }

  /* used for tests */
  std::shared_ptr<BcmMultiPathNextHop> getNextHop() const {
    return nextHopHostReference_;
  }

 private:
  std::shared_ptr<BcmHost> programHostRoute(
      bcm_if_t egressId,
      const RouteNextHopEntry& fwd,
      bool replace);
  void programLpmRoute(
      bcm_if_t egressId,
      const RouteNextHopEntry& fwd,
      std::optional<cfg::AclLookupClass> classID);

  /*
   * Check whether we can use the host route table. BCM platforms
   * support this from TD2 onwards
   */
  bool isHostRoute() const;
  bool canUseHostTable() const;
  // no copy or assign
  BcmRoute(const BcmRoute&) = delete;
  BcmRoute& operator=(const BcmRoute&) = delete;
  BcmSwitch* hw_;
  bcm_vrf_t vrf_;
  folly::IPAddress prefix_;
  uint8_t len_;
  RouteNextHopEntry fwd_{RouteNextHopEntry::Action::DROP,
                         AdminDistance::MAX_ADMIN_DISTANCE};
  bool added_{false}; // if the route added to HW or not
  bcm_if_t egressId_{-1};
  void initL3RouteT(bcm_l3_route_t* rt) const;
  std::shared_ptr<BcmMultiPathNextHop>
      nextHopHostReference_; // reference to nexthops
  std::shared_ptr<BcmHost> hostRouteEntry_; // for host routes
  std::optional<cfg::AclLookupClass> classID_{std::nullopt};
};

class BcmRouteTable {
 public:
  explicit BcmRouteTable(BcmSwitch* hw);
  ~BcmRouteTable();
  // throw an error if not found
  BcmRoute*
  getBcmRoute(bcm_vrf_t vrf, const folly::IPAddress& prefix, uint8_t len) const;
  // return nullptr if not found
  BcmRoute* getBcmRouteIf(
      bcm_vrf_t vrf,
      const folly::IPAddress& prefix,
      uint8_t len) const;

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock in BcmSwitch::lock_ for the protection.
   */
  template <typename RouteT>
  void addRoute(bcm_vrf_t vrf, const RouteT* route);
  template <typename RouteT>
  void deleteRoute(bcm_vrf_t vrf, const RouteT* route);

 private:
  struct Key {
    folly::IPAddress network;
    uint8_t mask;
    bcm_vrf_t vrf;
    bool operator<(const Key& k2) const;
  };

  BcmSwitch* hw_;

  boost::container::flat_map<Key, std::unique_ptr<BcmRoute>> fib_;
};

} // namespace facebook::fboss
