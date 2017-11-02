/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmHostKey.h"

#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

BcmHostKey::BcmHostKey(
    opennsl_vrf_t vrf,
    folly::IPAddress ipAddr,
    folly::Optional<InterfaceID> intf)
    : RouteNextHop(std::move(ipAddr), intf), vrf_(vrf) {
  // need the interface ID if and only if the address is v6 link-local
  if (addr().isV6() && addr().isLinkLocal()) {
    if (!intfID().hasValue()) {
      throw FbossError(
          "Missing interface scoping for link-local address {}.", addr().str());
    }
  } else {
    // for not v6 link-local address, do not track the interface ID
    setIntfID(folly::none);
  }
}

std::string BcmHostKey::str() const {
  return folly::to<std::string>(RouteNextHop::str(), "@vrf", getVrf());
}

bool operator==(const BcmHostKey& a, const BcmHostKey& b) {
  return (a.getVrf() == b.getVrf() &&
          static_cast<RouteNextHop>(a) == static_cast<RouteNextHop>(b));
}

bool operator< (const BcmHostKey& a, const BcmHostKey& b) {
  return ((a.getVrf() == b.getVrf())
          ? (static_cast<RouteNextHop>(a) < static_cast<RouteNextHop>(b))
          : (a.getVrf() < b.getVrf()));
}

void toAppend(const BcmHostKey& key, std::string *result) {
  result->append(key.str());
}

std::ostream& operator<<(std::ostream& os, const BcmHostKey& key) {
  return os << key.str();
}

}}
