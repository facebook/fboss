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

#include "fboss/agent/types.h"
#include <folly/IPAddress.h>
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteForwardInfo.h"
#include "fboss/agent/state/RouteTypes.h"

#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

/**
 * RouteFields<> Class
 *
 * Temporarily need this class when using NodeMap to store the routes
 */
template<typename AddrT>
struct RouteFields {
  enum CopyBehavior {
    COPY_ALL_MEMBERS,
    COPY_ONLY_PREFIX,
  };
  typedef RoutePrefix<AddrT> Prefix;
  explicit RouteFields(const Prefix& prefix);
  RouteFields(const RouteFields& rf, CopyBehavior copyBehavior);
  template <typename Fn>
  void forEachChild(Fn fn) {}
  bool operator==(const RouteFields& rf) const;
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteFields fromFollyDynamic(const folly::dynamic& routeJson);

  Prefix prefix;
  // The following fields will not be copied during clone()
  /*
   * All next hops of the routes. This set could be empty if and only if
   * the route is directly connected
   */
  RouteNextHops nexthops;
  RouteForwardInfo fwd;
  uint32_t flags{0};
};

/// Route<> Class
template<typename AddrT>
class Route : public NodeBaseT<Route<AddrT>, RouteFields<AddrT>> {
 public:
  typedef NodeBaseT<Route<AddrT>, RouteFields<AddrT>> RouteBase;
  typedef typename RouteFields<AddrT>::Prefix Prefix;
  typedef RouteForwardAction Action;
  // Constructor for directly connected route
  Route(const Prefix& prefix, InterfaceID intf, const folly::IPAddress& addr);
  // Constructor for a route with ECMP
  Route(const Prefix& prefix, const RouteNextHops& nhs);
  Route(const Prefix& prefix, RouteNextHops&& nhs);
  // Constructor for a route with special forwarding action
  Route(const Prefix& prefix, Action action);

  ~Route() override;

  static std::shared_ptr<Route<AddrT>>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = RouteFields<AddrT>::fromFollyDynamic(json);
    return std::make_shared<Route<AddrT>>(fields);
  }

  static std::shared_ptr<Route<AddrT>>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toFollyDynamic();
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
  bool isWithNexthops() const {
    return !nexthops().empty();
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
    return !(RouteBase::getFields()->flags
             & (RESOLVED|UNRESOLVABLE|PROCESSING));
  }
  std::string str() const;
  // Return the forwarding info for this route
  const RouteForwardInfo& getForwardInfo() const {
    CHECK(isResolved());
    return RouteBase::getFields()->fwd;
  }
  const RouteNextHops& nexthops() const {
    return RouteBase::getFields()->nexthops;
  }
  bool isSame(InterfaceID intf, const folly::IPAddress& addr) const;
  bool isSame(const RouteNextHops& nhs) const;
  bool isSame(Action action) const;
  bool isSame(const Route* rt) const;
  /*
   * The following functions modify the route object.
   * They should only be called on unpublished objects which are only visible
   * to a single thread
   */
  void setFlagsProcessing() {
    CHECK(!isProcessing());
    RouteBase::writableFields()->flags |= PROCESSING;
  }
  void setResolved(RouteForwardNexthops&& fwd) {
    RouteBase::writableFields()->fwd.setNexthops(std::move(fwd));
    setFlagsResolved();
  }
  void setResolved(const RouteForwardNexthops& fwd) {
    RouteBase::writableFields()->fwd.setNexthops(fwd);
    setFlagsResolved();
  }
  void setResolved(Action action) {
    RouteBase::writableFields()->fwd.setAction(action);
    setFlagsResolved();
  }
  void clearFlags() {
    auto& flags = RouteBase::writableFields()->flags;
    flags = 0x0;
  }

  bool flagsCleared() const {
    return RouteBase::getFields()->flags == 0;
  }
  void setUnresolvable();
  void update(InterfaceID intf, const folly::IPAddress& addr);
  void update(const RouteNextHops& nhs);
  void update(RouteNextHops&& nhs);
  void update(Action action);
 private:
  // no copy or assign operator
  Route(const Route &) = delete;
  Route& operator&(const Route &) = delete;
  void updateNexthopCommon(const RouteNextHops& nhs);
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
  void setFlagsResolved() {
    auto& flags = RouteBase::writableFields()->flags;
    flags |= RESOLVED;
    flags &= ~(UNRESOLVABLE|PROCESSING);
  }
  void setFlagsUnresolvable() {
    auto& flags = RouteBase::writableFields()->flags;
    flags |= UNRESOLVABLE;
    flags &= ~(RESOLVED|PROCESSING);
  }
  void setFlagsConnected() {
    RouteBase::writableFields()->flags = CONNECTED;
    setFlagsResolved();
  }
  // Inherit the constructors required for clone()
  using NodeBaseT<Route<AddrT>, RouteFields<AddrT>>::NodeBaseT;
  friend class CloneAllocator;
};

typedef Route<folly::IPAddressV4> RouteV4;
typedef Route<folly::IPAddressV6> RouteV6;

}}
