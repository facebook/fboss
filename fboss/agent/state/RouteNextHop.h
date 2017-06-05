/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/IPAddress.h>
#include <folly/dynamic.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class RouteNextHop {

 public:

  /**
   * createNextHop() creates a nexthop specified by client,
   * used as the nexthop of an route.
   * It can optionally be scoped via InterfaceID attribute.
   */
  static RouteNextHop createNextHop(
      folly::IPAddress addr,
      folly::Optional<InterfaceID> intfID = folly::none);

  /**
   * createForward() creates a path to reach a given nexthop, which is set
   * after the route is resolved.
   * In the case for the directly connected route, the 'addr_' is not used.
   */
  static RouteNextHop createForward(folly::IPAddress addr, InterfaceID intf) {
    return RouteNextHop(std::move(addr), intf);
  }

   /* Utility function to get thrift representation of this nexthop */
  network::thrift::BinaryAddress toThrift() const;
  static RouteNextHop fromThrift(network::thrift::BinaryAddress const& nexthop);

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteNextHop fromFollyDynamic(const folly::dynamic& nhopJson);

  const folly::IPAddress& addr() const noexcept {
    return addr_;
  }

  const folly::Optional<InterfaceID>& intfID() const noexcept {
    return intfID_;
  }

  InterfaceID intf() const {
    // could throw if intfID_ does not have value
    return intfID_.value();
  }

  std::string str() const;

 protected:
  explicit RouteNextHop(folly::IPAddress addr,
                        folly::Optional<InterfaceID> intfID = folly::none)
      : addr_(std::move(addr)), intfID_(intfID) {
  }

 private:
  folly::IPAddress addr_;
  folly::Optional<InterfaceID> intfID_;
};

/**
 * Comparision operators
 */
bool operator==(const RouteNextHop& a, const RouteNextHop& b);
bool operator< (const RouteNextHop& a, const RouteNextHop& b);

void toAppend(const RouteNextHop& nhop, std::string *result);
std::ostream& operator<<(std::ostream& os, const RouteNextHop& nhop);

}}
