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
RouteFields<AddrT>::RouteFields(const Prefix& prefix)
    : RouteFields(getRouteFields(prefix)) {}

template <typename AddrT>
bool RouteFields<AddrT>::operator==(const RouteFields& rf) const {
  return this->data() == rf.data();
}

template <typename AddrT>
RouteDetails RouteFields<AddrT>::toRouteDetails(
    bool normalizedNhopWeights) const {
  RouteDetails rd;
  if constexpr (
      std::is_same_v<folly::IPAddressV6, AddrT> ||
      std::is_same_v<folly::IPAddressV4, AddrT>) {
    // Add the prefix
    rd.dest()->ip() = toBinaryAddress(prefix().network());
    rd.dest()->prefixLength() = prefix().mask();
  }
  // Add the action
  rd.action() = forwardActionStr(fwd().getAction());
  auto nhopSet = normalizedNhopWeights ? fwd().normalizedNextHops()
                                       : fwd().getNextHopSet();
  // Add the forwarding info
  for (const auto& nh : nhopSet) {
    IfAndIP ifAndIp;
    *ifAndIp.interfaceID() = nh.intf();
    *ifAndIp.ip() = toBinaryAddress(nh.addr());
    rd.fwdInfo()->push_back(ifAndIp);
    rd.nextHops()->push_back(nh.toThrift());
  }

  // Add the multi-nexthops
  rd.nextHopMulti() = nexthopsmulti().toThriftLegacy();
  rd.isConnected() = isConnected();
  // add counter id
  if (fwd().getCounterID().has_value()) {
    rd.counterID() = *fwd().getCounterID();
  }
  // add class id
  if (fwd().getClassID().has_value()) {
    rd.classID() = *fwd().getClassID();
  }
  return rd;
}

template <typename AddrT>
void RouteFields<AddrT>::update(
    ClientID clientId,
    const RouteNextHopEntry& entry) {
  this->writableData().fwd() = state::RouteNextHopEntry{};
  RouteNextHopsMulti::update(
      clientId, *(this->writableData().nexthopsmulti()), entry.toThrift());
}
template <typename AddrT>

bool RouteFields<AddrT>::has(ClientID clientId, const RouteNextHopEntry& entry)
    const {
  auto found = RouteNextHopsMulti::getEntryForClient(
      clientId, *(this->data().nexthopsmulti()));
  return found and *found == entry;
}

template <typename AddrT>
std::string RouteFields<AddrT>::strLegacy() const {
  std::string ret;
  ret = folly::to<string>(prefix(), '@');
  ret.append(nexthopsmulti().strLegacy());
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
  ret.append(fwd().str());

  auto _classID = classID();
  auto classIDStr = _classID.has_value()
      ? folly::to<std::string>(static_cast<int>(_classID.value()))
      : "None";
  ret.append(", classID: ");
  ret.append(classIDStr);

  return ret;
}

template <typename AddrT>
void RouteFields<AddrT>::delEntryForClient(ClientID clientId) {
  RouteNextHopsMulti::delEntryForClient(
      clientId, *(this->writableData().nexthopsmulti()));
}

template <typename AddrT>
ThriftFieldsT<AddrT> RouteFields<AddrT>::getRouteFields(
    const PrefixT<AddrT>& prefix,
    const RouteNextHopsMulti& multi,
    const RouteNextHopEntry& fwd,
    uint32_t flags,
    const std::optional<cfg::AclLookupClass>& classID) {
  ThriftFieldsT<AddrT> fields{};
  if constexpr (std::is_same_v<AddrT, LabelID>) {
    fields.label() = prefix.toThrift();
  } else {
    fields.prefix() = prefix.toThrift();
  }
  fields.nexthopsmulti() = multi.toThrift();
  fields.fwd() = fwd.toThrift();
  fields.flags() = flags;
  if (classID) {
    fields.classID() = *classID;
  }
  return fields;
}

template struct RouteFields<folly::IPAddressV4>;
template struct RouteFields<folly::IPAddressV6>;
template struct RouteFields<LabelID>;

template <typename AddrT>
bool Route<AddrT>::isSame(const Route<AddrT>* rt) const {
  return *this == *rt;
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> Route<AddrT>::fromFollyDynamic(
    const folly::dynamic& json) {
  auto fields = RouteFields<AddrT>::fromFollyDynamic(json);
  return std::make_shared<Route<AddrT>>(fields.toThrift());
}

template <typename AddrT>
folly::dynamic Route<AddrT>::toFollyDynamic() const {
  auto fields = RouteFields<AddrT>::fromThrift(this->toThrift());
  return fields.toFollyDynamic();
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> Route<AddrT>::cloneForReresolve() const {
  auto unresolvedRoute = this->clone();

  unresolvedRoute->clearFlags();
  unresolvedRoute->clearForward();
  return unresolvedRoute;
}

template class Route<folly::IPAddressV4>;
template class Route<folly::IPAddressV6>;
template class Route<LabelID>;

} // namespace facebook::fboss
