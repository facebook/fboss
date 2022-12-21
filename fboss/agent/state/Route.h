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

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteNextHopsMulti.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>

namespace facebook::fboss {

template <typename AddrT>
struct RouteFields;

template <typename AddrT>
using PrefixT = std::
    conditional_t<std::is_same_v<AddrT, LabelID>, Label, RoutePrefix<AddrT>>;

template <typename AddrT>
using ThriftFieldsT = std::conditional_t<
    std::is_same_v<AddrT, LabelID>,
    state::LabelForwardingEntryFields,
    state::RouteFields>;

template <typename AddrT>
struct RouteTraits {
  using AddrType = AddrT;
  using PrefixType = PrefixT<AddrT>;
  using ThriftFieldsType = ThriftFieldsT<AddrT>;
  using RouteFieldsType = RouteFields<AddrT>;
};

/**
 * RouteFields<> Class
 *
 * Temporarily need this class when using NodeMap to store the routes
 */
template <typename AddrT>
struct RouteFields
    : public ThriftyFields<RouteFields<AddrT>, ThriftFieldsT<AddrT>> {
  using Prefix = PrefixT<AddrT>;
  using ThriftFields = ThriftFieldsT<AddrT>;

  explicit RouteFields(const Prefix& prefix);
  RouteFields(
      const Prefix& prefix,
      ClientID clientId,
      const RouteNextHopEntry& entry)
      : RouteFields(prefix) {
    if (!entry.isValid(std::is_same_v<LabelID, AddrT>)) {
      throw FbossError("Invalid label forwarding action for IP route");
    }
    update(clientId, entry);
  }
  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}
  bool operator==(const RouteFields& rf) const;
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamicLegacy() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteFields fromFollyDynamicLegacy(const folly::dynamic& routeJson);

  RouteDetails toRouteDetails(bool normalizedNhopWeights = false) const;
  bool isHostRoute() const {
    if constexpr (
        std::is_same_v<folly::IPAddressV6, AddrT> ||
        std::is_same_v<folly::IPAddressV4, AddrT>) {
      return prefix().mask() == prefix().network().bitCount();
    } else {
      return false;
    }
  }

  bool hasNoEntry() const {
    return nexthopsmulti().isEmpty();
  }
  std::pair<ClientID, const state::RouteNextHopEntry*> getBestEntry() const {
    return RouteNextHopsMulti::getBestEntry(*(this->data().nexthopsmulti()));
  }
  size_t numClientEntries() const {
    return nexthopsmulti().size();
  }
  std::optional<cfg::AclLookupClass> getClassID() const {
    return classID();
  }
  void setClassID(std::optional<cfg::AclLookupClass> c) {
    if (c) {
      this->writableData().classID() = c.value();
    } else {
      this->writableData().classID().reset();
    }
  }
  void delEntryForClient(ClientID clientId);
  const state::RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return RouteNextHopsMulti::getEntryForClient(
        clientId, *(this->data().nexthopsmulti()));
  }

  RouteNextHopsMulti getEntryForClients() const {
    return nexthopsmulti();
  }

  ThriftFields toThrift() const override {
    return this->data();
  }

  static RouteFields fromThrift(ThriftFields const& fields) {
    return RouteFields(fields);
  }

  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);

  void clearFlags() {
    this->writableData().flags() = 0;
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
    setFlags(flags() | PROCESSING);
    setFlags(flags() & (~(RESOLVED | UNRESOLVABLE | CONNECTED)));
  }
  void setFlagsResolved() {
    setFlags(flags() | RESOLVED);
    setFlags(flags() & (~(UNRESOLVABLE | PROCESSING)));
  }
  void setFlagsUnresolvable() {
    setFlags(flags() | UNRESOLVABLE);
    setFlags(flags() & (~(RESOLVED | PROCESSING | CONNECTED)));
  }
  void setFlagsConnected() {
    setFlags(flags() | CONNECTED);
  }
  void clearForwardInFlags() {
    setFlags(flags() & (~(RESOLVED | PROCESSING | CONNECTED | UNRESOLVABLE)));
  }
  void setFlags(uint32_t flags) {
    this->writableData().flags() = flags;
  }

  // private constructor for thrift to fields
  RouteFields(
      const Prefix& argsPrefix,
      const RouteNextHopsMulti& argsNexthopsmulti,
      const RouteNextHopEntry& argsFwd,
      uint32_t argsFlags,
      std::optional<cfg::AclLookupClass> argsClassID)
      : RouteFields(getRouteFields(
            argsPrefix,
            argsNexthopsmulti,
            argsFwd,
            argsFlags,
            argsClassID)) {}

  explicit RouteFields(const ThriftFields& fields) {
    this->writableData() = fields;
  }

  static ThriftFields getRouteFields(
      const PrefixT<AddrT>& prefix,
      const RouteNextHopsMulti& multi =
          RouteNextHopsMulti::fromThrift(state::RouteNextHopsMulti{}),
      const RouteNextHopEntry& fwd =
          RouteNextHopEntry{
              RouteNextHopEntry::Action::DROP,
              AdminDistance::MAX_ADMIN_DISTANCE},
      uint32_t flags = 0,
      const std::optional<cfg::AclLookupClass>& classID = std::nullopt);

 public:
  std::string strLegacy() const;
  void update(ClientID clientId, const RouteNextHopEntry& entry);
  void updateClassID(std::optional<cfg::AclLookupClass> c) {
    setClassID(c);
  }
  bool has(ClientID clientId, const RouteNextHopEntry& entry) const;

  bool isResolved() const {
    return (flags() & RESOLVED);
  }
  bool isUnresolvable() const {
    return (flags() & UNRESOLVABLE);
  }
  bool isConnected() const {
    return (flags() & CONNECTED);
  }
  bool isDrop() const {
    return isResolved() &&
        RouteNextHopEntry::isAction(
               *(this->data().fwd()), RouteForwardAction::DROP);
  }
  bool isToCPU() const {
    return isResolved() &&
        RouteNextHopEntry::isAction(
               *(this->data().fwd()), RouteForwardAction::TO_CPU);
  }
  bool isProcessing() const {
    return (flags() & PROCESSING);
  }
  bool needResolve() const {
    // not resolved, nor unresolvable, nor in processing
    return !(flags() & (RESOLVED | UNRESOLVABLE | PROCESSING));
  }
  void setProcessing() {
    CHECK(!isProcessing());
    setFlagsProcessing();
  }
  void setResolved(const RouteNextHopEntry& f) {
    this->writableData().fwd() = f.toThrift();
    setFlagsResolved();
  }
  void setUnresolvable() {
    this->writableData().fwd() = state::RouteNextHopEntry{};
    setFlagsUnresolvable();
  }
  void setConnected() {
    setFlagsConnected();
  }
  void clearForward() {
    this->writableData().fwd() = state::RouteNextHopEntry{};
    clearForwardInFlags();
  }

  Prefix prefix() const {
    if constexpr (std::is_same_v<AddrT, LabelID>) {
      return Prefix::fromThrift(*(this->data().label()));
    } else {
      return Prefix::fromThrift(*(this->data().prefix()));
    }
  }
  RouteNextHopsMulti nexthopsmulti() const {
    return RouteNextHopsMulti::fromThrift(*(this->data().nexthopsmulti()));
  }
  RouteNextHopEntry fwd() const {
    return RouteNextHopEntry(*(this->data().fwd()));
  }
  uint32_t flags() const {
    return *(this->data().flags());
  }
  std::optional<cfg::AclLookupClass> classID() const {
    if (auto classID = this->data().classID()) {
      return *classID;
    }
    return std::nullopt;
  }
};

/// Route<> Class
template <typename AddrT>
class Route : public ThriftyBaseT<
                  ThriftFieldsT<AddrT>,
                  Route<AddrT>,
                  RouteFields<AddrT>> {
 public:
  using RouteBase = ThriftyBaseT<
      typename RouteTraits<AddrT>::ThriftFieldsType,
      Route<AddrT>,
      RouteFields<AddrT>>;
  using Prefix = typename RouteFields<AddrT>::Prefix;
  using Action = RouteForwardAction;
  using Addr = AddrT;

  // Constructor for a route
  Route(const Prefix& prefix, ClientID clientId, const RouteNextHopEntry& entry)
      : RouteBase(prefix, clientId, entry) {}

  static std::shared_ptr<Route<AddrT>> fromFollyDynamicLegacy(
      const folly::dynamic& json);

  folly::dynamic toFollyDynamicLegacy() const {
    return RouteBase::getFields()->toFollyDynamicLegacy();
  }

  static std::shared_ptr<Route<AddrT>> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamicLegacy(folly::parseJson(jsonStr));
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

  Prefix prefix() const {
    return RouteBase::getFields()->prefix();
  }
  Prefix getID() const {
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
  RouteNextHopEntry getForwardInfo() const {
    return RouteBase::getFields()->fwd();
  }
  const state::RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return RouteBase::getFields()->getEntryForClient(clientId);
  }
  std::pair<ClientID, const state::RouteNextHopEntry*> getBestEntry() const {
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
  void setResolved(const RouteNextHopEntry& fwd) {
    RouteBase::writableFields()->setResolved(fwd);
  }
  void setUnresolvable() {
    RouteBase::writableFields()->setUnresolvable();
  }
  void clearForward() {
    RouteBase::writableFields()->clearForward();
  }

  void update(ClientID clientId, const RouteNextHopEntry& entry) {
    RouteBase::writableFields()->update(clientId, entry);
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

  RouteNextHopsMulti getEntryForClients() const {
    return RouteBase::getFields()->getEntryForClients();
  }

  bool isPopAndLookup() const {
    const auto nexthops = RouteBase::getFields()->fwd().getNextHopSet();
    if (nexthops.size() == 1) {
      // there must be exactly one next hop for POP_AND_LOOKUP action
      return nexthops.begin()->isPopAndLookup();
    }
    return false;
  }

 private:
  // no copy or assign operator
  Route(const Route&) = delete;
  Route& operator=(const Route&) = delete;

  // Inherit the constructors required for clone()
  using ThriftyBaseT<ThriftFieldsT<AddrT>, Route<AddrT>, RouteFields<AddrT>>::
      ThriftyBaseT;
  friend class CloneAllocator;
};

using RouteV4 = Route<folly::IPAddressV4>;
using RouteV6 = Route<folly::IPAddressV6>;
using RouteMpls = Route<LabelID>;

} // namespace facebook::fboss
