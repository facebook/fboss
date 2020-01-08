/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#include "Route.h"

#include "fboss/agent/Constants.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook::fboss::rib {

template <typename AddrT>
Route<AddrT>::Route(const Prefix& prefix) : prefix_(prefix) {}

template <typename AddrT>
bool Route<AddrT>::operator==(const Route& rf) const {
  return (
      flags == rf.flags && prefix() == rf.prefix() &&
      nexthopsmulti == rf.nexthopsmulti && fwd == rf.fwd);
}

template <typename AddrT>
folly::dynamic Route<AddrT>::toFollyDynamic() const {
  folly::dynamic routeFields = folly::dynamic::object;
  routeFields[kPrefix] = prefix_.toFollyDynamic();
  routeFields[kNextHopsMulti] = nexthopsmulti.toFollyDynamic();
  routeFields[kFwdInfo] = fwd.toFollyDynamic();
  routeFields[kFlags] = flags;
  return routeFields;
}

template <typename AddrT>
Route<AddrT> Route<AddrT>::fromFollyDynamic(const folly::dynamic& routeJson) {
  Route route(Prefix::fromFollyDynamic(routeJson[kPrefix]));
  route.nexthopsmulti =
      RouteNextHopsMulti::fromFollyDynamic(routeJson[kNextHopsMulti]);
  route.fwd = RouteNextHopEntry::fromFollyDynamic(routeJson[kFwdInfo]);
  route.flags = routeJson[kFlags].asInt();

  CHECK(!route.hasNoEntry());
  return route;
}

template <typename AddrT>
RouteDetails Route<AddrT>::toRouteDetails() const {
  RouteDetails rd;
  // Add the prefix
  rd.dest.ip = network::toBinaryAddress(prefix_.network);
  rd.dest.prefixLength = prefix_.mask;
  // Add the action
  rd.action = forwardActionStr(fwd.getAction());
  // Add the forwarding info
  for (const auto& nh : fwd.getNextHopSet()) {
    IfAndIP ifAndIp;
    ifAndIp.interfaceID = nh.intf();
    ifAndIp.ip = network::toBinaryAddress(nh.addr());
    rd.fwdInfo.push_back(ifAndIp);
  }

  // Add the multi-nexthops
  rd.nextHopMulti = nexthopsmulti.toThrift();

  rd.isConnected = isConnected();

  return rd;
}

template <typename AddrT>
std::string Route<AddrT>::str() const {
  std::string ret;
  ret = folly::to<std::string>(prefix(), '@');
  ret.append(nexthopsmulti.str());
  ret.append(" State:");
  if (isConnected()) {
    ret.append("C");
  }
  if (isResolved()) {
    ret.append("R");
  }
  if (isUnresolvable()) {
    ret.append("U");
  }
  if (isProcessing()) {
    ret.append("P");
  }
  ret.append(", => ");
  ret.append(fwd.str());
  return ret;
}

template <typename AddrT>
void Route<AddrT>::update(ClientID clientId, RouteNextHopEntry entry) {
  fwd.reset();
  nexthopsmulti.update(clientId, std::move(entry));
}

template <typename AddrT>
bool Route<AddrT>::has(ClientID clientId, const RouteNextHopEntry& entry)
    const {
  auto found = nexthopsmulti.getEntryForClient(clientId);
  return found and (*found) == entry;
}

template <typename AddrT>
bool Route<AddrT>::isSame(const Route<AddrT>* rt) const {
  return *this == *rt;
}

template <typename AddrT>
void Route<AddrT>::delEntryForClient(ClientID clientId) {
  nexthopsmulti.delEntryForClient(clientId);
}

template <typename AddrT>
void Route<AddrT>::setResolved(RouteNextHopEntry fwd) {
  this->fwd = std::move(fwd);
  setFlagsResolved();
}

template <typename AddrT>
void Route<AddrT>::setUnresolvable() {
  fwd.reset();
  setFlagsUnresolvable();
}

template <typename AddrT>
void Route<AddrT>::clearForward() {
  fwd.reset();
  clearForwardInFlags();
}

template class Route<folly::IPAddressV4>;
template class Route<folly::IPAddressV6>;

} // namespace facebook::fboss::rib
