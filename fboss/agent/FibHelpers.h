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

#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

#include <memory>

namespace facebook::fboss {

class SwitchState;

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state);

template <typename Func>
void forAllRoutes(
    bool isStandaloneRib,
    const std::shared_ptr<SwitchState>& state,
    Func& func) {
  if (isStandaloneRib) {
    for (const auto& fibContainer : *state->getFibs()) {
      auto rid = fibContainer->getID();
      for (const auto& route : *(fibContainer->getFibV6())) {
        func(rid, route);
      }
      for (const auto& route : *(fibContainer->getFibV4())) {
        func(rid, route);
      }
    }
  } else {
    for (const auto& routeTable : *state->getRouteTables()) {
      auto rid = routeTable->getID();
      for (const auto& route : *(routeTable->getRibV6()->routes())) {
        func(rid, route);
      }
      for (const auto& route : *(routeTable->getRibV4()->routes())) {
        func(rid, route);
      }
    }
  }
}

} // namespace facebook::fboss
