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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/FbossEventBase.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <folly/Synchronized.h>

#include <functional>
#include <memory>
#include <thread>
#include <vector>

DECLARE_bool(mpls_rib);

namespace facebook::fboss {
class SwitchState;
class MultiSwitchForwardingInformationBaseMap;
class MultiSwitchFibInfoMap;
class MultiSwitchMySidMap;
class SwitchIdScopeResolver;
class StateDelta;

template <typename AddressT, typename FibType>
void reconstructRibFromFib(
    const std::shared_ptr<FibType>& fib,
    NetworkToRouteMap<AddressT>* addrToRoute);

void reconstructMySidTableFromSwitchState(
    const std::shared_ptr<MultiSwitchMySidMap>& mySidMap,
    MySidTable* mySidTable);

using RibToSwitchStateFunction = std::function<StateDelta(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    const NextHopIDManager* nextHopIDManager,
    const MySidTable& mySidTable,
    void* cookie)>;

using RibMySidToSwitchStateFunction = std::function<StateDelta(
    const SwitchIdScopeResolver* resolver,
    const NextHopIDManager* nextHopIDManager,
    const MySidTable& mySidTable,
    void* cookie)>;

// Each pair is (MySid state object, its unresolved next-hop set). The
// next-hop set is empty for DECAPSULATE_AND_LOOKUP / ADJACENCY_MICRO_SID
// entries; populated for NODE_MICRO_SID. Bundling them in a pair keeps the
// two pieces of data inseparable across all RIB / ConfigApplier call sites
// that consume config-derived MySid state.
using MySidWithNextHops = std::pair<std::shared_ptr<MySid>, RouteNextHopSet>;

/*
 * RibRouteTables provides a thread safe abstraction for maintaining Rib data
 * structures and programming them down to the FIB. Its designed to abstract
 * away granular locking logic over RIB data structures to allow for fast
 * lookups that are not encumbered by long HW write cycles
 */
class RibRouteTables {
 public:
  RibRouteTables() = default;

  // Returns a deep copy of the NextHopIDManager. This is an expensive
  // operation and is intended only for tests.
  std::unique_ptr<NextHopIDManager> getNextHopIDManagerCopy() const {
    return synchronizedRouteTables_.withRLock(
        [&](const auto& routeTables) -> std::unique_ptr<NextHopIDManager> {
          if (routeTables.nextHopIDManager) {
            return std::make_unique<NextHopIDManager>(
                *routeTables.nextHopIDManager);
          }
          return nullptr;
        });
  }

  void addOrUpdateNamedNextHopGroups(
      const std::vector<std::pair<std::string, RouteNextHopSet>>& groups,
      const std::function<void(const NextHopIDManager*)>& stateUpdateFn);

  void deleteNamedNextHopGroups(
      const std::vector<std::string>& names,
      const std::function<void(const NextHopIDManager*)>& stateUpdateFn);

  template <typename RouteType, typename RouteIdType>
  void update(
      const SwitchIdScopeResolver* resolver,
      RouterID routerID,
      ClientID clientID,
      AdminDistance adminDistanceFromClientID,
      const std::vector<RouteType>& toAddRoutes,
      const std::vector<RouteIdType>& toDelPrefixes,
      bool resetClientsRoutes,
      folly::StringPiece updateType,
      const RibToSwitchStateFunction& ribToSwitchStateFunc,
      void* cookie);

  void update(
      const SwitchIdScopeResolver* resolver,
      const std::vector<MySidEntry>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      const RibMySidToSwitchStateFunction& ribMySidToSwitchStateFunc,
      void* cookie);

  void update(
      const SwitchIdScopeResolver* resolver,
      const std::vector<MySidWithNextHops>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      const RibMySidToSwitchStateFunction& ribMySidToSwitchStateFunc,
      void* cookie);

  void setClassID(
      const SwitchIdScopeResolver* resolver,
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      std::optional<cfg::AclLookupClass> classId,
      void* cookie);
  void setOverrideEcmpMode(
      RouterID rid,
      const std::unordered_map<
          folly::CIDRNetwork,
          std::optional<cfg::SwitchingMode>>& prefix2EcmpMode);
  void setOverrideEcmpNhops(
      RouterID rid,
      const std::unordered_map<
          folly::CIDRNetwork,
          std::optional<RouteNextHopSet>>& prefix2Nhops);
  /*
   * VrfAndNetworkToInterfaceRoute is conceptually a mapping from the pair
   * (RouterID, folly::CIDRNetwork) to the pair (Interface(1),
   * folly::IPAddress). An example of an element in this map is: (RouterID(0),
   * 169.254.0.0/16) --> (Interface(1), 169.254.0.1) This specifies that the
   * network 169.254.0.0/16 in VRF 0 can be reached via Interface 1, which has
   * an address of 169.254.0.1. in that subnet. Note that the IP address in the
   * key has its mask applied to it while the IP address value doesn't.
   */
  using PrefixToInterfaceIDAndIP = boost::container::
      flat_map<folly::CIDRNetwork, std::pair<InterfaceID, folly::IPAddress>>;
  using RouterIDAndNetworkToInterfaceRoutes =
      boost::container::flat_map<RouterID, PrefixToInterfaceIDAndIP>;

  void reconfigure(
      const SwitchIdScopeResolver* resolver,
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes,
      const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu,
      const std::vector<cfg::StaticIp2MplsRoute>& staticIp2MplsRoutes,
      const std::vector<cfg::StaticMplsRouteWithNextHops>&
          staticMplsRoutesWithNextHops,
      const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
      const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCpu,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      void* cookie);

  void updateRemoteInterfaceRoutes(
      const SwitchIdScopeResolver* resolver,
      const RouterIDAndNetworkToInterfaceRoutes& toAdd,
      const boost::container::flat_map<
          facebook::fboss::RouterID,
          std::vector<folly::CIDRNetwork>>& toDel,
      const RibToSwitchStateFunction& ribToSwitchStateFunc,
      void* cookie);

  /*
   * FIB assisted fromThrift. With shared data structure of routes
   * all except the unresolved routes are shared b/w rib and FIB, so
   * we can simply reconstruct RIB by ser/deser unresolved routes
   * and importing FIB
   */
  static RibRouteTables fromThrift(
      const std::map<int32_t, state::RouteTableFields>& ribThrift,
      const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap,
      const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFib,
      const std::shared_ptr<MultiSwitchMySidMap>& mySidMap);

  void ensureVrf(RouterID rid);
  std::vector<RouterID> getVrfList() const;
  std::vector<RouteDetails> getRouteTableDetails(RouterID rid) const;
  std::vector<MplsRouteDetails> getMplsRouteTableDetails() const;

  // Returns a copy of the MySidTable as thrift fields.
  // This is expensive and should only be used in tests.
  std::unordered_map<folly::CIDRNetworkV6, state::MySidFields>
  getMySidTableCopy() const;

  template <typename AddressT>
  std::shared_ptr<Route<AddressT>> longestMatch(
      const AddressT& address,
      RouterID vrf) const;

  // Atomically perform longestMatch + NextHopSetID resolution under a single
  // rlock on synchronizedRouteTables_. Returns nullopt if no route found.
  // Otherwise returns {route, resolved nexthops}.
  // When normalized=true (default), returns weight-normalized nexthops.
  // When normalized=false, returns raw resolved nexthops.
  template <typename AddressT>
  std::optional<std::pair<std::shared_ptr<Route<AddressT>>, RouteNextHopSet>>
  getRouteAndNextHops(
      const AddressT& address,
      RouterID vrf,
      bool normalized = true) const;

  std::map<int32_t, state::RouteTableFields> toThrift() const;
  static RibRouteTables fromThrift(
      const std::map<int32_t, state::RouteTableFields>&);
  std::map<int32_t, state::RouteTableFields> warmBootState() const;

  void updateEcmpOverrides(const StateDelta& delta);

 private:
  struct VrfRouteTable {
    IPv4NetworkToRouteMap v4NetworkToRoute;
    IPv6NetworkToRouteMap v6NetworkToRoute;
    LabelToRouteMap labelToRoute;

    bool operator==(const VrfRouteTable& other) const {
      return v4NetworkToRoute == other.v4NetworkToRoute &&
          v6NetworkToRoute == other.v6NetworkToRoute;
    }
    bool operator!=(const VrfRouteTable& other) const {
      return !(*this == other);
    }
    std::shared_ptr<Route<folly::IPAddressV4>> longestMatch(
        const folly::IPAddressV4& addr) const {
      auto it = v4NetworkToRoute.longestMatch(addr, addr.bitCount());
      return it == v4NetworkToRoute.end() ? nullptr : it->value();
    }
    std::shared_ptr<Route<folly::IPAddressV6>> longestMatch(
        const folly::IPAddressV6& addr) const {
      auto it = v6NetworkToRoute.longestMatch(addr, addr.bitCount());
      return it == v6NetworkToRoute.end() ? nullptr : it->value();
    }
    state::RouteTableFields toThrift() const;
    static VrfRouteTable fromThrift(const state::RouteTableFields&);
    state::RouteTableFields warmBootState() const;
  };

  void updateFib(
      const SwitchIdScopeResolver* resolver,
      RouterID vrf,
      const RibToSwitchStateFunction& ribToSwitchStateFunc,
      void* cookie);
  template <typename RibUpdateFn>
  void updateRib(RouterID vrf, const RibUpdateFn& updateRib);
  void updateEcmpOverrides(RouterID vrf, const StateDelta& delta);

  /*
   * MySid updates
   */
  void updateFib(
      const SwitchIdScopeResolver* resolver,
      const RibMySidToSwitchStateFunction& ribMySidToSwitchStateFunc,
      void* cookie);
  template <typename RibUpdateFn>
  void updateRibMySids(const RibUpdateFn& updateRibFn);

  /*
   * Named next-hop group updates
   */
  template <typename RibUpdateFn>
  void updateRibNamedNextHopGroups(const RibUpdateFn& updateRibFn);
  void updateFibNamedNextHopGroups(
      const std::function<void(const NextHopIDManager*)>& stateUpdateFn);

  /*
   * Currently, route updates to separate VRFs are made to be sequential. In the
   * event FBOSS has to operate in a routing architecture with numerous VRFs,
   * we can avoid a slow down by a factor of number of VRFs by parallelizing
   * route updates across VRFs. This can be accomplished simply by associating
   * the mutex implicit in folly::Synchronized with an individual RouteTable.
   */
  using RouterIDToRouteTable =
      boost::container::flat_map<RouterID, VrfRouteTable>;

  struct RouteTables {
    RouterIDToRouteTable routerIDToRouteTable;
    std::unordered_map<folly::CIDRNetworkV6, std::shared_ptr<MySid>> mySidTable;
    std::unique_ptr<NextHopIDManager> nextHopIDManager{
        FLAGS_enable_nexthop_id_manager ? std::make_unique<NextHopIDManager>()
                                        : nullptr};
  };

  using SynchronizedRouteTables = folly::Synchronized<RouteTables>;

  void importFibs(
      const SynchronizedRouteTables::WLockedPtr& lockedRouteTables,
      const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap,
      const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFibs);

  // Shared implementation for both MySid update() overloads.
  void updateMySidsImpl(
      const SwitchIdScopeResolver* resolver,
      const std::vector<MySidWithNextHops>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      const RibMySidToSwitchStateFunction& ribMySidToSwitchStateFunc,
      void* cookie);

  RouterIDToRouteTable constructRouteTables(
      const SynchronizedRouteTables::WLockedPtr& lockedRouteTables,
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes) const;

  SynchronizedRouteTables synchronizedRouteTables_;
};

class RoutingInformationBase {
 public:
  RoutingInformationBase(const RoutingInformationBase& o) = delete;
  RoutingInformationBase& operator=(const RoutingInformationBase& o) = delete;
  RoutingInformationBase(RoutingInformationBase&&) = delete;
  RoutingInformationBase& operator=(RoutingInformationBase&&) = delete;
  RoutingInformationBase();
  ~RoutingInformationBase();

  struct UpdateStatistics {
    std::size_t v4RoutesAdded{0};
    std::size_t v4RoutesDeleted{0};
    std::size_t v6RoutesAdded{0};
    std::size_t v6RoutesDeleted{0};
    std::size_t mplsRoutesAdded{0};
    std::size_t mplsRoutesDeleted{0};
    std::chrono::microseconds duration{0};
  };

  /*
   * `update()` first acquires exclusive ownership of the RIB and executes the
   * following sequence of actions:
   * 1. Injects and removes routes in `toAdd` and `toDelete`, respectively.
   * 2. Triggers recursive (IP) resolution.
   * 3. Updates the FIB synchronously.
   * NOTE : there is no order guarantee b/w toAdd and toDelete. We may do
   * either first. This does not matter for non overlapping add/del, but
   * can be meaningful for overlaps. If so, the caller is responsible for
   * ensuring this order - e.g. by first calling update with only toAdd
   * routes and then with toDel routes or vice versa. In our fboss agent
   * applications we separate this out by making synchronous calls in
   * response to add/del route thrift calls, which are distinct apis.
   * TODO: Consider breaking down update into add, del, syncClient
   * interfaces
   *
   * If a UnicastRoute does not specify its admin distance, then we derive its
   * admin distance via its clientID.  This is accomplished by a mapping from
   * client IDs to admin distances provided in configuration. Unfortunately,
   * this mapping is exposed via SwSwitch, which we can't a dependency on here.
   * The adminDistanceFromClientID allows callsites to propagate admin distances
   * per client.
   */
  UpdateStatistics update(
      const SwitchIdScopeResolver* resolver,
      RouterID routerID,
      ClientID clientID,
      AdminDistance adminDistanceFromClientID,
      const std::vector<UnicastRoute>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      bool resetClientsRoutes,
      folly::StringPiece updateType,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      void* cookie);

  UpdateStatistics update(
      const SwitchIdScopeResolver* resolver,
      RouterID routerID,
      ClientID clientID,
      AdminDistance adminDistanceFromClientID,
      const std::vector<MplsRoute>& toAdd,
      const std::vector<MplsLabel>& toDelete,
      bool resetClientsRoutes,
      folly::StringPiece updateType,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      void* cookie);
  /*
   * Update mySids in RIB and switchState
   */
  void update(
      const SwitchIdScopeResolver* resolver,
      const std::vector<MySidEntry>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      folly::StringPiece updateType,
      const RibMySidToSwitchStateFunction ribMySidToSwitchStateFunc,
      void* cookie);
  /*
   * Update mySids in RIB and switchState from pre-built MySid state objects.
   * Used by the config-driven path (SwSwitch::applyMySidConfig).
   * Each entry in toAdd is (MySid state object, its unresolved next-hop set).
   * The next-hop set is empty for DECAPSULATE_AND_LOOKUP / ADJACENCY_MICRO_SID
   * entries; populated for NODE_MICRO_SID.
   */
  void update(
      const SwitchIdScopeResolver* resolver,
      const std::vector<MySidWithNextHops>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      folly::StringPiece updateType,
      const RibMySidToSwitchStateFunction& ribMySidToSwitchStateFunc,
      void* cookie);
  /*
   * VrfAndNetworkToInterfaceRoute is conceptually a mapping from the pair
   * (RouterID, folly::CIDRNetwork) to the pair (Interface(1),
   * folly::IPAddress). An example of an element in this map is: (RouterID(0),
   * 169.254.0.0/16) --> (Interface(1), 169.254.0.1) This specifies that the
   * network 169.254.0.0/16 in VRF 0 can be reached via Interface 1, which has
   * an address of 169.254.0.1. in that subnet. Note that the IP address in the
   * key has its mask applied to it while the IP address value doesn't.
   */
  using RouterIDAndNetworkToInterfaceRoutes =
      RibRouteTables::RouterIDAndNetworkToInterfaceRoutes;

  void reconfigure(
      const SwitchIdScopeResolver* resolver,
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes,
      const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu,
      const std::vector<cfg::StaticIp2MplsRoute>& staticIp2MplsRoutes,
      const std::vector<cfg::StaticMplsRouteWithNextHops>&
          staticMplsRoutesWithNextHops,
      const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
      const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCpu,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      void* cookie);

  void updateRemoteInterfaceRoutes(
      const SwitchIdScopeResolver* resolver,
      const RouterIDAndNetworkToInterfaceRoutes& toAdd,
      const boost::container::flat_map<
          facebook::fboss::RouterID,
          std::vector<folly::CIDRNetwork>>& toDel,
      const RibToSwitchStateFunction& ribToSwitchStateFunc,
      void* cookie);

  void setClassID(
      const SwitchIdScopeResolver* resolver,
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      std::optional<cfg::AclLookupClass> classId,
      void* cookie) {
    setClassIDImpl(
        resolver, rid, prefixes, ribToSwitchStateFunc, classId, cookie, false);
  }

  void setClassIDAsync(
      const SwitchIdScopeResolver* resolver,
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      std::optional<cfg::AclLookupClass> classId,
      void* cookie) {
    setClassIDImpl(
        resolver, rid, prefixes, ribToSwitchStateFunc, classId, cookie, true);
  }
  void updateEcmpOverrides(const StateDelta& delta);
  void updateStateInRibThread(const std::function<void()>& fn);

  /*
   * FIB assisted fromThrift. With shared data structure of routes
   * all except the unresolved routes are shared b/w rib and FIB, so
   * we can simply reconstruct RIB by ser/deser unresolved routes
   * and importing FIB
   */
  static std::unique_ptr<RoutingInformationBase> fromThrift(
      const std::map<int32_t, state::RouteTableFields>& ribJson,
      const std::shared_ptr<MultiSwitchFibInfoMap>& fibsInfoMap,
      const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFib,
      const std::shared_ptr<MultiSwitchMySidMap>& mySidMap);

  void ensureVrf(RouterID rid) {
    ribTables_.ensureVrf(rid);
  }
  std::vector<RouterID> getVrfList() const {
    return ribTables_.getVrfList();
  }
  std::vector<RouteDetails> getRouteTableDetails(RouterID rid) const {
    return ribTables_.getRouteTableDetails(rid);
  }
  std::vector<MplsRouteDetails> getMplsRouteTableDetails() const {
    return ribTables_.getMplsRouteTableDetails();
  }
  std::unordered_map<folly::CIDRNetworkV6, state::MySidFields>
  getMySidTableCopy() const {
    return ribTables_.getMySidTableCopy();
  }
  void waitForRibUpdates() {
    ensureRunning();
    ribUpdateEventBase_.runInFbossEventBaseThreadAndWait([] { return; });
  }

  void stop();

  template <typename AddressT>
  std::shared_ptr<Route<AddressT>> longestMatch(
      const AddressT& address,
      RouterID vrf) const {
    return ribTables_.longestMatch(address, vrf);
  }

  template <typename AddressT>
  std::optional<std::pair<std::shared_ptr<Route<AddressT>>, RouteNextHopSet>>
  getRouteAndNextHops(
      const AddressT& address,
      RouterID vrf,
      bool normalized = true) const {
    return ribTables_.getRouteAndNextHops(address, vrf, normalized);
  }

  std::map<int32_t, state::RouteTableFields> toThrift() const;
  static std::unique_ptr<RoutingInformationBase> fromThrift(
      const std::map<int32_t, state::RouteTableFields>&);
  std::map<int32_t, state::RouteTableFields> warmBootState() const;

  // Returns a deep copy of the NextHopIDManager. This is an expensive
  // operation and is intended only for tests.
  std::unique_ptr<NextHopIDManager> getNextHopIDManagerCopy() const {
    return ribTables_.getNextHopIDManagerCopy();
  }

  // Named next-hop group operations. These run on the RIB thread to ensure
  // thread-safe access to NextHopIDManager. Allocation/deallocation is followed
  // by a state update callback (under rlock) to sync switch state.
  void addOrUpdateNamedNextHopGroups(
      const std::vector<std::pair<std::string, RouteNextHopSet>>& groups,
      const std::function<void(const NextHopIDManager*)>& stateUpdateFn);

  void deleteNamedNextHopGroups(
      const std::vector<std::string>& names,
      const std::function<void(const NextHopIDManager*)>& stateUpdateFn);

 private:
  void ensureRunning() const;
  void setClassIDImpl(
      const SwitchIdScopeResolver* resolver,
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      std::optional<cfg::AclLookupClass> classId,
      void* cookie,
      bool async);

  template <typename TraitsType>
  UpdateStatistics updateImpl(
      const SwitchIdScopeResolver* resolver,
      RouterID routerID,
      ClientID clientID,
      AdminDistance adminDistanceFromClientID,
      const std::vector<typename TraitsType::ThriftRoute>& toAdd,
      const std::vector<typename TraitsType::ThriftRouteId>& toDelete,
      bool resetClientsRoutes,
      folly::StringPiece updateType,
      RibToSwitchStateFunction ribToSwitchStateFunc,
      void* cookie);

  std::unique_ptr<std::thread> ribUpdateThread_;
  FbossEventBase ribUpdateEventBase_{"RibUpdateEventBase"};
  RibRouteTables ribTables_;
};

} // namespace facebook::fboss
