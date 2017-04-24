/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteTypes.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/StateUtils.h"

namespace {
constexpr auto kAddress = "address";
constexpr auto kMask = "mask";
constexpr auto kDrop = "Drop";
constexpr auto kToCpu = "ToCPU";
constexpr auto kNexthops = "Nexthops";
}

namespace facebook { namespace fboss {

using std::string;

std::string forwardActionStr(RouteForwardAction action) {
  switch (action) {
  case DROP: return kDrop;
  case TO_CPU: return kToCpu;
  case NEXTHOPS: return kNexthops;
  }
  CHECK(0);
}

RouteForwardAction str2ForwardAction(const std::string& action) {
  if (action == kDrop) {
    return DROP;
  } else if (action == kToCpu) {
    return TO_CPU;
  } else {
    CHECK(action == kNexthops);
    return NEXTHOPS;
  }
}

//
// RouteNextHop Class
//

RouteNextHop::RouteNextHop(
    folly::IPAddress addr,
    folly::Optional<InterfaceID> intfID)
    : addr_(std::move(addr)), intfID_(std::move(intfID)) {

  // V4 link-local are not considered here as they can break BGPD route
  // programming in Galaxy or ExaBGP peering routes to servers as they are
  // using v4 link-local subnets on internal/downlink interfaces
  // NOTE: we do allow interface be specified with v4 link-local but we are not
  // being scrict about it
  if (!intfID_ and addr_.isV6() and addr_.isLinkLocal()) {
    throw FbossError(
        "Missing interface scoping for link-local nexthop {}", addr_.str());
  }

  // Interface scoping shouldn't be specified with non link-local addresses
  if (intfID_ and !addr_.isLinkLocal()) {
    throw FbossError(
       "Interface scoping ({}) specified for non link-local nexthop {}.",
       static_cast<uint32_t>(intfID_.value()), addr_.str());
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
    intfID = getIDFromTunIntfName(nexthop.ifName);
  }

  return RouteNextHop(std::move(addr), std::move(intfID));
}

network::thrift::BinaryAddress
RouteNextHop::toThrift() const {
  network::thrift::BinaryAddress nexthop = network::toBinaryAddress(addr_);
  if (intfID_) {
    nexthop.__isset.ifName = true;
    nexthop.ifName = createTunIntfName(intfID_.value());
  }
  return nexthop;
}

bool operator==(const RouteNextHop& a, const RouteNextHop& b) {
  return (a.addr() == b.addr()) && (a.intfID() == b.intfID());
}

bool operator< (const RouteNextHop& a, const RouteNextHop& b) {
  if (a.addr() == b.addr()) {
    return a.intfID() < b.intfID();
  }
  return a.addr() < b.addr();
}

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
// RoutePrefix<> Class
//

template<typename AddrT>
bool RoutePrefix<AddrT>::operator<(const RoutePrefix& p2) const {
  if (mask < p2.mask) {
    return true;
  } else if (mask > p2.mask) {
    return false;
  }
  return network < p2.network;
}

template<typename AddrT>
bool RoutePrefix<AddrT>::operator>(const RoutePrefix& p2) const {
  if (mask > p2.mask) {
    return true;
  } else if (mask < p2.mask) {
    return false;
  }
  return network > p2.network;
}

template<typename AddrT>
folly::dynamic RoutePrefix<AddrT>::toFollyDynamic() const {
  folly::dynamic pfx = folly::dynamic::object;
  pfx[kAddress] = network.str();
  pfx[kMask] = mask;
  return pfx;
}

template <typename AddrT>
RoutePrefix<AddrT>
RoutePrefix<AddrT>::fromFollyDynamic(const folly::dynamic& pfxJson) {
  RoutePrefix pfx;
  pfx.network = AddrT(pfxJson[kAddress].stringPiece());
  pfx.mask = pfxJson[kMask].asInt();
  return pfx;
}

void toAppend(const RoutePrefixV4& prefix, std::string *result) {
  result->append(prefix.str());
}

void toAppend(const RoutePrefixV6& prefix, std::string *result) {
  result->append(prefix.str());
}

folly::dynamic RouteNextHopsMulti::toFollyDynamic() const {
  // Store the clientid->nextHops map as a dynamic::object
  folly::dynamic obj = folly::dynamic::object();
  for (auto const& row : map_) {
    int clientid = row.first;
    RouteNextHops const& nxtHps = row.second;

    folly::dynamic nxtHopCopy = folly::dynamic::array;
    for (const auto& nhop: nxtHps) {
      nxtHopCopy.push_back(nhop.addr().str());
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
    for (const auto& nh : srcPair.second) {
      destPair.nextHopAddrs.push_back(network::toBinaryAddress(nh.addr()));
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
    for (const auto& ip : pair.second) {
      // test
      std::string theIP = ip.asString();
      list.emplace(RouteNextHop(folly::IPAddress(ip.asString())));
    }
    nh.update(ClientID(clientId), std::move(list));
  }
  return nh;
}

std::string RouteNextHopsMulti::str() const {
  std::string ret = "";
  for (auto const & row : map_) {
    int clientid = row.first;
    RouteNextHops const& nxtHps = row.second;

    ret.append(folly::to<string>("(client#", clientid, ": "));
    for (const auto& nh : nxtHps) {
      ret.append(folly::to<string>(nh.addr(), ", "));
    }
    ret.append(")");
  }
  return ret;
}

void RouteNextHopsMulti::update(ClientID clientId, const RouteNextHops& nhs) {
  map_[clientId] = nhs;
}

void RouteNextHopsMulti::update(ClientID clientId, const RouteNextHops&& nhs) {
  map_[clientId] = std::move(nhs);
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
  return it->second;
}

bool RouteNextHopsMulti::hasNextHopsForClient(ClientID clientId) const {
  return map_.find(clientId) != map_.end();
}

bool RouteNextHopsMulti::isSame(ClientID id, const RouteNextHops& nhs) const {
  auto iter = map_.find(id);
  if (iter == map_.end()) {
      return false;
  }
  return nhs == iter->second;
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
    return map_.begin()->second;
  } else {
    // Throw an exception if the map is empty.  That seems
    // like the right behavior.
    throw FbossError("bestNextHopList() called on a Route with no nexthops");
  }
}


template class RoutePrefix<folly::IPAddressV4>;
template class RoutePrefix<folly::IPAddressV6>;

}}
