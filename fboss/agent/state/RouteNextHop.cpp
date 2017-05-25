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
constexpr auto kNexthopDelim = "@";
}

namespace facebook { namespace fboss {

namespace util {

RouteNextHops
toRouteNextHops(std::vector<network::thrift::BinaryAddress> const& nhAddrs) {
  RouteNextHops nhs;
  nhs.reserve(nhAddrs.size());
  for (auto const& nhAddr : nhAddrs) {
    nhs.emplace(RouteNextHop::fromThrift(nhAddr));
  }
  return nhs;
}

std::vector<network::thrift::BinaryAddress>
fromRouteNextHops(RouteNextHops const& nhs) {
  std::vector<network::thrift::BinaryAddress> nhAddrs;
  nhAddrs.reserve(nhs.size());
  for (auto const& nh : nhs) {
    nhAddrs.emplace_back(nh.toThrift());
  }
  return nhAddrs;
}

} // namespace util

//
// RouteNextHop Class
//

RouteNextHop::RouteNextHop(folly::IPAddress addrV,
                           folly::Optional<InterfaceID> intfIDV)
    : RouteNextHopBase(std::move(addrV), std::move(intfIDV)) {

  // V4 link-local are not considered here as they can break BGPD route
  // programming in Galaxy or ExaBGP peering routes to servers as they are
  // using v4 link-local subnets on internal/downlink interfaces
  // NOTE: we do allow interface be specified with v4 link-local but we are not
  // being scrict about it
  if (!intfID() and addr().isV6() and addr().isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop {}", addr().str());
  }

  // Interface scoping shouldn't be specified with non link-local addresses
  if (intfID() and !addr().isLinkLocal()) {
    throw FbossError(
       "Interface scoping ({}) specified for non link-local nexthop {}.",
       static_cast<uint32_t>(intfID().value()), addr().str());
  }
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

  return RouteNextHop(std::move(addr), std::move(intfID));
}

folly::dynamic RouteNextHopsMulti::toFollyDynamic() const {
  // Store the clientid->nextHops map as a dynamic::object
  folly::dynamic obj = folly::dynamic::object();
  for (auto const& row : map_) {
    int clientid = row.first;
    RouteNextHops const& nxtHps = row.second.getNextHopSet();

    folly::dynamic nxtHopCopy = folly::dynamic::array;
    for (const auto& nhop: nxtHps) {
      std::string intfID = "";
      if (nhop.intfID().hasValue()) {
        intfID = folly::sformat(
            "{}{}", kNexthopDelim,
            static_cast<uint32_t>(nhop.intfID().value()));
      }
      nxtHopCopy.push_back(folly::sformat("{}{}", nhop.addr().str(), intfID));
    }
    obj[folly::to<std::string>(clientid)] = nxtHopCopy;
  }
  return obj;
}

std::vector<ClientAndNextHops> RouteNextHopsMulti::toThrift() const {
  std::vector<ClientAndNextHops> list;
  for (const auto& srcPair : map_) {
    ClientAndNextHops destPair;
    destPair.clientId = srcPair.first;
    for (const auto& nh : srcPair.second.getNextHopSet()) {
      destPair.nextHopAddrs.push_back(network::toBinaryAddress(nh.addr()));
      if (nh.intfID().hasValue()) {
        auto& nhAddr = destPair.nextHopAddrs.back();
        nhAddr.__isset.ifName = true;
        nhAddr.ifName = util::createTunIntfName(nh.intfID().value());
      }
    }
    list.push_back(destPair);
  }
  return list;
}

RouteNextHopsMulti
RouteNextHopsMulti::fromFollyDynamic(const folly::dynamic& json) {
  RouteNextHopsMulti nh;
  for (const auto& pair: json.items()) {
    int clientId = pair.first.asInt();
    RouteNextHops list;
    for (const auto& ipnh : pair.second) {
      std::vector<std::string> parts;
      folly::split(kNexthopDelim, ipnh.asString(), parts);
      CHECK(0 < parts.size() && parts.size() < 3);

      folly::Optional<InterfaceID> intfID;
      if (parts.size() == 2) {
        intfID = InterfaceID(folly::to<uint32_t>(parts.at(1)));
      }

      list.emplace(folly::IPAddress(parts[0]), intfID);
    }
    nh.update(ClientID(clientId), RouteNextHopEntry(std::move(list)));
  }
  return nh;
}

std::string RouteNextHopsMulti::str() const {
  std::string ret = "";
  for (auto const & row : map_) {
    int clientid = row.first;
    RouteNextHops const& nxtHps = row.second.getNextHopSet();

    ret.append(folly::to<std::string>("(client#", clientid, ": "));
    for (const auto& nh : nxtHps) {
      ret.append(folly::to<std::string>(nh.addr(), ", "));
    }
    ret.append(")");
  }
  return ret;
}

void RouteNextHopsMulti::update(ClientID clientId, RouteNextHopEntry nhe) {
  map_[clientId] = std::move(nhe);
}

void RouteNextHopsMulti::delNexthopsForClient(ClientID clientId) {
  map_.erase(clientId);
}

folly::Optional<RouteNextHops>
RouteNextHopsMulti::getNexthopsForClient(ClientID clientId) const {
  auto it = map_.find(clientId);
  if (it == map_.end()) {
    return folly::none;
  }
  return it->second.getNextHopSet();
}

bool RouteNextHopsMulti::hasNextHopsForClient(ClientID clientId) const {
  return map_.find(clientId) != map_.end();
}

bool RouteNextHopsMulti::isSame(ClientID id, const RouteNextHops& nhs) const {
  auto iter = map_.find(id);
  if (iter == map_.end()) {
      return false;
  }
  return nhs == iter->second.getNextHopSet();
}

const RouteNextHops&
RouteNextHopsMulti::bestNextHopList() const {
  // I still need to implement a scheme where each clientId
  // can be assigned a priority.  But for now, we're just saying
  // the clientId == its priority.
  //
  // Since flat_map stores items in key-sorted order, the smallest key
  // (and hence the highest-priority client) is first.
  if (map_.size() > 0) {
    return map_.begin()->second.getNextHopSet();
  } else {
    // Throw an exception if the map is empty.  That seems
    // like the right behavior.
    throw FbossError("bestNextHopList() called on a Route with no nexthops");
  }
}

}}
