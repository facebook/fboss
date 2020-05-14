/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteNextHop.h"

#include <folly/Conv.h>

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::rib {

namespace util {
NextHop fromThrift(const NextHopThrift& nht) {
  std::optional<LabelForwardingAction> action = std::nullopt;
  if (nht.mplsAction_ref()) {
    action = LabelForwardingAction::fromThrift(
        nht.mplsAction_ref().value_unchecked());
  }
  auto address = network::toIPAddress(*nht.address_ref());
  NextHopWeight weight = static_cast<NextHopWeight>(*nht.weight_ref());
  bool v6LinkLocal = address.isV6() and address.isLinkLocal();
  // Only honor interface specified over thrift if the address
  // is a v6 link-local. Otherwise, consume it as an unresolved
  // next hop and let route resolution populate the interface.
  if (nht.address_ref()->get_ifName() and v6LinkLocal) {
    InterfaceID intfID = facebook::fboss::util::getIDFromTunIntfName(
        *(nht.address_ref()->get_ifName()));
    return ResolvedNextHop(std::move(address), intfID, weight, action);
  } else {
    return UnresolvedNextHop(std::move(address), weight, action);
  }
}
NextHop nextHopFromFollyDynamic(const folly::dynamic& nhopJson) {
  folly::IPAddress address(nhopJson[kNexthop()].stringPiece());
  auto it = nhopJson.find(kInterface());
  std::optional<LabelForwardingAction> action = std::nullopt;
  auto labelAction = nhopJson.find(kLabelForwardingAction());
  if (labelAction != nhopJson.items().end()) {
    action = LabelForwardingAction::fromFollyDynamic(labelAction->second);
  }
  // NOTE: we use the ECMP weight (0) as the default here for proper
  // forward/backward compatibility. A quick explanation of the cases:
  // 1) Warm boot from agent with no notion of weight to one with a notion of
  // weight:
  //   All routes will come back from the warm boot file with a default weight
  //   of 0, which matches the fact that everything is ECMP. The first config
  //   application will result in interface/static routes getting a UCMP
  //   compatible weight of 1 (UCMP_DEFAULT_WEIGHT)
  // 2) Warm boot from agent with notion of weight back to one without a notion
  // of weight:
  //   All ECMP routes will be read in by code that is unaware of weight
  //   and will stay ECMP. Routes with UCMP weights will revert to ECMP
  //   at the SwitchState level seamlessly. TODO: test what happens to
  //   programmed UCMP routes in this case
  NextHopWeight weight(ECMP_WEIGHT);
  auto weightItr = nhopJson.find(kWeight);
  if (weightItr != nhopJson.items().end()) {
    weight = weightItr->second.asInt();
  }
  if (it != nhopJson.items().end()) {
    int64_t stored = it->second.asInt();
    if (stored < 0 || stored > std::numeric_limits<uint32_t>::max()) {
      throw FbossError("stored InterfaceID exceeds uint32_t limit");
    }
    InterfaceID intfID = InterfaceID(it->second.asInt());
    return ResolvedNextHop(std::move(address), intfID, weight, action);
  } else {
    return UnresolvedNextHop(std::move(address), weight, action);
  }
}
} // namespace util

void toAppend(const NextHop& nhop, std::string* result) {
  folly::toAppend(nhop.str(), result);
}

std::ostream& operator<<(std::ostream& os, const NextHop& nhop) {
  return os << nhop.str();
}

bool operator<(const NextHop& a, const NextHop& b) {
  if (a.intfID() != b.intfID()) {
    return a.intfID() < b.intfID();
  } else if (a.addr() != b.addr()) {
    return a.addr() < b.addr();
  } else if (a.labelForwardingAction() != b.labelForwardingAction()) {
    return a.labelForwardingAction() < b.labelForwardingAction();
  } else {
    return a.weight() < b.weight();
  }
}

bool operator>(const NextHop& a, const NextHop& b) {
  return (b < a);
}

bool operator<=(const NextHop& a, const NextHop& b) {
  return !(b < a);
}

bool operator>=(const NextHop& a, const NextHop& b) {
  return !(a < b);
}

bool operator==(const NextHop& a, const NextHop& b) {
  return (
      a.intfID() == b.intfID() && a.addr() == b.addr() &&
      a.weight() == b.weight() &&
      a.labelForwardingAction() == b.labelForwardingAction());
}

bool operator!=(const NextHop& a, const NextHop& b) {
  return !(a == b);
}

UnresolvedNextHop::UnresolvedNextHop(
    const folly::IPAddress& addr,
    const NextHopWeight& weight,
    const std::optional<LabelForwardingAction>& action)
    : addr_(addr), weight_(weight), labelForwardingAction_(action) {
  if (addr.isV6() and addr.isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop ", addr.str());
  }
}

UnresolvedNextHop::UnresolvedNextHop(
    folly::IPAddress&& addr,
    const NextHopWeight& weight,
    std::optional<LabelForwardingAction>&& action)
    : addr_(std::move(addr)),
      weight_(weight),
      labelForwardingAction_(std::move(action)) {
  if (addr.isV6() and addr.isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop ", addr.str());
  }
}

} // namespace facebook::fboss::rib
