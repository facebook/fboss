/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#pragma once

#include <boost/container/flat_map.hpp>
#include <folly/IPAddress.h>

#include "fboss/agent/state/RouteForwardInfo.h"
#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
#include "sairoute.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;

class SaiRoute {
public:
  /**
   * Constructs the SaiRoute object.
   *
   * This method doesn't make any calls to the SAI to program route there.
   * Program() will be called soon after construction, and any
   * actual initialization logic is be performed there.
   */
  SaiRoute(const SaiSwitch *hw,
           RouterID vrf,
           const folly::IPAddress &addr,
           uint8_t prefixLen);
  /**
   * Destructs the SaiRoute object.
   *
   * This method removes the route from the HW and unreferences vrf and 
   * next hops used.
   */
  virtual ~SaiRoute();
  /**
   * Programs route in HW and stores sai_unicast_route_entry_t
   * which holds SAI route data needed. 
   *  
   * If one of SAI HW calls fails it throws an exception; 
   *  
   * @return none
   */
  void Program(const RouteForwardInfo &fwd);

  /**
   * Checks whether the Route is resolved.  
   *  
   * @return 'true' if Route is resolved otherwise 'false'
   */
  bool isResolved() const {
    return resolved_;
  }

  /**
   * Adds Next Hop to Route on HW in case the unresolved 
   * Next Hop with "ip" is resolved.
   *  
   * @return none
   */
  void onResolved(InterfaceID intf, const folly::IPAddress &ip);

private:
  SaiRoute(const SaiRoute &rhs);
  SaiRoute &operator=(const SaiRoute &rhs);

  const SaiSwitch *hw_;
  RouterID vrf_;
  folly::IPAddress ipAddr_;
  uint8_t prefixLen_;
  RouteForwardInfo fwd_;
  bool resolved_ {false};

  sai_unicast_route_entry_t routeEntry_;
  bool added_ {false};
  bool isLocal_ {false};

  sai_route_api_t *pSaiRouteApi_ { nullptr };
};

}} // facebook::fboss
