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

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeBase-defs.h"

using facebook::network::toBinaryAddress;

namespace facebook::fboss {

using folly::IPAddress;
using std::string;

// RouteFields<> Class
template <typename AddrT>
RouteFields<AddrT>::RouteFields(const Prefix& prefix) : prefix(prefix) {}

template <typename AddrT>
RouteFields<AddrT>::RouteFields(
    const RouteFields& rf,
    CopyBehavior copyBehavior)
    : prefix(rf.prefix) {
  switch (copyBehavior) {
    case COPY_PREFIX_AND_NEXTHOPS:
      nexthopsmulti = rf.nexthopsmulti;
      break;
    default:
      throw FbossError("Unknown CopyBehavior passed to RouteFields ctor");
  }
}

template <typename AddrT>
bool RouteFields<AddrT>::operator==(const RouteFields& rf) const {
  return (
      flags == rf.flags && prefix == rf.prefix &&
      nexthopsmulti == rf.nexthopsmulti && fwd == rf.fwd);
}

template <typename AddrT>
folly::dynamic Route<AddrT>::toFollyDynamic() const {
  folly::dynamic routeFields = folly::dynamic::object;
  routeFields[kPrefix] = RouteBase::getFields()->prefix.toFollyDynamic();
  routeFields[kNextHopsMulti] =
      RouteBase::getFields()->nexthopsmulti.toFollyDynamic();
  routeFields[kFwdInfo] = RouteBase::getFields()->fwd.toFollyDynamic();
  routeFields[kFlags] = RouteBase::getFields()->flags;
  if (RouteBase::getFields()->classID.has_value()) {
    routeFields[kClassID] =
        static_cast<int>(RouteBase::getFields()->classID.value());
  }

  return routeFields;
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> Route<AddrT>::fromFollyDynamic(
    const folly::dynamic& routeJson) {
  RouteFields<AddrT> rt(Prefix::fromFollyDynamic(routeJson[kPrefix]));
  rt.nexthopsmulti =
      RouteNextHopsMulti::fromFollyDynamic(routeJson[kNextHopsMulti]);
  rt.fwd = RouteNextHopEntry::fromFollyDynamic(routeJson[kFwdInfo]);
  rt.flags = routeJson[kFlags].asInt();
  if (routeJson.find(kClassID) != routeJson.items().end()) {
    rt.classID = cfg::AclLookupClass(routeJson[kClassID].asInt());
  }

  auto route = std::make_shared<Route<AddrT>>(rt);
  CHECK(!route->hasNoEntry());
  return route;
}

template <typename AddrT>
RouteDetails RouteFields<AddrT>::toRouteDetails() const {
  RouteDetails rd;
  // Add the prefix
  rd.dest.ip = toBinaryAddress(prefix.network);
  rd.dest.prefixLength = prefix.mask;
  // Add the action
  rd.action = forwardActionStr(fwd.getAction());
  // Add the forwarding info
  for (const auto& nh : fwd.getNextHopSet()) {
    IfAndIP ifAndIp;
    ifAndIp.interfaceID = nh.intf();
    ifAndIp.ip = toBinaryAddress(nh.addr());
    rd.fwdInfo.push_back(ifAndIp);
    rd.nextHops_ref()->push_back(nh.toThrift());
  }

  // Add the multi-nexthops
  rd.nextHopMulti = nexthopsmulti.toThrift();
  return rd;
}

template class RouteFields<folly::IPAddressV4>;
template class RouteFields<folly::IPAddressV6>;

template <typename AddrT>
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

  auto classIDStr = getClassID().has_value()
      ? folly::to<std::string>(static_cast<int>(getClassID().value()))
      : "None";
  ret.append(", classID: ");
  ret.append(classIDStr);

  return ret;
}

template <typename AddrT>
void Route<AddrT>::update(ClientID clientId, RouteNextHopEntry entry) {
  RouteBase::writableFields()->fwd.reset();
  RouteBase::writableFields()->nexthopsmulti.update(clientId, std::move(entry));
}

template <typename AddrT>
void Route<AddrT>::updateClassID(std::optional<cfg::AclLookupClass> classID) {
  RouteBase::writableFields()->classID = classID;
}

template <typename AddrT>
bool Route<AddrT>::has(ClientID clientId, const RouteNextHopEntry& entry)
    const {
  auto found =
      RouteBase::getFields()->nexthopsmulti.getEntryForClient(clientId);
  return found and (*found) == entry;
}

template <typename AddrT>
bool Route<AddrT>::isSame(const Route<AddrT>* rt) const {
  return *this->getFields() == *rt->getFields();
}

template <typename AddrT>
void Route<AddrT>::delEntryForClient(ClientID clientId) {
  RouteBase::writableFields()->nexthopsmulti.delEntryForClient(clientId);
}

template <typename AddrT>
void Route<AddrT>::setResolved(RouteNextHopEntry fwd) {
  RouteBase::writableFields()->fwd = std::move(fwd);
  setFlagsResolved();
}

template <typename AddrT>
void Route<AddrT>::setUnresolvable() {
  RouteBase::writableFields()->fwd.reset();
  setFlagsUnresolvable();
}

template <typename AddrT>
void Route<AddrT>::clearForward() {
  RouteBase::writableFields()->fwd.reset();
  clearForwardInFlags();
}

template class Route<folly::IPAddressV4>;
template class Route<folly::IPAddressV6>;

} // namespace facebook::fboss
