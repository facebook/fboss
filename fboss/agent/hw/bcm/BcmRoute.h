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
#include <opennsl/types.h>
#include <opennsl/l3.h>
}

#include <folly/dynamic.h>
#include <folly/IPAddress.h>
#include "fboss/agent/types.h"
#include "fboss/agent/state/RouteForwardInfo.h"

#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

class BcmSwitch;
class BcmHost;

/**
 * BcmRoute represents a L3 route object.
 */
class BcmRoute {
 public:
  BcmRoute(const BcmSwitch* hw, opennsl_vrf_t vrf,
           const folly::IPAddress& addr, uint8_t len);
  ~BcmRoute();
  void program(const RouteForwardInfo& fwd);
  static bool deleteLpmRoute(int unit,
                             opennsl_vrf_t vrf,
                             const folly::IPAddress& prefix,
                             uint8_t prefixLength);
  static void initL3RouteFromArgs(opennsl_l3_route_t* rt,
                                  opennsl_vrf_t vrf,
                                  const folly::IPAddress& prefix,
                                  uint8_t prefixLength);
  folly::dynamic toFollyDynamic() const;

 private:
  void programHostRoute(opennsl_if_t egressId,
                        const RouteForwardInfo& fwd,
                        bool replace);
  void programLpmRoute(opennsl_if_t egressId, const RouteForwardInfo& fwd);
  /*
   * Check whether we can use the host route table. BCM platforms
   * support this from TD2 onwards
   */
  bool isHostRoute() const;
  bool canUseHostTable() const;
  // no copy or assign
  BcmRoute(const BcmRoute &) = delete;
  BcmRoute& operator=(const BcmRoute &) = delete;
  const BcmSwitch* hw_;
  opennsl_vrf_t vrf_;
  folly::IPAddress prefix_;
  uint8_t len_;
  RouteForwardInfo fwd_;
  bool added_{false};           // if the route added to HW or not
  opennsl_if_t egressId_{-1};
  void initL3RouteT(opennsl_l3_route_t* rt) const;
};

class BcmRouteTable {
 public:
  explicit BcmRouteTable(const BcmSwitch* hw);
  ~BcmRouteTable();
  // throw an error if not found
  BcmRoute* getBcmRoute(
      opennsl_vrf_t vrf, const folly::IPAddress& prefix, uint8_t len) const;
  // return nullptr if not found
  BcmRoute* getBcmRouteIf(
      opennsl_vrf_t vrf, const folly::IPAddress& prefix, uint8_t len) const;

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock in BcmSwitch::lock_ for the protection.
   */
  template<typename RouteT>
  void addRoute(opennsl_vrf_t vrf, const RouteT *route);
  template<typename RouteT>
  void deleteRoute(opennsl_vrf_t vrf, const RouteT *route);
  folly::dynamic toFollyDynamic() const;
 private:
  struct Key {
    folly::IPAddress network;
    uint8_t mask;
    opennsl_vrf_t vrf;
    bool operator<(const Key& k2) const;
  };
  const BcmSwitch *hw_;
  boost::container::flat_map<Key, std::unique_ptr<BcmRoute>> fib_;
};

}}
