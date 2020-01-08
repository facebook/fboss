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

#include <folly/IPAddress.h>
#include <folly/json.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteNextHopsMulti.h"
#include "fboss/agent/rib/RouteTypes.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>
#include <memory>

namespace facebook::fboss {

template <typename AddressT>
class Route;

} // namespace facebook::fboss

namespace facebook::fboss::rib {

template <typename AddrT>
class Route {
 public:
  using Prefix = RoutePrefix<AddrT>;

  explicit Route(const Prefix& prefix);
  Route(const Prefix& prefix, ClientID clientId, RouteNextHopEntry entry)
      : prefix_(prefix) {
    if (!entry.isValid(false)) {
      throw FbossError("Invalid label forwarding action for IP route");
    }
    nexthopsmulti.update(clientId, entry);
  }

  static Route<AddrT> fromFollyDynamic(const folly::dynamic& json);

  folly::dynamic toFollyDynamic() const;

  static Route<AddrT> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  RouteDetails toRouteDetails() const;

  const Prefix& prefix() const {
    return prefix_;
  }
  bool isResolved() const {
    return (flags & RESOLVED);
  }
  bool isUnresolvable() const {
    return (flags & UNRESOLVABLE);
  }
  bool isConnected() const {
    return (flags & CONNECTED);
  }
  bool isDrop() const {
    return isResolved() && fwd.isDrop();
  }
  bool isToCPU() const {
    return isResolved() && fwd.isToCPU();
  }
  bool isProcessing() const {
    return (flags & PROCESSING);
  }
  bool needResolve() const {
    // not resolved, nor unresolvable, nor in processing
    return !(flags & (RESOLVED | UNRESOLVABLE | PROCESSING));
  }
  std::string str() const;
  // Return the forwarding info for this route
  const RouteNextHopEntry& getForwardInfo() const {
    return fwd;
  }
  const RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return nexthopsmulti.getEntryForClient(clientId);
  }
  std::pair<ClientID, const RouteNextHopEntry*> getBestEntry() const {
    return nexthopsmulti.getBestEntry();
  }
  bool hasNoEntry() const {
    return nexthopsmulti.isEmpty();
  }

  bool has(ClientID clientId, const RouteNextHopEntry& entry) const;

  bool isSame(const Route* rt) const;

  void setProcessing() {
    CHECK(!isProcessing());
    setFlagsProcessing();
  }
  void setConnected() {
    setFlagsConnected();
  }
  void setResolved(RouteNextHopEntry fwd);
  void setUnresolvable();
  void clearForward();

  void update(ClientID clientId, RouteNextHopEntry entry);

  void addEntryForClient();
  void delEntryForClient(ClientID clientId);

  bool operator==(const Route& rf) const;

 private:
  /**
   * Bit definition for RouteFields<>::flags
   *
   * CONNECTED: This route is directly connected. For example, a route based on
   *            an interface subnet is a directly connected route.
   * RESOLVED: This route has been resolved. 'RouteFields::fwd' is valid.
   * UNRESOLVABLE: This route is not resolvable. A route without RESOLVED set
   *               does not mean that it is UNRESOLVABLE.
   *               A route with neither RESOLVED nor UNRESOLVABLE just means
   *               the route needs to be resolved. As the result of process,
   *               the route could have either RESOLVED set (resolved) or
   *               UNRESOLVABLE set.
   * PROCESSING: This route is being processed to resolve the nexthop.
   *             This bit is set before look up a route to reach the nexthop.
   *             If the route used to reach the nexthop also has this bit set,
   *             that means we have a loop to resolve the route. In this case,
   *             none of the routes in the loop is resolvable.
   *             This bit is cleared when setting this route as RESOLVED or
   *             UNRESOLVABLE.
   */
  enum : uint32_t {
    CONNECTED = 0x1,
    RESOLVED = 0x2,
    UNRESOLVABLE = 0x4,
    PROCESSING = 0x8,
  };
  void setFlagsProcessing() {
    flags |= PROCESSING;
    flags &= ~(RESOLVED | UNRESOLVABLE | CONNECTED);
  }
  void setFlagsResolved() {
    flags |= RESOLVED;
    flags &= ~(UNRESOLVABLE | PROCESSING);
  }
  void setFlagsUnresolvable() {
    flags |= UNRESOLVABLE;
    flags &= ~(RESOLVED | PROCESSING | CONNECTED);
  }
  void setFlagsConnected() {
    flags |= CONNECTED;
  }
  void clearForwardInFlags() {
    flags &= ~(RESOLVED | PROCESSING | CONNECTED | UNRESOLVABLE);
  }
  uint32_t flags{0};
  RouteNextHopEntry fwd{RouteNextHopEntry::Action::DROP,
                        AdminDistance::MAX_ADMIN_DISTANCE};

  Prefix prefix_;
  /*
   * All next hops of the routes. This set could be empty if and only if
   * the route is directly connected
   */
  RouteNextHopsMulti nexthopsmulti;
};

typedef Route<folly::IPAddressV4> RouteV4;
typedef Route<folly::IPAddressV6> RouteV6;

} // namespace facebook::fboss::rib
