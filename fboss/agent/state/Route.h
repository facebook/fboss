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
  typedef RoutePrefix<AddrT> Prefix;
  explicit RouteFields(const Prefix& prefix);
  RouteFields(const Prefix& prefix, ClientID clientId, RouteNextHopEntry entry)
      : RouteFields(prefix) {
    if (!entry.isValid(false)) {
      throw FbossError("Invalid label forwarding action for IP route");
    }
    update(clientId, std::move(entry));
  }
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

  RouteDetails toRouteDetails(bool normalizedNhopWeights = false) const;
  bool isHostRoute() const {
    return prefix.mask == prefix.network.bitCount();
  }

  bool hasNoEntry() const {
    return nexthopsmulti.isEmpty();
  }
  std::pair<ClientID, const RouteNextHopEntry*> getBestEntry() const {
    return nexthopsmulti.getBestEntry();
  }
  size_t numClientEntries() const {
    return nexthopsmulti.size();
  }
  std::optional<cfg::AclLookupClass> getClassID() const {
    return classID;
  }
  void setClassID(std::optional<cfg::AclLookupClass> c) {
    classID = c;
  }
  void delEntryForClient(ClientID clientId);
  const RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return nexthopsmulti.getEntryForClient(clientId);
  }

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

 public:
  std::string str() const;
  void update(ClientID clientId, RouteNextHopEntry entry);
  void updateClassID(std::optional<cfg::AclLookupClass> c) {
    classID = c;
  }
  bool has(ClientID clientId, const RouteNextHopEntry& entry) const;

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
  void setProcessing() {
    CHECK(!isProcessing());
    setFlagsProcessing();
  }
  void setResolved(RouteNextHopEntry f) {
    fwd = std::move(f);
    setFlagsResolved();
  }
  void setUnresolvable() {
    fwd.reset();
    setFlagsUnresolvable();
  }
  void setConnected() {
    setFlagsConnected();
  }
  void clearForward() {
    fwd.reset();
    clearForwardInFlags();
  }
  Prefix prefix;
  // The following fields will not be copied during clone()
  /*
   * All next hops of the routes. This set could be empty if and only if
   * the route is directly connected
   */
  RouteNextHopsMulti nexthopsmulti;
  RouteNextHopEntry fwd{
      RouteNextHopEntry::Action::DROP,
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
      : RouteBase(prefix, clientId, std::move(entry)) {}

  static std::shared_ptr<Route<AddrT>> fromFollyDynamic(
      const folly::dynamic& json);

  folly::dynamic toFollyDynamic() const override {
    return RouteBase::getFields()->toFollyDynamic();
  }

  static std::shared_ptr<Route<AddrT>> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  RouteDetails toRouteDetails(bool normalizedNhopWeights = false) const {
    return RouteBase::getFields()->toRouteDetails(normalizedNhopWeights);
  }

  /*
   * clone and clear all forwarding info. Forwarding info will be recomputed
   * via route resolution
   */
  std::shared_ptr<Route<AddrT>> cloneForReresolve() const;

  static void modify(std::shared_ptr<SwitchState>* state);

  const Prefix& prefix() const {
    return RouteBase::getFields()->prefix;
  }
  const Prefix& getID() const {
    return prefix();
  }
  bool isResolved() const {
    return RouteBase::getFields()->isResolved();
  }
  bool isUnresolvable() const {
    return RouteBase::getFields()->isUnresolvable();
  }
  bool isConnected() const {
    return RouteBase::getFields()->isConnected();
  }
  bool isHostRoute() const {
    return RouteBase::getFields()->isHostRoute();
  }
  bool isDrop() const {
    return RouteBase::getFields()->isDrop();
  }
  bool isToCPU() const {
    return RouteBase::getFields()->isToCPU();
  }
  bool isProcessing() const {
    return RouteBase::getFields()->isProcessing();
  }
  bool needResolve() const {
    // not resolved, nor unresolvable, nor in processing
    return RouteBase::getFields()->needResolve();
  }
  std::string str() const {
    return RouteBase::getFields()->str();
  }
  // Return the forwarding info for this route
  const RouteNextHopEntry& getForwardInfo() const {
    return RouteBase::getFields()->fwd;
  }
  const RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return RouteBase::getFields()->getEntryForClient(clientId);
  }
  std::pair<ClientID, const RouteNextHopEntry*> getBestEntry() const {
    return RouteBase::getFields()->getBestEntry();
  }
  bool hasNoEntry() const {
    return RouteBase::getFields()->hasNoEntry();
  }

  size_t numClientEntries() const {
    return RouteBase::getFields()->numClientEntries();
  }

  bool has(ClientID clientId, const RouteNextHopEntry& entry) const {
    return RouteBase::getFields()->has(clientId, entry);
  }

  bool isSame(const Route* rt) const;

  /*
   * The following functions modify the route object.
   * They should only be called on unpublished objects which are only visible
   * to a single thread
   */
  void setProcessing() {
    RouteBase::writableFields()->setProcessing();
  }
  void setConnected() {
    RouteBase::writableFields()->setConnected();
  }
  void setResolved(RouteNextHopEntry fwd) {
    RouteBase::writableFields()->setResolved(std::move(fwd));
  }
  void setUnresolvable() {
    RouteBase::writableFields()->setUnresolvable();
  }
  void clearForward() {
    RouteBase::writableFields()->clearForward();
  }

  void update(ClientID clientId, RouteNextHopEntry entry) {
    RouteBase::writableFields()->update(clientId, std::move(entry));
  }

  void updateClassID(std::optional<cfg::AclLookupClass> classID) {
    RouteBase::writableFields()->updateClassID(classID);
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    return RouteBase::getFields()->getClassID();
  }
  void delEntryForClient(ClientID clientId) {
    RouteBase::writableFields()->delEntryForClient(clientId);
  }

 private:
  // no copy or assign operator
  Route(const Route&) = delete;
  Route& operator&(const Route&) = delete;

  // Inherit the constructors required for clone()
  using NodeBaseT<Route<AddrT>, RouteFields<AddrT>>::NodeBaseT;
  friend class CloneAllocator;
};

typedef Route<folly::IPAddressV4> RouteV4;
typedef Route<folly::IPAddressV6> RouteV6;

} // namespace facebook::fboss
