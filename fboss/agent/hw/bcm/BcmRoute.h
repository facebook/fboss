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
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmRouteCounter.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <map>

namespace facebook::fboss {

class BcmSwitch;
class BcmHostIf;
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
      RouteNextHopEntry&& fwd,
      std::optional<cfg::AclLookupClass> classID);

  bcm_if_t getEgressId() const {
    return egressId_;
  }

  /* used for tests */
  std::shared_ptr<BcmMultiPathNextHop> getNextHop() const {
    return nextHopHostReference_;
  }

 private:
  std::shared_ptr<BcmHostIf> programHostRoute(
      bcm_if_t egressId,
      const RouteNextHopEntry& fwd,
      bool replace);

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
  RouteNextHopEntry fwd_{
      RouteNextHopEntry::Action::DROP,
      AdminDistance::MAX_ADMIN_DISTANCE};
  bool added_{false}; // if the route added to HW or not
  bcm_if_t egressId_{-1};
  std::shared_ptr<BcmMultiPathNextHop>
      nextHopHostReference_; // reference to nexthops
  std::shared_ptr<BcmHostIf> hostRouteEntry_; // for host routes
  std::optional<cfg::AclLookupClass> classID_{std::nullopt};
  std::shared_ptr<BcmRouteCounterBase> counterIDReference_;
};

/**
 * BcmHostRoute represents a L3 host entry programmed to route table.
 */
class BcmHostRoute : public BcmHostIf {
 public:
  BcmHostRoute(BcmSwitch* hw, BcmHostKey key) : BcmHostIf(hw, key) {}
  void addToBcmHw(bool isMultipath = false, bool replace = false) override;
  ~BcmHostRoute() override;

 private:
  // no copy or assignment
  BcmHostRoute(BcmHostRoute const&) = delete;
  BcmHostRoute& operator=(BcmHostRoute const&) = delete;
};

// inherit and implement program host related APIs from BcmHostTableIf,
// so as to progrom routes for hosts if host table is not available
class BcmRouteTable : public BcmHostTableIf {
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

  BcmHostIf* getBcmHostIf(const BcmHostKey& key) const noexcept override;
  std::shared_ptr<BcmHostIf> refOrEmplaceHost(const BcmHostKey& key) override {
    auto rv = hostRoutes_.refOrEmplace(key, hw_, key);
    return rv.first;
  }
  int getNumBcmHostRoute() const {
    return hostRoutes_.size();
  }
  void releaseHosts() override {
    hostRoutes_.clear();
  }
  const FlatRefMap<BcmHostKey, BcmHostRoute>& getHostRoutes() const {
    return hostRoutes_;
  }

 private:
  struct Key {
    folly::IPAddress network;
    uint8_t mask;
    bcm_vrf_t vrf;
    bool operator<(const Key& k2) const;
  };

  BcmSwitch* hw_;

  // routes programmed from addRoute()
  std::map<Key, std::unique_ptr<BcmRoute>> fib_;
  // host routes programmed from programHostRoutes*()
  FlatRefMap<BcmHostKey, BcmHostRoute> hostRoutes_;
};

} // namespace facebook::fboss
