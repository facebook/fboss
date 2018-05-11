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

namespace facebook { namespace fboss {

namespace util {
NextHop fromThrift(const NextHopThrift& nht) {
  auto address = network::toIPAddress(nht.address);
  bool v6LinkLocal = address.isV6() and address.isLinkLocal();
  // Only honor interface specified over thrift if the address
  // is a v6 link-local. Otherwise, consume it as an unresolved
  // next hop and let route resolution populate the interface.
  if (nht.address.get_ifName() and v6LinkLocal) {
    InterfaceID intfID =
        util::getIDFromTunIntfName(*(nht.address.get_ifName()));
    return ResolvedNextHop(std::move(address), intfID);
  } else {
    return UnresolvedNextHop(std::move(address));
  }
}

NextHop nextHopFromFollyDynamic(const folly::dynamic& nhopJson) {
  folly::IPAddress address(nhopJson[kNexthop()].stringPiece());
  auto it = nhopJson.find(kInterface());
  if (it != nhopJson.items().end()) {
    int64_t stored = it->second.asInt();
    if (stored < 0 || stored > std::numeric_limits<uint32_t>::max()) {
      throw FbossError("stored InterfaceID exceeds uint32_t limit");
    }
    InterfaceID intfID = InterfaceID(it->second.asInt());
    return ResolvedNextHop(std::move(address), intfID);
  } else {
    return UnresolvedNextHop(std::move(address));
  }
}
}

void toAppend(const NextHop& nhop, std::string *result) {
  folly::toAppend(nhop.str(), result);
}

std::ostream& operator<<(std::ostream& os, const NextHop& nhop) {
  return os << nhop.str();
}

bool operator<(const NextHop& a, const NextHop& b) {
  if (a.intfID() == b.intfID()) {
    return a.addr() < b.addr();
  } else {
    return a.intfID() < b.intfID();
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
  return (a.intfID() == b.intfID() && a.addr() == b.addr());
}

bool operator!=(const NextHop& a, const NextHop& b) {
  return !(a == b);
}

UnresolvedNextHop::UnresolvedNextHop(const folly::IPAddress& addr)
    : addr_(addr) {
  if (addr.isV6() and addr.isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop ", addr.str());
  }
}

UnresolvedNextHop::UnresolvedNextHop(folly::IPAddress&& addr)
    : addr_(std::move(addr)) {
  if (addr.isV6() and addr.isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop ", addr.str());
  }
}
} // namespace fboss
} // namespace facebook
