/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteNextHopBase.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/state/StateUtils.h"

namespace facebook { namespace fboss {

RouteNextHopBase::RouteNextHopBase(
    folly::IPAddress addr,
    folly::Optional<InterfaceID> intfID)
    : addr_(std::move(addr)), intfID_(std::move(intfID)) {
}

network::thrift::BinaryAddress
RouteNextHopBase::toThrift() const {
  network::thrift::BinaryAddress nexthop = network::toBinaryAddress(addr_);
  if (intfID_) {
    nexthop.__isset.ifName = true;
    nexthop.ifName = util::createTunIntfName(intfID_.value());
  }
  return nexthop;
}

bool operator==(const RouteNextHopBase& a, const RouteNextHopBase& b) {
  return (a.intfID() == b.intfID() and a.addr() == b.addr());
}

bool operator< (const RouteNextHopBase& a, const RouteNextHopBase& b) {
  return ((a.intfID() == b.intfID())
          ? (a.addr() < b.addr())
          : (a.intfID() < b.intfID()));
}

std::string RouteNextHopBase::str() const {
  if (intfID_) {
    return folly::to<std::string>(addr_, "@I", intfID_.value());
  } else {
    return folly::to<std::string>(addr_);
  }
}

}}
