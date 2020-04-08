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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteNextHopsMulti.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>

namespace facebook::fboss {

/**
 * RouteFields<> Class
 *
 * Temporarily need this class when using NodeMap to store the routes
 */
template <typename AddrT>
struct RouteFields {
  enum CopyBehavior {
    COPY_PREFIX_AND_NEXTHOPS,
  };
  typedef RoutePrefix<AddrT> Prefix;
  explicit RouteFields(const Prefix& prefix);
  RouteFields(const RouteFields& rf, CopyBehavior copyBehavior);
  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}
  bool operator==(const RouteFields& rf) const;
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteFields fromFollyDynamic(const folly::dynamic& routeJson);

  RouteDetails toRouteDetails() const;

  Prefix prefix;
  // The following fields will not be copied during clone()
  /*
   * All next hops of the routes. This set could be empty if and only if
   * the route is directly connected
   */
  RouteNextHopsMulti nexthopsmulti;
  RouteNextHopEntry fwd{RouteNextHopEntry::Action::DROP,
                        AdminDistance::MAX_ADMIN_DISTANCE};
  uint32_t flags{0};
  std::optional<cfg::AclLookupClass> classID{std::nullopt};
};

/// Route<> Class
template <typename AddrT>
class Route : public NodeBaseT<Route<AddrT>, RouteFields<AddrT>> {
 public:
  typedef NodeBaseT<Route<AddrT>, RouteFields<AddrT>> RouteBase;
  typedef typename RouteFields<AddrT>::Prefix Prefix;
  typedef RouteForwardAction Action;
  using Addr = AddrT;

  // Constructor for a route
  Route(const Prefix& prefix, ClientID clientId, RouteNextHopEntry entry)
      : RouteBase(prefix) {
    if (!entry.isValid(false)) {
      throw FbossError("Invalid label forwarding action for IP route");
    }
    update(clientId, std::move(entry));
  }

  static std::shared_ptr<Route<AddrT>> fromFollyDynamic(
      const folly::dynamic& json);

  folly::dynamic toFollyDynamic() const override;

  static std::shared_ptr<Route<AddrT>> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  RouteDetails toRouteDetails() const {
    RouteDetails rd = this->getFields()->toRouteDetails();
    rd.isConnected = isConnected();
    return rd;
  }

  static void modify(std::shared_ptr<SwitchState>* state);

  const Prefix& prefix() const {
    return RouteBase::getFields()->prefix;
  }
  const Prefix& getID() const {
    return prefix();
  }
  bool isResolved() const {
    return (RouteBase::getFields()->flags & RESOLVED);
  }
  bool isUnresolvable() const {
    return (RouteBase::getFields()->flags & UNRESOLVABLE);
  }
  bool isConnected() const {
    return (RouteBase::getFields()->flags & CONNECTED);
  }
  bool isHostRoute() const {
    return prefix().mask == prefix().network.bitCount();
  }
  bool isDrop() const {
    return isResolved() && RouteBase::getFields()->fwd.isDrop();
  }
  bool isToCPU() const {
    return isResolved() && RouteBase::getFields()->fwd.isToCPU();
  }
  bool isProcessing() const {
    return (RouteBase::getFields()->flags & PROCESSING);
  }
  bool needResolve() const {
    // not resolved, nor unresolvable, nor in processing
    return !(
        RouteBase::getFields()->flags & (RESOLVED | UNRESOLVABLE | PROCESSING));
  }
  std::string str() const;
  // Return the forwarding info for this route
  const RouteNextHopEntry& getForwardInfo() const {
    return RouteBase::getFields()->fwd;
  }
  const RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return RouteBase::getFields()->nexthopsmulti.getEntryForClient(clientId);
  }
  std::pair<ClientID, const RouteNextHopEntry*> getBestEntry() const {
    return RouteBase::getFields()->nexthopsmulti.getBestEntry();
  }
  bool hasNoEntry() const {
    return RouteBase::getFields()->nexthopsmulti.isEmpty();
  }

  bool has(ClientID clientId, const RouteNextHopEntry& entry) const;

  bool isSame(const Route* rt) const;

  /*
   * The following functions modify the route object.
   * They should only be called on unpublished objects which are only visible
   * to a single thread
   */
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

  void updateClassID(std::optional<cfg::AclLookupClass> classID);

  void delEntryForClient(ClientID clientId);

  std::optional<cfg::AclLookupClass> getClassID() const {
    return RouteBase::getFields()->classID;
  }

 private:
  // no copy or assign operator
  Route(const Route&) = delete;
  Route& operator&(const Route&) = delete;
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
    auto& flags = RouteBase::writableFields()->flags;
    flags |= PROCESSING;
    flags &= ~(RESOLVED | UNRESOLVABLE | CONNECTED);
  }
  void setFlagsResolved() {
    auto& flags = RouteBase::writableFields()->flags;
    flags |= RESOLVED;
    flags &= ~(UNRESOLVABLE | PROCESSING);
  }
  void setFlagsUnresolvable() {
    auto& flags = RouteBase::writableFields()->flags;
    flags |= UNRESOLVABLE;
    flags &= ~(RESOLVED | PROCESSING | CONNECTED);
  }
  void setFlagsConnected() {
    auto& flags = RouteBase::writableFields()->flags;
    flags |= CONNECTED;
  }
  void clearForwardInFlags() {
    auto& flags = RouteBase::writableFields()->flags;
    flags &= ~(RESOLVED | PROCESSING | CONNECTED | UNRESOLVABLE);
  }

  // Inherit the constructors required for clone()
  using NodeBaseT<Route<AddrT>, RouteFields<AddrT>>::NodeBaseT;
  friend class CloneAllocator;
};

typedef Route<folly::IPAddressV4> RouteV4;
typedef Route<folly::IPAddressV6> RouteV6;

} // namespace facebook::fboss
