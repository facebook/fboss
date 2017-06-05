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

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/StateUtils.h"

namespace {
constexpr auto kInterface = "interface";
constexpr auto kNexthop = "nexthop";
}

namespace facebook { namespace fboss {

RouteNextHop RouteNextHop::createNextHop(
    folly::IPAddress addr, folly::Optional<InterfaceID> intf) {
  // V4 link-local are not considered here as they can break BGPD route
  // programming in Galaxy or ExaBGP peering routes to servers as they are
  // using v4 link-local subnets on internal/downlink interfaces
  // NOTE: we do allow interface be specified with v4 link-local but we are not
  // being scrict about it
  if (not intf and addr.isV6() and addr.isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop {}", addr.str());
  }

  // Interface scoping shouldn't be specified with non link-local addresses
  if (intf and not addr.isLinkLocal()) {
    throw FbossError(
       "Interface scoping ({}) specified for non link-local nexthop {}.",
       static_cast<uint32_t>(intf.value()), addr.str());
  }

  return RouteNextHop(std::move(addr), intf);
}

network::thrift::BinaryAddress
RouteNextHop::toThrift() const {
  network::thrift::BinaryAddress nexthop = network::toBinaryAddress(addr_);
  if (intfID_) {
    nexthop.__isset.ifName = true;
    nexthop.ifName = util::createTunIntfName(intfID_.value());
  }
  return nexthop;
}

RouteNextHop
RouteNextHop::fromThrift(network::thrift::BinaryAddress const& nexthop) {
  // Get nexthop address
  const auto addr = network::toIPAddress(nexthop);

  // Get nexthop interface if specified. This can throw exception if interfaceID
  // is not properly formatted.
  folly::Optional<InterfaceID> intfID;
  if (nexthop.__isset.ifName) {
    intfID = util::getIDFromTunIntfName(nexthop.ifName);
  }

  // Calling createNextHop() so that the parameter verification is enforced.
  // Currently, no one is calling this function other than unittest.
  // If we plan to use this function for other purpose (i.e.
  // forwarding nexthop). Need to revisit this func.
  return createNextHop(std::move(addr), intfID);
}

folly::dynamic RouteNextHop::toFollyDynamic() const {
  folly::dynamic nhop = folly::dynamic::object;
  if (intfID_.hasValue()) {
    nhop[kInterface] = static_cast<uint32_t>(intfID_.value());
  }
  nhop[kNexthop] = addr_.str();
  return nhop;
}

RouteNextHop RouteNextHop::fromFollyDynamic(const folly::dynamic& nhopJson) {
  auto it = nhopJson.find(kInterface);
  folly::Optional<InterfaceID>  intf{folly::none};
  if (it != nhopJson.items().end()) {
    intf = InterfaceID(it->second.asInt());
  }
  return RouteNextHop(folly::IPAddress(nhopJson[kNexthop].stringPiece()), intf);
}

bool operator==(const RouteNextHop& a, const RouteNextHop& b) {
  return (a.intfID() == b.intfID() and a.addr() == b.addr());
}

bool operator< (const RouteNextHop& a, const RouteNextHop& b) {
  return ((a.intfID() == b.intfID())
          ? (a.addr() < b.addr())
          : (a.intfID() < b.intfID()));
}

void toAppend(const RouteNextHop& nhop, std::string *result) {
  result->append(nhop.str());
}

std::ostream& operator<<(std::ostream& os, const RouteNextHop& nhop) {
  return os << nhop.str();
}

std::string RouteNextHop::str() const {
  if (intfID_) {
    return folly::to<std::string>(addr_, "@I", intfID_.value());
  } else {
    return folly::to<std::string>(addr_);
  }
}

}}
