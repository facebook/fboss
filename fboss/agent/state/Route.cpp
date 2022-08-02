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
#include "fboss/agent/state/Route.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "folly/IPAddressV4.h"

using facebook::network::toBinaryAddress;

namespace facebook::fboss {

using folly::IPAddress;
using std::string;

// RouteFields<> Class
template <typename AddrT>
RouteFields<AddrT>::RouteFields(const Prefix& prefix) : prefix(prefix) {}

template <typename AddrT>
bool RouteFields<AddrT>::operator==(const RouteFields& rf) const {
  return std::tie(flags, prefix, nexthopsmulti, fwd, classID) ==
      std::tie(rf.flags, rf.prefix, rf.nexthopsmulti, rf.fwd, rf.classID);
}

template <typename AddrT>
folly::dynamic RouteFields<AddrT>::toFollyDynamicLegacy() const {
  folly::dynamic routeFields = folly::dynamic::object;
  routeFields[kPrefix] = prefix.toFollyDynamicLegacy();
  routeFields[kNextHopsMulti] = nexthopsmulti.toFollyDynamicLegacy();
  routeFields[kFwdInfo] = fwd.toFollyDynamicLegacy();
  routeFields[kFlags] = flags;
  if (classID.has_value()) {
    routeFields[kClassID] = static_cast<int>(classID.value());
  }

  return routeFields;
}

template <typename AddrT>
RouteFields<AddrT> RouteFields<AddrT>::fromFollyDynamicLegacy(
    const folly::dynamic& routeJson) {
  RouteFields<AddrT> rt(Prefix::fromFollyDynamicLegacy(routeJson[kPrefix]));
  rt.nexthopsmulti =
      RouteNextHopsMulti::fromFollyDynamicLegacy(routeJson[kNextHopsMulti]);
  rt.fwd = RouteNextHopEntry::fromFollyDynamicLegacy(routeJson[kFwdInfo]);
  rt.flags = routeJson[kFlags].asInt();
  if (routeJson.find(kClassID) != routeJson.items().end()) {
    rt.classID = cfg::AclLookupClass(routeJson[kClassID].asInt());
  }
  return rt;
}

template <typename AddrT>
RouteDetails RouteFields<AddrT>::toRouteDetails(
    bool normalizedNhopWeights) const {
  RouteDetails rd;
  if constexpr (
      std::is_same_v<folly::IPAddressV6, AddrT> ||
      std::is_same_v<folly::IPAddressV4, AddrT>) {
    // Add the prefix
    rd.dest()->ip() = toBinaryAddress(prefix.network());
    rd.dest()->prefixLength() = prefix.mask();
  }
  // Add the action
  rd.action() = forwardActionStr(fwd.getAction());
  auto nhopSet =
      normalizedNhopWeights ? fwd.normalizedNextHops() : fwd.getNextHopSet();
  // Add the forwarding info
  for (const auto& nh : nhopSet) {
    IfAndIP ifAndIp;
    *ifAndIp.interfaceID() = nh.intf();
    *ifAndIp.ip() = toBinaryAddress(nh.addr());
    rd.fwdInfo()->push_back(ifAndIp);
    rd.nextHops()->push_back(nh.toThrift());
  }

  // Add the multi-nexthops
  rd.nextHopMulti() = nexthopsmulti.toThriftLegacy();
  rd.isConnected() = isConnected();
  // add counter id
  if (fwd.getCounterID().has_value()) {
    rd.counterID() = *fwd.getCounterID();
  }
  // add class id
  if (fwd.getClassID().has_value()) {
    rd.classID() = *fwd.getClassID();
  }
  return rd;
}

template <typename AddrT>
void RouteFields<AddrT>::update(ClientID clientId, RouteNextHopEntry entry) {
  fwd.reset();
  nexthopsmulti.update(clientId, std::move(entry));
}
template <typename AddrT>

bool RouteFields<AddrT>::has(ClientID clientId, const RouteNextHopEntry& entry)
    const {
  auto found = nexthopsmulti.getEntryForClient(clientId);
  return found and RouteNextHopEntry::fromThrift((*found)) == entry;
}

template <typename AddrT>
std::string RouteFields<AddrT>::str() const {
  std::string ret;
  ret = folly::to<string>(prefix, '@');
  ret.append(nexthopsmulti.strLegacy());
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

  auto classIDStr = classID.has_value()
      ? folly::to<std::string>(static_cast<int>(classID.value()))
      : "None";
  ret.append(", classID: ");
  ret.append(classIDStr);

  return ret;
}

template <typename AddrT>
void RouteFields<AddrT>::delEntryForClient(ClientID clientId) {
  nexthopsmulti.delEntryForClient(clientId);
}

template <typename AddrT>
folly::dynamic RouteFields<AddrT>::migrateToThrifty(folly::dynamic const& dyn) {
  folly::dynamic newDyn = folly::dynamic::object;
  if constexpr (std::is_same_v<AddrT, LabelID>) {
    newDyn["label"] =
        RouteFields<AddrT>::Prefix::migrateToThrifty(dyn[kPrefix]);
  } else {
    newDyn["prefix"] =
        RouteFields<AddrT>::Prefix::migrateToThrifty(dyn[kPrefix]);
  }
  newDyn["nexthopsmulti"] =
      RouteNextHopsMulti::migrateToThrifty(dyn[kNextHopsMulti]);
  newDyn["fwd"] = RouteNextHopEntry::migrateToThrifty(dyn[kFwdInfo]);
  newDyn["flags"] = dyn[kFlags].asInt();
  if (dyn.find(kClassID) != dyn.items().end()) {
    newDyn["classID"] = dyn[kClassID].asInt();
  }
  return newDyn;
}

template <typename AddrT>
void RouteFields<AddrT>::migrateFromThrifty(folly::dynamic& dyn) {
  if constexpr (std::is_same_v<AddrT, LabelID>) {
    ThriftyUtils::renameField(dyn, "label", std::string(kPrefix));
  }
  RouteFields<AddrT>::Prefix::migrateFromThrifty(dyn[kPrefix]);
  ThriftyUtils::renameField(dyn, "nexthopsmulti", std::string(kNextHopsMulti));
  ThriftyUtils::renameField(dyn, "fwd", std::string(kFwdInfo));
  RouteNextHopsMulti::migrateFromThrifty(dyn[kNextHopsMulti]);
  RouteNextHopEntry::migrateFromThrifty(dyn[kFwdInfo]);
  dyn[kFlags] = dyn["flags"].asInt();
  if (dyn.find("classID") != dyn.items().end()) {
    dyn[kClassID] = dyn["classID"].asInt();
  }
}

template struct RouteFields<folly::IPAddressV4>;
template struct RouteFields<folly::IPAddressV6>;
template struct RouteFields<LabelID>;

template <typename AddrT>
bool Route<AddrT>::isSame(const Route<AddrT>* rt) const {
  return *this->getFields() == *rt->getFields();
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> Route<AddrT>::fromFollyDynamicLegacy(
    const folly::dynamic& routeJson) {
  return std::make_shared<Route<AddrT>>(
      RouteFields<AddrT>::fromFollyDynamicLegacy(routeJson));
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> Route<AddrT>::cloneForReresolve() const {
  auto unresolvedRoute = this->clone();
  unresolvedRoute->writableFields()->flags = 0;
  unresolvedRoute->clearForward();
  return unresolvedRoute;
}

template class Route<folly::IPAddressV4>;
template class Route<folly::IPAddressV6>;
template class Route<LabelID>;

} // namespace facebook::fboss
