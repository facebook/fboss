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

#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/AddressUtil.h"

using facebook::network::toBinaryAddress;

namespace {
constexpr auto kPrefix = "prefix";
constexpr auto kNextHops = "nexthops"; // Necessary for reading old data
constexpr auto kNextHopsMulti = "nexthopsmulti";
constexpr auto kFwdInfo = "forwardingInfo";
constexpr auto kFlags = "flags";
}
namespace facebook { namespace fboss {

using std::string;
using folly::IPAddress;

// RouteFields<> Class
template<typename AddrT>
RouteFields<AddrT>::RouteFields(const Prefix& prefix) : prefix(prefix) {
}

template<typename AddrT>
RouteFields<AddrT>::RouteFields(const RouteFields& rf,
    CopyBehavior copyBehavior) : prefix(rf.prefix) {
  switch(copyBehavior) {
  case COPY_PREFIX_AND_NEXTHOPS:
    nexthopsmulti = rf.nexthopsmulti;
    break;
  default:
    throw FbossError("Unknown CopyBehavior passed to RouteFields ctor");
  }
}

template<typename AddrT>
bool RouteFields<AddrT>::operator==(const RouteFields& rf) const {
  return (flags == rf.flags
          && prefix == rf.prefix
          && nexthopsmulti == rf.nexthopsmulti
          && fwd == rf.fwd);
}

template<typename AddrT>
folly::dynamic RouteFields<AddrT>::toFollyDynamic() const {
  folly::dynamic routeFields = folly::dynamic::object;
  routeFields[kPrefix] = prefix.toFollyDynamic();
  routeFields[kNextHopsMulti] = nexthopsmulti.toFollyDynamic();
  //
  // For backward compatibility, we will also write the 1-best
  // nexthops list to the [kNextHops] field.  This is just so,
  // if a machine is reverted from a multi-nexthop code base back
  // to a code base that's not aware of multi-nexthops, it can continue
  // serving traffic without an outage.  Please remove this code
  // once all developers have rebased beyond the diff containing this
  // message.
  folly::dynamic nhopsList = folly::dynamic::array;
  if (!nexthopsmulti.isEmpty()) {
    for (const auto& nhop: nexthopsmulti.bestNextHopList()) {
      nhopsList.push_back(nhop.str());
    }
  }
  routeFields[kNextHops] = std::move(nhopsList);
  // End of backward-compatibility code
  //

  routeFields[kFwdInfo] = fwd.toFollyDynamic();
  routeFields[kFlags] = flags;
  return routeFields;
}

template<typename AddrT>
RouteDetails RouteFields<AddrT>::toRouteDetails() const {
  RouteDetails rd;
  // Add the prefix
  rd.dest.ip = toBinaryAddress(prefix.network);
  rd.dest.prefixLength = prefix.mask;
  // Add the action
  rd.action = forwardActionStr(fwd.getAction());
  // Add the forwarding info
  for (const auto& nh : fwd.getNexthops()) {
    IfAndIP ifAndIp;
    ifAndIp.interfaceID = nh.intf;
    ifAndIp.ip = toBinaryAddress(nh.nexthop);
    rd.fwdInfo.push_back(ifAndIp);
  }

  // Add the multi-nexthops
  rd.nextHopMulti = nexthopsmulti.toThrift();
  return rd;
}

template<typename AddrT>
RouteFields<AddrT>
RouteFields<AddrT>::fromFollyDynamic(const folly::dynamic& routeJson) {
  RouteFields rt(Prefix::fromFollyDynamic(routeJson[kPrefix]));
  if (routeJson.find(kNextHopsMulti) != routeJson.items().end()) {
    rt.nexthopsmulti =
      std::move(RouteNextHopsMulti::
                fromFollyDynamic(routeJson[kNextHopsMulti]));
  } else {
    // If routeJson[kNextMultiHops] does not exist, we're reading json that
    // was serialized by an older version of the code.
    // We will assume the nexthops were provided by BGPd.
    // It kind of doesn't matter.  In about 15 seconds, BGPd will have
    // assembled a new set of routes, and will replace these routes en masse.
    //
    // This code supporting routeJson[kNextHops] can be removed once all fboss
    // switches have been upgraded.
    //
    // This check for size is necessary since the old code serialized nexthop
    // lists of size zero even for non-NEXTHOPS routes.
    if (routeJson[kNextHops].size() > 0) {
      RouteNextHops nhops;
      for (const auto& nhop: routeJson[kNextHops]) {
        nhops.emplace(nhop.stringPiece());
      }
      rt.nexthopsmulti.update(ClientID((int32_t)StdClientIds::BGPD), nhops);
    }
  }
  rt.fwd = std::move(RouteForwardInfo::fromFollyDynamic(routeJson[kFwdInfo]));
  rt.flags = routeJson[kFlags].asInt();
  return rt;
}

template class RouteFields<folly::IPAddressV4>;
template class RouteFields<folly::IPAddressV6>;

// Route<> Class
template<typename AddrT>
Route<AddrT>::Route(const Prefix& prefix,
                    InterfaceID intf, const IPAddress& addr)
    : RouteBase(prefix) {
  update(intf, addr);
}

template<typename AddrT>
Route<AddrT>::Route(const Prefix& prefix, ClientID clientId,
                    const RouteNextHops& nhs)
    : RouteBase(prefix) {
  update(clientId, nhs);
}

template<typename AddrT>
Route<AddrT>::Route(const Prefix& prefix, ClientID client, RouteNextHops&& nhs)
    : RouteBase(prefix) {
  update(client, std::move(nhs));
}

template<typename AddrT>
Route<AddrT>::Route(const Prefix& prefix, Action action)
    : RouteBase(prefix) {
  update(action);
}

template<typename AddrT>
Route<AddrT>::~Route() {
}

template<typename AddrT>
std::string Route<AddrT>::str() const {
  std::string ret;
  ret = folly::to<string>(prefix(), '@');
  ret.append(RouteBase::getFields()->nexthopsmulti.str());
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
  ret.append(RouteBase::getFields()->fwd.str());
  return ret;
}

template<typename AddrT>
bool Route<AddrT>::isSame(InterfaceID intf, const IPAddress& addr) const {
  if (!isConnected()) {
    return false;
  }
  const auto& fwd = RouteBase::getFields()->fwd;
  const auto& nhops = fwd.getNexthops();
  CHECK_EQ(nhops.size(), 1);
  const auto iter = nhops.begin();
  return iter->intf == intf && iter->nexthop == addr;
}

template<typename AddrT>
bool Route<AddrT>::isSame(ClientID clientId, const RouteNextHops& nhs) const {
  return RouteBase::getFields()->nexthopsmulti.isSame(clientId, nhs);
}

template<typename AddrT>
bool Route<AddrT>::isSame(Action action) const {
  CHECK(action == Action::DROP || action == Action::TO_CPU);
  if (action == Action::DROP) {
    return isDrop();
  } else {
    return isToCPU();
  }
}

template<typename AddrT>
bool Route<AddrT>::isSame(const Route<AddrT>* rt) const {
  return *this->getFields() == *rt->getFields();
}

template<typename AddrT>
void Route<AddrT>::update(InterfaceID intf, const IPAddress& addr) {
  // clear all existing nexthop info
  RouteBase::writableFields()->nexthopsmulti.clear();
  // replace the forwarding info for this route with just one nexthop
  RouteBase::writableFields()->fwd.setNexthops(intf, addr);
  setFlagsConnected();
}

template<typename AddrT>
void Route<AddrT>::updateNexthopCommon(const RouteNextHops& nhs) {
  if (!nhs.size()) {
    throw FbossError("Update with an empty set of nexthops for route ", str());
  }
  RouteBase::writableFields()->fwd.reset();
  clearFlags();
}

template<typename AddrT>
void Route<AddrT>::update(ClientID clientid, const RouteNextHops& nhs) {
  updateNexthopCommon(nhs);
  RouteBase::writableFields()->nexthopsmulti.update(clientid, nhs);
}

template<typename AddrT>
void Route<AddrT>::update(ClientID clientid, RouteNextHops&& nhs) {
  updateNexthopCommon(nhs);
  RouteBase::writableFields()->nexthopsmulti.update(clientid, std::move(nhs));
}

template<typename AddrT>
void Route<AddrT>::update(Action action) {
  CHECK(action == Action::DROP || action == Action::TO_CPU);
  // clear all existing nexthop info
  RouteBase::writableFields()->nexthopsmulti.clear();
  if (action == Action::DROP) {
    this->writableFields()->fwd.setDrop();
    setFlagsResolved();
  } else {
    this->writableFields()->fwd.setToCPU();
    setFlagsResolved();
  }
}

template<typename AddrT>
void Route<AddrT>::delNexthopsForClient(ClientID clientId) {
  RouteBase::writableFields()->nexthopsmulti.delNexthopsForClient(clientId);
}

template<typename AddrT>
void Route<AddrT>::setUnresolvable() {
  RouteBase::writableFields()->fwd.reset();
  setFlagsUnresolvable();
}

template class Route<folly::IPAddressV4>;
template class Route<folly::IPAddressV6>;

}}
