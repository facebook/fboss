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
#pragma once

#include <folly/dynamic.h>
#include <folly/FBString.h>
#include "fboss/agent/types.h"
#include <folly/IPAddress.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

/**
 * RouteNextHop specified by client. It can optionally be scoped via
 * InterfaceID attribute.
 */
class RouteNextHop {
 public:
  /**
   * Create RouteNextHop. It performs validation check for interface scoping of
   * nexthop.
   */
  explicit RouteNextHop(
      folly::IPAddress addr,
      folly::Optional<InterfaceID> intfID = folly::none);

  /**
   * Utility function to get thrift representation of this nexthop or vice
   * versa.
   */
  static RouteNextHop fromThrift(network::thrift::BinaryAddress const& nexthop);
  network::thrift::BinaryAddress toThrift() const;

  const folly::IPAddress& addr() const noexcept {
    return addr_;
  }

  const folly::Optional<InterfaceID>& intfID() const noexcept {
    return intfID_;
  }

 private:
  folly::IPAddress addr_;
  folly::Optional<InterfaceID> intfID_;

};

/**
 * Comparision operators required to have flat_set<RouteNextHop>
 */
bool operator==(const RouteNextHop& a, const RouteNextHop& b);
bool operator< (const RouteNextHop& a, const RouteNextHop& b);

/**
 * Multiple RouteNextHop for ECMP route.
 */
using RouteNextHops = boost::container::flat_set<RouteNextHop>;

namespace util {

/**
 * Convert thrift representation of nexthops to RouteNextHops.
 */
RouteNextHops
toRouteNextHops(std::vector<network::thrift::BinaryAddress> const& nhAddrs);

/**
 * Convert RouteNextHops to thrift representaion of nexthops
 */
std::vector<network::thrift::BinaryAddress>
fromRouteNextHops(RouteNextHops const& nhs);

}

/**
 * Map form clientId -> RouteNextHops
 */
class RouteNextHopsMulti {
 protected:
   boost::container::flat_map<ClientID, RouteNextHops> map_;
 public:
  folly::dynamic toFollyDynamic() const;
  std::vector<ClientAndNextHops> toThrift() const;
  static RouteNextHopsMulti fromFollyDynamic(const folly::dynamic& json);
  std::string str() const;
  void update(ClientID clientid, const RouteNextHops& nhs);
  void update(ClientID clientid, const RouteNextHops&& nhs);
  bool operator==(const RouteNextHopsMulti& p2) const {
    return map_ == p2.map_;
  }
  void clear() {
    map_.clear();
  }
  bool isEmpty() const {
    // The code disallows adding/updating an empty nextHops list. So if the
    // map contains any entries, they are non-zero-length lists.
    return map_.empty();
  }

  void delNexthopsForClient(ClientID clientId);

  // Just used for testing
  bool hasNextHopsForClient(ClientID clientId) const;

  bool isSame(ClientID clientId, const RouteNextHops& nhs) const;

  const RouteNextHops& bestNextHopList() const;
};

/**
 * Route forward actions
 */
enum RouteForwardAction {
  DROP,             // Drop the packets
  TO_CPU,           // Punt the packets to the CPU
  NEXTHOPS,         // Forward the packets to one or multiple nexthops
};

std::string forwardActionStr(RouteForwardAction action);
RouteForwardAction str2ForwardAction(const std::string& action);

/**
 * Route prefix
 */
template<typename AddrT>
struct RoutePrefix {
  AddrT network;
  uint8_t mask;
  std::string str() const {
    return folly::to<std::string>(network, "/", static_cast<uint32_t>(mask));
  }

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RoutePrefix fromFollyDynamic(const folly::dynamic& prefixJson);

  bool operator<(const RoutePrefix&) const;
  bool operator>(const RoutePrefix&) const;
  bool operator==(const RoutePrefix& p2) const {
    return mask == p2.mask && network == p2.network;
  }
  bool operator!=(const RoutePrefix& p2) const {
    return !operator==(p2);
  }
  typedef AddrT AddressT;
};

typedef RoutePrefix<folly::IPAddressV4> RoutePrefixV4;
typedef RoutePrefix<folly::IPAddressV6> RoutePrefixV6;

void toAppend(const RoutePrefixV4& prefix, std::string *result);
void toAppend(const RoutePrefixV6& prefix, std::string *result);

}}
