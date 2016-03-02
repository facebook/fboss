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

#include "fboss/agent/types.h"


namespace facebook { namespace fboss {

class SaiSwitch;
class SaiRoute;

class SaiRouteTable {
public:
  explicit SaiRouteTable(const SaiSwitch *hw);
  ~SaiRouteTable();
  // throw an error if not found
  SaiRoute *GetRoute(
    RouterID vrf, const folly::IPAddress &prefix, uint8_t len) const;
  // return nullptr if not found
  SaiRoute *GetRouteIf(
    RouterID vrf, const folly::IPAddress &prefix, uint8_t len) const;

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock for the protection.
   */
  template<typename RouteT>
  void AddRoute(RouterID vrf, const RouteT *route);
  template<typename RouteT>
  void DeleteRoute(RouterID vrf, const RouteT *route);
private:
  struct Key {
    folly::IPAddress network;
    uint8_t mask;
    RouterID vrf;
    bool operator<(const Key &k2) const;
  };
  const SaiSwitch *hw_;
  boost::container::flat_map<Key, std::unique_ptr<SaiRoute>> fib_;

};

}} // facebook::fboss
