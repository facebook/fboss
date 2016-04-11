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
#include "fboss/agent/state/Route.h"
#include "SaiRouteTable.h"
#include "SaiSwitch.h"
#include "SaiError.h"
#include "SaiRoute.h"

namespace facebook { namespace fboss {

bool SaiRouteTable::Key::operator<(const Key &k2) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (vrf < k2.vrf) {
    return true;
  } else if (vrf > k2.vrf) {
    return false;
  }

  if (mask < k2.mask) {
    return true;
  } else if (mask > k2.mask) {
    return false;
  }

  return network < k2.network;
}

SaiRouteTable::SaiRouteTable(const SaiSwitch *hw)
  : hw_(hw) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiRouteTable::~SaiRouteTable() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiRoute *SaiRouteTable::GetRouteIf(
  RouterID vrf,
  const folly::IPAddress &network,
  uint8_t mask) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  Key key {network, mask, vrf};
  auto iter = fib_.find(key);

  if (iter == fib_.end()) {
    return nullptr;
  }

  return iter->second.get();
}

SaiRoute *SaiRouteTable::GetRoute(
  RouterID vrf,
  const folly::IPAddress &network,
  uint8_t mask) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto rt = GetRouteIf(vrf, network, mask);
  if (!rt) {
    throw SaiError("Cannot find route for ",
                   network, "/", mask, ", vrf ", vrf.t);
  }

  return rt;
}

template<typename RouteT>
void SaiRouteTable::AddRoute(RouterID vrf, const RouteT *route) {
  VLOG(4) << "Entering " << __FUNCTION__;

  const auto &prefix = route->prefix();
  Key key {folly::IPAddress(prefix.network), prefix.mask, vrf};
  auto ret = fib_.emplace(key, nullptr);

  if (ret.second) {
    SCOPE_FAIL {
      fib_.erase(ret.first);
    };
    ret.first->second.reset(new SaiRoute(hw_, vrf,
                                         folly::IPAddress(prefix.network),
                                         prefix.mask));
  }

  auto pRoute = ret.first->second.get();
  pRoute->Program(route->getForwardInfo());

  if (!pRoute->isResolved()) {
    unresolvedRoutes_.insert(ret.first->second.get());
  }
}

template<typename RouteT>
void SaiRouteTable::DeleteRoute(RouterID vrf, const RouteT *route) {
  VLOG(4) << "Entering " << __FUNCTION__;

  const auto &prefix = route->prefix();
  Key key {folly::IPAddress(prefix.network), prefix.mask, vrf};
  auto iter = fib_.find(key);

  if (iter == fib_.end()) {
    throw SaiError("Failed to delete a non-existing route ", route->str());
  }

  auto unresIter = unresolvedRoutes_.find(iter->second.get());
  if (unresIter != unresolvedRoutes_.end()) {
    unresolvedRoutes_.erase(unresIter);
  }

  fib_.erase(iter);
}

void SaiRouteTable::onResolved(InterfaceID intf, const folly::IPAddress &ip) {

  VLOG(4) << "Entering " << __FUNCTION__;

  auto iter = unresolvedRoutes_.begin();

  while (iter != unresolvedRoutes_.end()) {
    try {
      (*iter)->onResolved(intf, ip);

      if ((*iter)->isResolved()) {
        iter = unresolvedRoutes_.erase(iter);
        continue;
      }

    } catch (const SaiError &e) {
      LOG(ERROR) << e.what();
    }

    ++iter;
  }
}

template void SaiRouteTable::AddRoute(RouterID, const RouteV4 *);
template void SaiRouteTable::AddRoute(RouterID, const RouteV6 *);
template void SaiRouteTable::DeleteRoute(RouterID, const RouteV4 *);
template void SaiRouteTable::DeleteRoute(RouterID, const RouteV6 *);

}} // facebook::fboss
