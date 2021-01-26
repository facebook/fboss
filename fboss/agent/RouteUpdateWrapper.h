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

#include <folly/IPAddress.h>
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/types.h"

#include "fboss/agent/rib/RoutingInformationBase.h"

namespace facebook::fboss {

/*
 * Wrapper class to handle route updates and programming across both
 * stand alone RIB and legacy setups
 */
class RouteUpdateWrapper {
 public:
  explicit RouteUpdateWrapper(rib::RoutingInformationBase* rib) : rib_(rib) {}
  explicit RouteUpdateWrapper(const std::shared_ptr<RouteTableMap>& routeTables)
      : routeUpdater_(std::make_unique<RouteUpdater>(routeTables)) {}

  virtual ~RouteUpdateWrapper() = default;
  void addRoute(
      RouterID id,
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientId,
      RouteNextHopEntry entry);

  void delRoute(
      RouterID id,
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientId);
  virtual void program() = 0;

 private:
  rib::RoutingInformationBase* rib_;
  std::unique_ptr<RouteUpdater> routeUpdater_;
  std::unordered_map<std::pair<RouterID, ClientID>, std::vector<UnicastRoute>>
      ribRoutesToAdd_;
  std::unordered_map<std::pair<RouterID, ClientID>, std::vector<IpPrefix>>
      ribRoutesToDelete_;
};
} // namespace facebook::fboss
