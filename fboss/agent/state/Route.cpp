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

namespace {
constexpr auto kPrefix = "prefix";
constexpr auto kNextHops = "nexthops";
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
  case COPY_ALL_MEMBERS:
    nexthops = rf.nexthops;
    fwd = rf.fwd;
    flags = rf.flags;
    break;
  case COPY_ONLY_PREFIX:
    break;
  }
}

template<typename AddrT>
bool RouteFields<AddrT>::operator==(const RouteFields& rf) const {
  return (flags == rf.flags
          && prefix == rf.prefix
          && nexthops == rf.nexthops
          && fwd == rf.fwd);
}

template<typename AddrT>
folly::dynamic RouteFields<AddrT>::toFollyDynamic() const {
  folly::dynamic routeFields = folly::dynamic::object;
  routeFields[kPrefix] = prefix.toFollyDynamic();
  std::vector<folly::dynamic> nhopsList;
  for (const auto& nhop: nexthops) {
    nhopsList.emplace_back(nhop.str());
  }
  routeFields[kNextHops] = nhopsList;
  routeFields[kFwdInfo] = fwd.toFollyDynamic();
  routeFields[kFlags] = flags;
  return routeFields;
}

template<typename AddrT>
RouteFields<AddrT>
RouteFields<AddrT>::fromFollyDynamic(const folly::dynamic& routeJson) {
  RouteFields rt(Prefix::fromFollyDynamic(routeJson[kPrefix]));
  for (const auto& nhop: routeJson[kNextHops]) {
    rt.nexthops.emplace(nhop.stringPiece());
  }
  rt.fwd = RouteForwardInfo::fromFollyDynamic(routeJson[kFwdInfo]);
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
Route<AddrT>::Route(const Prefix& prefix, const RouteNextHops& nhs)
    : RouteBase(prefix) {
  update(nhs);
}

template<typename AddrT>
Route<AddrT>::Route(const Prefix& prefix, RouteNextHops&& nhs)
    : RouteBase(prefix) {
  update(std::move(nhs));
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
  for (const auto& nh : RouteBase::getFields()->nexthops) {
    ret.append(folly::to<string>(nh, "."));
  }
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
bool Route<AddrT>::isSame(const RouteNextHops& nhs) const {
  return RouteBase::getFields()->nexthops == nhs;
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
  RouteBase::writableFields()->nexthops.clear();
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
void Route<AddrT>::update(const RouteNextHops& nhs) {
  updateNexthopCommon(nhs);
  RouteBase::writableFields()->nexthops = nhs;
}

template<typename AddrT>
void Route<AddrT>::update(RouteNextHops&& nhs) {
  updateNexthopCommon(nhs);
  RouteBase::writableFields()->nexthops = std::move(nhs);
}

template<typename AddrT>
void Route<AddrT>::update(Action action) {
  CHECK(action == Action::DROP || action == Action::TO_CPU);
  // clear all existing nexthop info
  RouteBase::writableFields()->nexthops.clear();
  if (action == Action::DROP) {
    this->writableFields()->fwd.setDrop();
    setFlagsResolved();
  } else {
    this->writableFields()->fwd.setToCPU();
    setFlagsResolved();
  }
}

template<typename AddrT>
void Route<AddrT>::setUnresolvable() {
  RouteBase::writableFields()->fwd.reset();
  setFlagsUnresolvable();
}

template class Route<folly::IPAddressV4>;
template class Route<folly::IPAddressV6>;

}}
