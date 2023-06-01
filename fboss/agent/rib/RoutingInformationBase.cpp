/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/rib/RoutingInformationBase.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/rib/ConfigApplier.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/rib/RouteUpdater.h"

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#include <folly/ScopeGuard.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

namespace {

class RibIpRouteUpdate {
 public:
  using ThriftRoute = UnicastRoute;
  using ThriftRouteId = IpPrefix;
  using RibRoute = RibRouteUpdater::RouteEntry;
  using RibRouteId = folly::CIDRNetwork;
  static RibRoute ToAddFn(
      const ThriftRoute& route,
      const AdminDistance distance,
      RoutingInformationBase::UpdateStatistics& stats) {
    auto network = facebook::network::toIPAddress(*route.dest()->ip());
    auto mask = static_cast<uint8_t>(*route.dest()->prefixLength());
    std::optional<RouteCounterID> counterID;
    std::optional<cfg::AclLookupClass> classID;
    if (route.counterID().has_value()) {
      counterID = route.counterID().value();
    }
    if (route.classID().has_value()) {
      classID = route.classID().value();
    }
    if (network.isV4()) {
      ++stats.v4RoutesAdded;
    } else {
      ++stats.v6RoutesAdded;
    }
    return RibRoute{
        {network, mask},
        RouteNextHopEntry::from(route, distance, counterID, classID)};
  }
  static RibRouteId ToDelFn(
      const ThriftRouteId& prefix,
      RoutingInformationBase::UpdateStatistics& stats) {
    auto network = facebook::network::toIPAddress(*prefix.ip());
    auto mask = static_cast<uint8_t>(*prefix.prefixLength());
    if (network.isV4()) {
      ++stats.v4RoutesDeleted;
    } else {
      ++stats.v6RoutesDeleted;
    }
    return {network, mask};
  }
};

class RibMplsRouteUpdate {
 public:
  using ThriftRoute = MplsRoute;
  using ThriftRouteId = MplsLabel;
  using RibRoute = RibRouteUpdater::MplsRouteEntry;
  using RibRouteId = LabelID;
  static RibRoute ToAddFn(
      const ThriftRoute& route,
      const AdminDistance distance,
      RoutingInformationBase::UpdateStatistics& stats) {
    ++stats.mplsRoutesAdded;
    return RibRoute{
        LabelID(route.get_topLabel()),
        RouteNextHopEntry::from(route, distance, std::nullopt, std::nullopt)};
  }

  static RibRouteId ToDelFn(
      const ThriftRouteId& topLabel,
      RoutingInformationBase::UpdateStatistics& stats) {
    ++stats.mplsRoutesDeleted;
    return RibRouteId(topLabel);
  }
};

class Timer {
 public:
  explicit Timer(std::chrono::microseconds* duration)
      : duration_(duration), start_(std::chrono::steady_clock::now()) {}

  ~Timer() {
    auto end = std::chrono::steady_clock::now();
    *duration_ =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
  }

 private:
  std::chrono::microseconds* duration_;
  std::chrono::time_point<std::chrono::steady_clock> start_;
};

template <typename AddressT, typename FibType>
void reconstructRibFromFib(
    const std::shared_ptr<FibType>& fib,
    NetworkToRouteMap<AddressT>* addrToRoute) {
  // FIB has all but unresolved routes from RIB
  std::vector<std::shared_ptr<Route<AddressT>>> unresolvedRoutes;
  std::for_each(
      addrToRoute->begin(),
      addrToRoute->end(),
      [&unresolvedRoutes](auto& node) {
        auto& route = value(node);
        if (!route->isResolved()) {
          unresolvedRoutes.push_back(route);
        }
      });
  addrToRoute->clear();
  if constexpr (!std::is_same_v<FibType, MultiLabelForwardingInformationBase>) {
    for (auto& iter : std::as_const(*fib)) {
      const auto& route = iter.second;
      addrToRoute->insert(route->prefix(), route);
    }
  } else {
    for (auto& miter : std::as_const(*fib)) {
      for (const auto& iter : std::as_const(*miter.second)) {
        const auto& route = iter.second;
        addrToRoute->insert(route->prefix(), route);
      }
    }
  }
  // Copy unresolved routes
  std::for_each(
      unresolvedRoutes.begin(),
      unresolvedRoutes.end(),
      [addrToRoute](const auto& route) {
        addrToRoute->insert(route->prefix(), route);
      });
}
} // namespace

template <typename RibUpdateFn>
void RibRouteTables::updateRib(RouterID vrf, const RibUpdateFn& updateRibFn) {
  auto lockedRouteTables = synchronizedRouteTables_.wlock();
  auto it = lockedRouteTables->find(vrf);
  if (it == lockedRouteTables->end()) {
    throw FbossError("VRF ", vrf, " not configured");
  }
  auto& routeTable = it->second;
  updateRibFn(routeTable);
}

void RibRouteTables::reconfigure(
    const SwitchIdScopeResolver* resolver,
    const RouterIDAndNetworkToInterfaceRoutes& configRouterIDToInterfaceRoutes,
    const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
    const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
    const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu,
    const std::vector<cfg::StaticIp2MplsRoute>& staticIp2MplsRoutes,
    const std::vector<cfg::StaticMplsRouteWithNextHops>&
        staticMplsRoutesWithNextHops,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCpu,
    FibUpdateFunction updateFibCallback,
    void* cookie) {
  // Config application is accomplished in the following sequence of steps:
  // 1. Update the VRFs held in RoutingInformationBase's
  // SynchronizedRouteTables data-structure
  //
  // For each VRF specified in config:
  //
  // 2. Update all of RIB's static routes to be only those specified in
  // config
  //
  // 3. Update all of RIB's interface routes to be only those specified in
  // config
  //
  // 4. Re-resolve routes
  //
  // 5. Update FIB
  //
  // Steps 2-5 take place in ConfigApplier.

  std::vector<RouterID> existingVrfs = getVrfList();

  auto configureRoutesForVrf = [&](RouterID vrf,
                                   const PrefixToInterfaceIDAndIP&
                                       interfaceRoutes) {
    // A ConfigApplier object should be independent of the VRF whose
    // routes it is processing. However, because interface and static
    // routes for _all_ VRFs are passed to ConfigApplier, the vrf
    // argument is needed to identify the subset of those routes which
    // should be processed.

    // ConfigApplier can be made independent of the VRF whose routes it
    // is processing by the use of boost::filter_iterator.
    updateRib(vrf, [&](auto& routeTable) {
      ConfigApplier configApplier(
          vrf,
          &(routeTable.v4NetworkToRoute),
          &(routeTable.v6NetworkToRoute),
          &(routeTable.labelToRoute),
          folly::range(interfaceRoutes.cbegin(), interfaceRoutes.cend()),
          folly::range(staticRoutesToCpu.cbegin(), staticRoutesToCpu.cend()),
          folly::range(staticRoutesToNull.cbegin(), staticRoutesToNull.cend()),
          folly::range(
              staticRoutesWithNextHops.cbegin(),
              staticRoutesWithNextHops.cend()),
          folly::range(
              staticIp2MplsRoutes.cbegin(), staticIp2MplsRoutes.cend()),
          folly::range(
              staticMplsRoutesWithNextHops.cbegin(),
              staticMplsRoutesWithNextHops.cend()),
          folly::range(
              staticMplsRoutesToNull.cbegin(), staticMplsRoutesToNull.cend()),
          folly::range(
              staticMplsRoutesToCpu.cbegin(), staticMplsRoutesToCpu.cend()));
      // Apply config
      configApplier.apply();
    });
    updateFib(resolver, vrf, updateFibCallback, cookie);
  };
  // Because of this sequential loop over each VRF, config application scales
  // linearly with the number of VRFs. If FBOSS is run in a multi-VRF routing
  // architecture in the future, this slow-down can be avoided by
  // parallelizing this loop. Converting this loop to use task-level
  // parallelism should be straightfoward because it has been written to avoid
  // dependencies across different iterations of the loop.
  for (auto vrf : existingVrfs) {
    // First handle the VRFs for which no interface routes exist
    if (configRouterIDToInterfaceRoutes.find(vrf) !=
        configRouterIDToInterfaceRoutes.end()) {
      continue;
    }
    configureRoutesForVrf(
        vrf, {} /* No interface routes*/
    );
  }
  {
    auto lockedRouteTables = synchronizedRouteTables_.wlock();
    *lockedRouteTables = constructRouteTables(
        lockedRouteTables, configRouterIDToInterfaceRoutes);
  }
  for (auto& vrf : getVrfList()) {
    const auto& interfaceRoutes = configRouterIDToInterfaceRoutes.at(vrf);
    configureRoutesForVrf(vrf, interfaceRoutes);
  }
}

template <typename RouteType, typename RouteIdType>
void RibRouteTables::update(
    const SwitchIdScopeResolver* resolver,
    RouterID routerID,
    ClientID clientID,
    AdminDistance adminDistanceFromClientID,
    const std::vector<RouteType>& toAddRoutes,
    const std::vector<RouteIdType>& toDelPrefixes,
    bool resetClientsRoutes,
    folly::StringPiece updateType,
    const FibUpdateFunction& fibUpdateCallback,
    void* cookie) {
  updateRib(routerID, [&](auto& routeTable) {
    RibRouteUpdater updater(
        &(routeTable.v4NetworkToRoute),
        &(routeTable.v6NetworkToRoute),
        &(routeTable.labelToRoute));
    updater.update(clientID, toAddRoutes, toDelPrefixes, resetClientsRoutes);
  });
  updateFib(resolver, routerID, fibUpdateCallback, cookie);
}

void RibRouteTables::updateFib(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const FibUpdateFunction& fibUpdateCallback,
    void* cookie) {
  try {
    auto lockedRouteTables = synchronizedRouteTables_.rlock();
    auto& routeTable = lockedRouteTables->find(vrf)->second;
    fibUpdateCallback(
        resolver,
        vrf,
        routeTable.v4NetworkToRoute,
        routeTable.v6NetworkToRoute,
        routeTable.labelToRoute,
        cookie);
  } catch (const FbossHwUpdateError& hwUpdateError) {
    {
      SCOPE_FAIL {
        XLOG(FATAL) << " RIB Rollback failed, aborting program";
      };
      auto fib = hwUpdateError.appliedState->getFibs()->getNode(vrf);
      auto lockedRouteTables = synchronizedRouteTables_.wlock();
      auto& routeTable = lockedRouteTables->find(vrf)->second;
      reconstructRibFromFib<
          folly::IPAddressV4,
          ForwardingInformationBase<folly::IPAddressV4>>(
          fib->getFibV4(), &routeTable.v4NetworkToRoute);
      reconstructRibFromFib<
          folly::IPAddressV6,
          ForwardingInformationBase<folly::IPAddressV6>>(
          fib->getFibV6(), &routeTable.v6NetworkToRoute);
      if (FLAGS_mpls_rib) {
        auto labelFib =
            hwUpdateError.appliedState->getLabelForwardingInformationBase();
        reconstructRibFromFib<LabelID, MultiLabelForwardingInformationBase>(
            std::move(labelFib), &routeTable.labelToRoute);
      }
    }
    throw;
  }
}

void RibRouteTables::ensureVrf(RouterID rid) {
  auto lockedRouteTables = synchronizedRouteTables_.wlock();
  if (lockedRouteTables->find(rid) == lockedRouteTables->end()) {
    lockedRouteTables->insert(std::make_pair(rid, RouteTable()));
  }
}

std::vector<RouterID> RibRouteTables::getVrfList() const {
  auto lockedRouteTables = synchronizedRouteTables_.rlock();
  std::vector<RouterID> res(lockedRouteTables->size());
  for (const auto& entry : *lockedRouteTables) {
    res.push_back(entry.first);
  }
  return res;
}

void RibRouteTables::setClassID(
    const SwitchIdScopeResolver* resolver,
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    FibUpdateFunction fibUpdateCallback,
    std::optional<cfg::AclLookupClass> classId,
    void* cookie) {
  updateRib(rid, [&](auto& routeTable) {
    // Update rib
    auto updateRoute = [&classId](auto& rib, auto ip, uint8_t mask) {
      auto ritr = rib.exactMatch(ip, mask);
      if (ritr == rib.end() || ritr->value()->getClassID() == classId) {
        return;
      }
      ritr->value() = ritr->value()->clone();
      ritr->value()->updateClassID(classId);
      ritr->value()->publish();
    };
    auto& v4Rib = routeTable.v4NetworkToRoute;
    auto& v6Rib = routeTable.v6NetworkToRoute;
    for (auto& prefix : prefixes) {
      if (prefix.first.isV4()) {
        updateRoute(v4Rib, prefix.first.asV4(), prefix.second);
      } else {
        updateRoute(v6Rib, prefix.first.asV6(), prefix.second);
      }
    }
  });
  updateFib(resolver, rid, fibUpdateCallback, cookie);
}

template <typename AddressT>
std::shared_ptr<Route<AddressT>> RibRouteTables::longestMatch(
    const AddressT& address,
    RouterID vrf) const {
  StopWatch lookupTimer(std::nullopt, false);
  auto ribTables = synchronizedRouteTables_.rlock();
  auto vrfIt = ribTables->find(vrf);
  auto rt =
      vrfIt == ribTables->end() ? nullptr : vrfIt->second.longestMatch(address);
  if (lookupTimer.msecsElapsed().count() > 1000) {
    XLOG(WARNING) << " Lookup for : " << address
                  << " took: " << lookupTimer.msecsElapsed().count() << " ms ";
  }
  return rt;
}

RibRouteTables::RouterIDToRouteTable RibRouteTables::constructRouteTables(
    const SynchronizedRouteTables::WLockedPtr& lockedRouteTables,
    const RouterIDAndNetworkToInterfaceRoutes& configRouterIDToInterfaceRoutes)
    const {
  RouterIDToRouteTable newRouteTables;

  RouterIDToRouteTable::iterator newRouteTablesIter;
  for (const auto& routerIDAndInterfaceRoutes :
       configRouterIDToInterfaceRoutes) {
    const RouterID configVrf = routerIDAndInterfaceRoutes.first;

    newRouteTablesIter = newRouteTables.emplace_hint(
        newRouteTables.cend(), configVrf, RouteTable());

    auto oldRouteTablesIter = lockedRouteTables->find(configVrf);
    if (oldRouteTablesIter == lockedRouteTables->end()) {
      // configVrf did not exist in the RIB, so it has been added to
      // newRouteTables with an empty set of routes
      continue;
    }

    // configVrf exists in the RIB, so it will be shallow copied into
    // newRouteTables.
    newRouteTablesIter->second = std::move(oldRouteTablesIter->second);
  }

  return newRouteTables;
}

RoutingInformationBase::RoutingInformationBase() {
  ribUpdateThread_ = std::make_unique<std::thread>([this] {
    initThread("ribUpdateThread");
    ribUpdateEventBase_.loopForever();
  });
}

RoutingInformationBase::~RoutingInformationBase() {
  stop();
}

void RoutingInformationBase::stop() {
  if (ribUpdateThread_) {
    ribUpdateEventBase_.runInEventBaseThread(
        [this] { ribUpdateEventBase_.terminateLoopSoon(); });
    ribUpdateThread_->join();
    ribUpdateThread_.reset();
  }
}

void RoutingInformationBase::ensureRunning() const {
  if (!ribUpdateThread_) {
    throw FbossError(
        "RIB thread is not yet running or is in the process of exiting");
  }
}

void RoutingInformationBase::reconfigure(
    const SwitchIdScopeResolver* resolver,
    const RouterIDAndNetworkToInterfaceRoutes& configRouterIDToInterfaceRoutes,
    const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
    const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
    const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu,
    const std::vector<cfg::StaticIp2MplsRoute>& staticIp2MplsRoutes,
    const std::vector<cfg::StaticMplsRouteWithNextHops>&
        staticMplsRoutesWithNextHops,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToNull,
    const std::vector<cfg::StaticMplsRouteNoNextHops>& staticMplsRoutesToCpu,
    FibUpdateFunction updateFibCallback,
    void* cookie) {
  ensureRunning();
  auto updateFn = [&] {
    ribTables_.reconfigure(
        resolver,
        configRouterIDToInterfaceRoutes,
        staticRoutesWithNextHops,
        staticRoutesToNull,
        staticRoutesToCpu,
        staticIp2MplsRoutes,
        staticMplsRoutesWithNextHops,
        staticMplsRoutesToNull,
        staticMplsRoutesToCpu,
        updateFibCallback,
        cookie);
  };
  ribUpdateEventBase_.runInEventBaseThreadAndWait(updateFn);
}

template <typename TraitsType>
RoutingInformationBase::UpdateStatistics RoutingInformationBase::updateImpl(
    const SwitchIdScopeResolver* resolver,
    RouterID routerID,
    ClientID clientID,
    AdminDistance adminDistanceFromClientID,
    const std::vector<typename TraitsType::ThriftRoute>& toAdd,
    const std::vector<typename TraitsType::ThriftRouteId>& toDelete,
    bool resetClientsRoutes,
    folly::StringPiece updateType,
    FibUpdateFunction fibUpdateCallback,
    void* cookie) {
  ensureRunning();
  UpdateStatistics stats;
  std::chrono::microseconds duration;
  std::shared_ptr<SwitchState> appliedState;
  Timer updateTimer(&duration);
  std::exception_ptr updateException;
  auto updateFn = [&]() {
    std::vector<typename TraitsType::RibRoute> toAddRoutes;
    toAddRoutes.reserve(toAdd.size());

    try {
      std::for_each(
          toAdd.begin(),
          toAdd.end(),
          [adminDistanceFromClientID, &stats, &toAddRoutes](const auto& route) {
            toAddRoutes.push_back(
                TraitsType::ToAddFn(route, adminDistanceFromClientID, stats));
          });
      std::vector<typename TraitsType::RibRouteId> toDelPrefixes;
      toDelPrefixes.reserve(toDelete.size());
      std::for_each(
          toDelete.begin(),
          toDelete.end(),
          [&stats, &toDelPrefixes](const auto& prefix) {
            toDelPrefixes.push_back(TraitsType::ToDelFn(prefix, stats));
          });

      ribTables_.update(
          resolver,
          routerID,
          clientID,
          adminDistanceFromClientID,
          toAddRoutes,
          toDelPrefixes,
          resetClientsRoutes,
          updateType,
          fibUpdateCallback,
          cookie);
    } catch (const std::exception& e) {
      updateException = std::current_exception();
    }
  };
  ribUpdateEventBase_.runInEventBaseThreadAndWait(updateFn);
  if (updateException) {
    std::rethrow_exception(updateException);
  }
  stats.duration = duration;
  return stats;
}

void RoutingInformationBase::setClassIDImpl(
    const SwitchIdScopeResolver* resolver,
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    FibUpdateFunction fibUpdateCallback,
    std::optional<cfg::AclLookupClass> classId,
    void* cookie,
    bool async) {
  ensureRunning();
  auto updateFn = [=]() {
    ribTables_.setClassID(
        resolver, rid, prefixes, fibUpdateCallback, classId, cookie);
  };
  if (async) {
    ribUpdateEventBase_.runInEventBaseThread(updateFn);
  } else {
    ribUpdateEventBase_.runInEventBaseThreadAndWait(updateFn);
  }
}

RibRouteTables RibRouteTables::fromThrift(
    const std::map<int32_t, state::RouteTableFields>& ribThrift,
    const std::shared_ptr<MultiSwitchForwardingInformationBaseMap>& fibs,
    const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFib) {
  RibRouteTables rib;
  auto lockedRouteTables = rib.synchronizedRouteTables_.wlock();

  for (const auto& [rid, table] : ribThrift) {
    RouteTable rtable = RouteTable::fromThrift(table);
    auto vrf = RouterID(rid);
    lockedRouteTables->emplace(vrf, std::move(rtable));
  }

  if (fibs) {
    rib.importFibs(lockedRouteTables, fibs, labelFib);
  }
  return rib;
}

std::unique_ptr<RoutingInformationBase> RoutingInformationBase::fromThrift(
    const std::map<int32_t, state::RouteTableFields>& ribThrift,
    const std::shared_ptr<MultiSwitchForwardingInformationBaseMap>& fibs,
    const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFib) {
  auto rib = std::make_unique<RoutingInformationBase>();
  rib->ribTables_ = RibRouteTables::fromThrift(ribThrift, fibs, labelFib);
  return rib;
}

std::vector<MplsRouteDetails> RibRouteTables::getMplsRouteTableDetails() const {
  std::vector<MplsRouteDetails> mplsRouteDetails;
  synchronizedRouteTables_.withRLock([&](const auto& synchronizedRouteTables) {
    const auto it = synchronizedRouteTables.find(RouterID(0));
    if (it != synchronizedRouteTables.end()) {
      for (auto rit = it->second.labelToRoute.begin();
           rit != it->second.labelToRoute.end();
           ++rit) {
        MplsRouteDetails mplsRouteDetail;
        auto routeDetails = rit->second->toRouteDetails();
        mplsRouteDetail.topLabel() = rit->first;
        mplsRouteDetail.nextHopMulti() = *routeDetails.nextHopMulti();
        mplsRouteDetail.nextHops() = *routeDetails.nextHops();
        if (routeDetails.adminDistance().has_value()) {
          mplsRouteDetail.adminDistance() = *routeDetails.adminDistance();
        }
        mplsRouteDetails.emplace_back(mplsRouteDetail);
      }
    }
  });
  return mplsRouteDetails;
}

std::vector<RouteDetails> RibRouteTables::getRouteTableDetails(
    RouterID rid) const {
  std::vector<RouteDetails> routeDetails;
  synchronizedRouteTables_.withRLock([&](const auto& synchronizedRouteTables) {
    const auto it = synchronizedRouteTables.find(rid);
    if (it != synchronizedRouteTables.end()) {
      for (auto rit = it->second.v4NetworkToRoute.begin();
           rit != it->second.v4NetworkToRoute.end();
           ++rit) {
        routeDetails.emplace_back(rit->value()->toRouteDetails());
      }
      for (auto rit = it->second.v6NetworkToRoute.begin();
           rit != it->second.v6NetworkToRoute.end();
           ++rit) {
        routeDetails.emplace_back(rit->value()->toRouteDetails());
      }
    }
  });
  return routeDetails;
}

RoutingInformationBase::UpdateStatistics RoutingInformationBase::update(
    const SwitchIdScopeResolver* resolver,
    RouterID routerID,
    ClientID clientID,
    AdminDistance adminDistanceFromClientID,
    const std::vector<UnicastRoute>& toAdd,
    const std::vector<IpPrefix>& toDelete,
    bool resetClientsRoutes,
    folly::StringPiece updateType,
    FibUpdateFunction fibUpdateCallback,
    void* cookie) {
  return updateImpl<RibIpRouteUpdate>(
      resolver,
      routerID,
      clientID,
      adminDistanceFromClientID,
      toAdd,
      toDelete,
      resetClientsRoutes,
      updateType,
      fibUpdateCallback,
      cookie);
}

RoutingInformationBase::UpdateStatistics RoutingInformationBase::update(
    const SwitchIdScopeResolver* resolver,
    RouterID routerID,
    ClientID clientID,
    AdminDistance adminDistanceFromClientID,
    const std::vector<MplsRoute>& toAdd,
    const std::vector<MplsLabel>& toDelete,
    bool resetClientsRoutes,
    folly::StringPiece updateType,
    FibUpdateFunction fibUpdateCallback,
    void* cookie) {
  return updateImpl<RibMplsRouteUpdate>(
      resolver,
      routerID,
      clientID,
      adminDistanceFromClientID,
      toAdd,
      toDelete,
      resetClientsRoutes,
      updateType,
      fibUpdateCallback,
      cookie);
}

state::RouteTableFields RibRouteTables::RouteTable ::toThrift() const {
  state::RouteTableFields obj{};
  obj.v4NetworkToRoute() = v4NetworkToRoute.toThrift();
  obj.v6NetworkToRoute() = v6NetworkToRoute.toThrift();
  obj.labelToRoute() = labelToRoute.toThrift();
  return obj;
}

state::RouteTableFields RibRouteTables::RouteTable::warmBootState() const {
  state::RouteTableFields obj{};
  obj.v4NetworkToRoute() = v4NetworkToRoute.warmBootState();
  obj.v6NetworkToRoute() = v6NetworkToRoute.warmBootState();
  obj.labelToRoute() = labelToRoute.warmBootState();
  return obj;
}

RibRouteTables::RouteTable RibRouteTables::RouteTable::fromThrift(
    const state::RouteTableFields& obj) {
  RouteTable routeTable;
  routeTable.v4NetworkToRoute =
      IPv4NetworkToRouteMap::fromThrift(*obj.v4NetworkToRoute());
  routeTable.v6NetworkToRoute =
      IPv6NetworkToRouteMap::fromThrift(*obj.v6NetworkToRoute());
  routeTable.labelToRoute = LabelToRouteMap::fromThrift(*obj.labelToRoute());
  return routeTable;
}

std::map<int32_t, state::RouteTableFields> RibRouteTables::toThrift() const {
  std::map<int32_t, state::RouteTableFields> obj{};
  auto routeTables = synchronizedRouteTables_.rlock();
  for (const auto& [rid, routeTable] : *routeTables) {
    obj.emplace(rid, routeTable.toThrift());
  }
  return obj;
}

std::map<int32_t, state::RouteTableFields> RibRouteTables::warmBootState()
    const {
  std::map<int32_t, state::RouteTableFields> obj{};
  const auto& routeTables = *synchronizedRouteTables_.rlock();
  for (const auto& [rid, routeTable] : routeTables) {
    obj.emplace(rid, routeTable.warmBootState());
  }
  return obj;
}

RibRouteTables RibRouteTables::fromThrift(
    const std::map<int32_t, state::RouteTableFields>& obj) {
  RibRouteTables ribRouteTables;
  auto routeTables = ribRouteTables.synchronizedRouteTables_.wlock();
  for (const auto& [rid, routeTableFields] : obj) {
    // @lint-ignore CLANGTIDY
    routeTables->emplace(
        RouterID(rid),
        RibRouteTables::RouteTable::fromThrift(routeTableFields));
  }
  return ribRouteTables;
}

std::map<int32_t, state::RouteTableFields> RoutingInformationBase::toThrift()
    const {
  return ribTables_.toThrift();
}
std::unique_ptr<RoutingInformationBase> RoutingInformationBase::fromThrift(
    const std::map<int32_t, state::RouteTableFields>& obj) {
  auto rib = std::make_unique<RoutingInformationBase>();
  rib->ribTables_ = RibRouteTables::fromThrift(obj);
  return rib;
}

void RibRouteTables::importFibs(
    const SynchronizedRouteTables::WLockedPtr& lockedRouteTables,
    const std::shared_ptr<MultiSwitchForwardingInformationBaseMap>&
        multiSwitchfibs,
    const std::shared_ptr<MultiLabelForwardingInformationBase>& labelFibs) {
  auto importRoutes = [](const auto& fib, auto* addrToRoute) {
    for (const auto& iter : std::as_const(*fib)) {
      const auto& route = iter.second;
      auto [itr, inserted] = addrToRoute->insert(route->prefix(), route);
      if (!inserted) {
        // If RIB already had a route, replace it with FIB route so we
        // share the same objects. The only case where this can occur is
        // when we WB from old style RIB ser (all routes ser) to FIB assisted
        // ser/deser
        itr->value() = route;
      }
      DCHECK_EQ(
          addrToRoute
              ->exactMatch(route->prefix().network(), route->prefix().mask())
              ->value(),
          route);
    }
  };
  for (const auto& [_, fibs] : std::as_const(*multiSwitchfibs)) {
    for (const auto& iter : std::as_const(*fibs)) {
      const auto& fib = iter.second;
      auto& routeTables = (*lockedRouteTables)[fib->getID()];
      importRoutes(fib->getFibV6(), &routeTables.v6NetworkToRoute);
      importRoutes(fib->getFibV4(), &routeTables.v4NetworkToRoute);
      auto mplsTable = &routeTables.labelToRoute;
      if (FLAGS_mpls_rib && labelFibs) {
        for (const auto& [_, labelFib] : std::as_const(*labelFibs)) {
          for (const auto& entry : std::as_const(*labelFib)) {
            const auto& route = entry.second;
            auto [itr, inserted] = mplsTable->insert(route->prefix(), route);
            if (!inserted) {
              itr->second = route;
            }
            DCHECK_EQ(mplsTable->find(route->getID())->second, route);
          }
        }
      }
    }
  }
}

std::map<int32_t, state::RouteTableFields>
RoutingInformationBase::warmBootState() const {
  return ribTables_.warmBootState();
}

template std::shared_ptr<Route<folly::IPAddressV4>>
RibRouteTables::longestMatch(const folly::IPAddressV4& address, RouterID vrf)
    const;
template std::shared_ptr<Route<folly::IPAddressV6>>
RibRouteTables::longestMatch(const folly::IPAddressV6& address, RouterID vrf)
    const;

} // namespace facebook::fboss
