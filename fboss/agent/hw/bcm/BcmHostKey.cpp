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
    : vrf_(vrf), addr_(std::move(ipAddr)), intfID_(intf) {
  // need the interface ID if and only if the address is v6 link-local
  if (addr().isV6() && addr().isLinkLocal()) {
    if (!intfID().hasValue()) {
      throw FbossError(
          "Missing interface scoping for link-local address {}.", addr().str());
    }
  } else {
    // for not v6 link-local address, do not track the interface ID
    intfID_ = folly::none;
  }
}

std::string BcmHostKey::str() const {
  std::string intfStr = "";
  if (intfID_) {
    intfStr = folly::to<std::string>("@I", intfID_.value());
  }
  return folly::to<std::string>("BcmHost: ", addr_, intfStr, "@vrf", getVrf());
}

bool operator==(const BcmHostKey& a, const BcmHostKey& b) {
  return (
      a.getVrf() == b.getVrf() && a.addr() == b.addr() &&
      a.intfID() == b.intfID());
}

bool operator< (const BcmHostKey& a, const BcmHostKey& b) {
  if (a.getVrf() != b.getVrf()) {
    return a.getVrf() < b.getVrf();
  } else if (a.intfID() != b.intfID()) {
    return a.intfID() < b.intfID();
  } else {
    return a.addr() < b.addr();
  }
}

void toAppend(const BcmHostKey& key, std::string *result) {
  result->append(key.str());
}

std::ostream& operator<<(std::ostream& os, const BcmHostKey& key) {
  return os << key.str();
}

}}
