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
#include "fboss/agent/rib/ConfigApplier.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/rib/RouteUpdater.h"

#include <exception>
#include <memory>
#include <utility>

#include <folly/ScopeGuard.h>
#include <folly/logging/xlog.h>

namespace {
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
} // namespace

namespace facebook::fboss {

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
    const RouterIDAndNetworkToInterfaceRoutes& configRouterIDToInterfaceRoutes,
    const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
    const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
    const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu,
    FibUpdateFunction updateFibCallback,
    void* cookie) {
  ensureRunning();
  auto updateFn = [&] {
    auto lockedRouteTables = synchronizedRouteTables_.wlock();

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

    std::vector<RouterID> existingVrfs;
    std::for_each(
        lockedRouteTables->begin(),
        lockedRouteTables->end(),
        [&existingVrfs](const auto& vrfAndRouteTable) {
          existingVrfs.push_back(vrfAndRouteTable.first);
        });

    auto configureRoutesForVrf =
        [&](RouterID vrf,
            RouteTable& routeTable,
            const PrefixToInterfaceIDAndIP& interfaceRoutes) {
          routeTable.makeWritable(true);
          SCOPE_EXIT {
            routeTable.makeWritable(false);
          };
          // A ConfigApplier object should be independent of the VRF whose
          // routes it is processing. However, because interface and static
          // routes for _all_ VRFs are passed to ConfigApplier, the vrf argument
          // is needed to identify the subset of those routes which should be
          // processed.

          // ConfigApplier can be made independent of the VRF whose routes it is
          // processing by the use of boost::filter_iterator.
          ConfigApplier configApplier(
              vrf,
              &(routeTable.v4NetworkToRoute),
              &(routeTable.v6NetworkToRoute),
              folly::range(interfaceRoutes.cbegin(), interfaceRoutes.cend()),
              folly::range(
                  staticRoutesToCpu.cbegin(), staticRoutesToCpu.cend()),
              folly::range(
                  staticRoutesToNull.cbegin(), staticRoutesToNull.cend()),
              folly::range(
                  staticRoutesWithNextHops.cbegin(),
                  staticRoutesWithNextHops.cend()));
          configApplier.apply();
          updateFib(vrf, &routeTable, updateFibCallback, cookie);
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
          vrf, lockedRouteTables->find(vrf)->second, {} /* No interface routes*/
      );
    }
    *lockedRouteTables = constructRouteTables(
        lockedRouteTables, configRouterIDToInterfaceRoutes);
    for (auto& vrfAndRouteTable : *lockedRouteTables) {
      auto vrf = vrfAndRouteTable.first;
      const auto& interfaceRoutes = configRouterIDToInterfaceRoutes.at(vrf);
      configureRoutesForVrf(vrf, vrfAndRouteTable.second, interfaceRoutes);
    }
  };
  ribUpdateEventBase_.runInEventBaseThreadAndWait(updateFn);
}

RoutingInformationBase::UpdateStatistics RoutingInformationBase::update(
    RouterID routerID,
    ClientID clientID,
    AdminDistance adminDistanceFromClientID,
    const std::vector<UnicastRoute>& toAdd,
    const std::vector<IpPrefix>& toDelete,
    bool resetClientsRoutes,
    folly::StringPiece updateType,
    FibUpdateFunction fibUpdateCallback,
    void* cookie) {
  ensureRunning();
  UpdateStatistics stats;
  std::chrono::microseconds duration;
  {
    std::shared_ptr<SwitchState> appliedState;
    Timer updateTimer(&duration);

    auto lockedRouteTables = synchronizedRouteTables_.wlock();

    auto it = lockedRouteTables->find(routerID);
    if (it == lockedRouteTables->end()) {
      throw FbossError("VRF ", routerID, " not configured");
    }

    std::vector<RibRouteUpdater::RouteEntry> updateLog;
    std::tie(appliedState, stats) = updateImpl(
        &(it->second),
        routerID,
        clientID,
        adminDistanceFromClientID,
        toAdd,
        toDelete,
        resetClientsRoutes,
        updateType,
        fibUpdateCallback,
        cookie,
        &updateLog);
  }
  stats.duration = duration;
  return stats;
}

std::
    pair<std::shared_ptr<SwitchState>, RoutingInformationBase::UpdateStatistics>
    RoutingInformationBase::updateImpl(
        RouteTable* routeTables,
        RouterID routerID,
        ClientID clientID,
        AdminDistance adminDistanceFromClientID,
        const std::vector<UnicastRoute>& toAdd,
        const std::vector<IpPrefix>& toDelete,
        bool resetClientsRoutes,
        folly::StringPiece updateType,
        FibUpdateFunction& fibUpdateCallback,
        void* cookie,
        std::vector<RibRouteUpdater::RouteEntry>* updateLog) {
  UpdateStatistics stats;
  std::shared_ptr<SwitchState> appliedState;

  std::exception_ptr updateException;
  auto updateFn = [&]() {
    routeTables->makeWritable(true);
    SCOPE_EXIT {
      routeTables->makeWritable(false);
    };
    RibRouteUpdater updater(
        &(routeTables->v4NetworkToRoute), &(routeTables->v6NetworkToRoute));
    if (resetClientsRoutes) {
      auto removedRoutes = updater.removeAllRoutesForClient(clientID);
      std::move(
          removedRoutes.begin(),
          removedRoutes.end(),
          std::back_inserter(*updateLog));
    }

    for (const auto& route : toAdd) {
      auto network =
          facebook::network::toIPAddress(*route.dest_ref()->ip_ref());
      auto mask = static_cast<uint8_t>(*route.dest_ref()->prefixLength_ref());

      if (network.isV4()) {
        ++stats.v4RoutesAdded;
      } else {
        ++stats.v6RoutesAdded;
      }

      auto addOrReplace = updater.addOrReplaceRoute(
          network,
          mask,
          clientID,
          RouteNextHopEntry::from(route, adminDistanceFromClientID));
      if (addOrReplace) {
        updateLog->push_back(std::move(*addOrReplace));
      }
    }

    for (const auto& prefix : toDelete) {
      auto network = facebook::network::toIPAddress(*prefix.ip_ref());
      auto mask = static_cast<uint8_t>(*prefix.prefixLength_ref());

      if (network.isV4()) {
        ++stats.v4RoutesDeleted;
      } else {
        ++stats.v6RoutesDeleted;
      }

      auto deleted = updater.delRoute(network, mask, clientID);
      if (deleted) {
        updateLog->push_back(std::move(*deleted));
      }
    }

    updater.updateDone();
    try {
      updateFib(routerID, routeTables, fibUpdateCallback, cookie);
    } catch (const std::exception& ex) {
      updateException = std::current_exception();
    }
  };
  ribUpdateEventBase_.runInEventBaseThreadAndWait(updateFn);
  if (updateException) {
    std::rethrow_exception(updateException);
  }
  return std::make_pair(appliedState, stats);
}

void RoutingInformationBase::setClassIDImpl(
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    FibUpdateFunction fibUpdateCallback,
    std::optional<cfg::AclLookupClass> classId,
    void* cookie,
    bool async) {
  ensureRunning();
  auto updateFn = [=]() {
    auto lockedRouteTables = synchronizedRouteTables_.wlock();

    auto it = lockedRouteTables->find(rid);
    if (it == lockedRouteTables->end()) {
      throw FbossError("VRF ", rid, " not configured");
    }
    auto updateRoute = [&classId](auto& rib, auto ip, uint8_t mask) {
      auto ritr = rib.exactMatch(ip, mask);
      if (ritr == rib.end() || ritr->value()->getClassID() == classId) {
        return;
      }
      ritr->value() = ritr->value()->clone();
      ritr->value()->updateClassID(classId);
      ritr->value()->publish();
    };
    auto& v4Rib = it->second.v4NetworkToRoute;
    auto& v6Rib = it->second.v6NetworkToRoute;
    for (auto& prefix : prefixes) {
      if (prefix.first.isV4()) {
        updateRoute(v4Rib, prefix.first.asV4(), prefix.second);
      } else {
        updateRoute(v6Rib, prefix.first.asV6(), prefix.second);
      }
    }

    updateFib(rid, &(it->second), fibUpdateCallback, cookie);
  };
  if (async) {
    ribUpdateEventBase_.runInEventBaseThread(updateFn);
  } else {
    ribUpdateEventBase_.runInEventBaseThreadAndWait(updateFn);
  }
}

folly::dynamic RoutingInformationBase::toFollyDynamic() const {
  folly::dynamic rib = folly::dynamic::object;

  auto lockedRouteTables = synchronizedRouteTables_.rlock();
  for (const auto& routeTable : *lockedRouteTables) {
    auto routerIdStr =
        folly::to<std::string>(static_cast<uint32_t>(routeTable.first));
    rib[routerIdStr] = folly::dynamic::object;
    rib[routerIdStr][kRouterId] = static_cast<uint32_t>(routeTable.first);
    rib[routerIdStr][kRibV4] =
        routeTable.second.v4NetworkToRoute.toFollyDynamic();
    rib[routerIdStr][kRibV6] =
        routeTable.second.v6NetworkToRoute.toFollyDynamic();
  }

  return rib;
}

std::unique_ptr<RoutingInformationBase>
RoutingInformationBase::fromFollyDynamic(const folly::dynamic& ribJson) {
  auto rib = std::make_unique<RoutingInformationBase>();

  auto lockedRouteTables = rib->synchronizedRouteTables_.wlock();
  auto lockedShadowRouteTables = rib->synchronizedShadowRouteTables_.wlock();
  for (const auto& routeTable : ribJson.items()) {
    auto vrf = RouterID(routeTable.first.asInt());
    lockedRouteTables->insert(std::make_pair(
        vrf,
        RouteTable{
            IPv4NetworkToRouteMap::fromFollyDynamic(routeTable.second[kRibV4]),
            IPv6NetworkToRouteMap::fromFollyDynamic(routeTable.second[kRibV6]),
            true}));

    lockedShadowRouteTables->insert(std::make_pair(
        vrf,
        RouteTable{
            {lockedRouteTables->find(vrf)->second.v4NetworkToRoute.clone()},
            {lockedRouteTables->find(vrf)->second.v6NetworkToRoute.clone()},
            true}));
    CHECK_EQ(
        (*lockedShadowRouteTables)[vrf].v4NetworkToRoute.size(),
        (*lockedRouteTables)[vrf].v4NetworkToRoute.size());
    CHECK_EQ(
        (*lockedShadowRouteTables)[vrf].v6NetworkToRoute.size(),
        (*lockedRouteTables)[vrf].v6NetworkToRoute.size());
  }

  return rib;
}

void RoutingInformationBase::updateFib(
    RouterID vrf,
    RouteTable* routeTable,
    const FibUpdateFunction& fibUpdateCallback,
    void* cookie) {
  std::shared_ptr<SwitchState> appliedState;
  try {
    // Publish routes before sending to FIB
    routeTable->makeWritable(false);
    appliedState = fibUpdateCallback(
        vrf,
        routeTable->v4NetworkToRoute,
        routeTable->v6NetworkToRoute,
        cookie);
  } catch (const FbossHwUpdateError& hwUpdateError) {
    {
      SCOPE_FAIL {
        XLOG(FATAL) << " RIB Rollback failed, aborting program";
      };
      auto lockedShadowRouteTables = synchronizedShadowRouteTables_.rlock();
      // Rollback route tables back to state prior to update failure
      *routeTable = RouteTable{
          {lockedShadowRouteTables->find(vrf)->second.v4NetworkToRoute.clone()},
          {lockedShadowRouteTables->find(vrf)->second.v6NetworkToRoute.clone()},
          false};
      // Attempt to rollback HW
      appliedState = fibUpdateCallback(
          vrf,
          routeTable->v4NetworkToRoute,
          routeTable->v6NetworkToRoute,
          cookie);
    }
    throw FbossHwUpdateError(hwUpdateError.desiredState, appliedState);
  }
  // Get shadow rib to mirror new route tables
  auto lockedShadowRouteTables = synchronizedShadowRouteTables_.wlock();
  auto& shadowRouteTable = (*lockedShadowRouteTables)[vrf];
  // Update shadow rib to new routes
  // TODO: optimize and assert equivalence here
  shadowRouteTable = RouteTable{
      {routeTable->v4NetworkToRoute.clone()},
      {routeTable->v6NetworkToRoute.clone()},
      false};
}

void RoutingInformationBase::RouteTable::makeWritable(bool _writable) {
  if (_writable == writable) {
    return;
  }
  if (_writable) {
    v4NetworkToRoute.cloneAll();
    v6NetworkToRoute.cloneAll();
  } else {
    v4NetworkToRoute.publishAll();
    v6NetworkToRoute.publishAll();
  }
  writable = _writable;
}

void RoutingInformationBase::ensureVrf(RouterID rid) {
  auto lockedRouteTables = synchronizedRouteTables_.wlock();
  lockedRouteTables->insert(std::make_pair(rid, RouteTable()));
}

std::vector<RouterID> RoutingInformationBase::getVrfList() const {
  auto lockedRouteTables = synchronizedRouteTables_.rlock();
  std::vector<RouterID> res(lockedRouteTables->size());
  for (const auto& entry : *lockedRouteTables) {
    res.push_back(entry.first);
  }
  return res;
}

std::vector<RouteDetails> RoutingInformationBase::getRouteTableDetails(
    RouterID rid) const {
  std::vector<RouteDetails> routeDetails;
  SYNCHRONIZED_CONST(synchronizedRouteTables_) {
    const auto it = synchronizedRouteTables_.find(rid);
    if (it != synchronizedRouteTables_.end()) {
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
  }
  return routeDetails;
}

RoutingInformationBase::RouterIDToRouteTable
RoutingInformationBase::constructRouteTables(
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

bool RoutingInformationBase::operator==(
    const RoutingInformationBase& other) const {
  const auto& routeTables = synchronizedRouteTables_.rlock();
  const auto& otherTables = other.synchronizedRouteTables_.rlock();

  return *routeTables == *otherTables;
}

} // namespace facebook::fboss
