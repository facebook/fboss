/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "RouteUpdater.h"

#include <numeric>

#include <boost/integer/common_factor.hpp>

#include <folly/logging/xlog.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"

using boost::container::flat_map;
using boost::container::flat_set;
using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

static const RouteUpdater::PrefixV6 kIPv6LinkLocalPrefix{
    folly::IPAddressV6("fe80::"),
    64};
static const auto kInterfaceRouteClientId = ClientID::INTERFACE_ROUTE;

using std::make_shared;

RouteUpdater::RouteUpdater(const std::shared_ptr<RouteTableMap>& orig)
    : orig_(orig) {
  for (const auto& rt : orig->getAllNodes()) {
    auto& rib = clonedRibs_[rt.first];
    rib.v4.rib = rt.second->getRibV4();
    rib.v6.rib = rt.second->getRibV6();
  }
}

RouteUpdater::ClonedRib* RouteUpdater::createNewRib(RouterID id) {
  ClonedRib newRib;
  newRib.v4.rib = make_shared<RouteTableRibV4>();
  newRib.v4.cloned = true;
  newRib.v6.rib = make_shared<RouteTableRibV6>();
  newRib.v6.cloned = true;
  auto ret = clonedRibs_.emplace(id, newRib);
  if (!ret.second) {
    throw FbossError("Duplicated cloned RIB for vrf ", id);
  }
  return &ret.first->second;
}

RouteUpdater::ClonedRib::RibV4* RouteUpdater::getRibV4(
    RouterID id,
    bool createIfNotExist) {
  auto iter = clonedRibs_.find(id);
  if (iter != clonedRibs_.end()) {
    return &iter->second.v4;
  } else if (!createIfNotExist) {
    return nullptr;
  }
  return &createNewRib(id)->v4;
}

RouteUpdater::ClonedRib::RibV6* RouteUpdater::getRibV6(
    RouterID id,
    bool createIfNotExist) {
  auto iter = clonedRibs_.find(id);
  if (iter != clonedRibs_.end()) {
    return &iter->second.v6;
  } else if (!createIfNotExist) {
    return nullptr;
  }
  return &createNewRib(id)->v6;
}

template <typename RibT>
auto RouteUpdater::makeClone(RibT* rib) -> decltype(rib->rib.get()) {
  if (rib->cloned) {
    return rib->rib.get();
  }
  auto newRtRib = rib->rib->clone();
  rib->rib.swap(newRtRib);
  rib->cloned = true;
  return rib->rib.get();
}

template <typename PrefixT, typename RibT>
void RouteUpdater::addRouteImpl(
    const PrefixT& prefix,
    RibT* ribCloned,
    ClientID clientId,
    RouteNextHopEntry entry) {
  typedef Route<typename PrefixT::AddressT> RouteT;
  auto rib = ribCloned->rib.get();
  auto old = rib->exactMatch(prefix);
  if (old && old->has(clientId, entry)) {
    return;
  }
  rib = makeClone(ribCloned);
  // make sure rib is cloned before any change
  CHECK(ribCloned->cloned);
  if (old) {
    std::shared_ptr<RouteT> newRoute;
    // If the node is not published yet, we assume this thread has exclusive
    // access to the node. Therefore, we can do the modification in-place
    // directly.
    if (old->isPublished()) {
      newRoute = old->clone(
          RouteFields<typename PrefixT::AddressT>::COPY_PREFIX_AND_NEXTHOPS);
      rib->updateRoute(newRoute);
    } else {
      newRoute = old;
    }
    newRoute->update(clientId, std::move(entry));
    XLOG(DBG3) << "Updated route " << newRoute->str();
  } else {
    auto newRoute = make_shared<RouteT>(prefix, clientId, std::move(entry));
    rib->addRoute(newRoute);
    XLOG(DBG3) << "Added route " << newRoute->str();
  }
}

void RouteUpdater::addRoute(
    RouterID id,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId,
    RouteNextHopEntry entry) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    addRouteImpl(prefix, getRibV4(id), clientId, std::move(entry));
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    if (prefix.network.isLinkLocal()) {
      XLOG(DBG2) << "Ignoring v6 link-local interface route: " << prefix.str();
      return;
    }
    addRouteImpl(prefix, getRibV6(id), clientId, std::move(entry));
  }
}

void RouteUpdater::addLinkLocalRoutes(RouterID id) {
  // NOTE: v4 link-local route is not added because currently fboss handles
  // v4 link-locals as normal routes.

  // Add v6 link-local route
  addRouteImpl(
      kIPv6LinkLocalPrefix,
      getRibV6(id),
      ClientID::LINKLOCAL_ROUTE,
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::DIRECTLY_CONNECTED));
}

void RouteUpdater::delLinkLocalRoutes(RouterID id) {
  // Delete v6 link-local route
  delRouteImpl(kIPv6LinkLocalPrefix, getRibV6(id), ClientID::LINKLOCAL_ROUTE);
}

template <typename PrefixT, typename RibT>
void RouteUpdater::delRouteImpl(
    const PrefixT& prefix,
    RibT* ribCloned,
    ClientID clientId) {
  if (!ribCloned) {
    XLOG(DBG3) << "Failed to delete non-existing route " << prefix.str();
    return;
  }
  auto rib = ribCloned->rib.get();
  auto old = rib->exactMatch(prefix);
  if (!old) {
    XLOG(DBG3) << "Failed to delete non-existing route " << prefix.str();
    return;
  }
  rib = makeClone(ribCloned);

  // make sure rib is cloned before any change
  CHECK(ribCloned->cloned);
  // If the node is not published yet, we assume this thread has exclusive
  // access to the node. Therefore, we can do the modification in-place
  // directly.
  if (old->isPublished()) {
    old = old->clone();
    rib->updateRoute(old);
  }
  old->delEntryForClient(clientId);
  // TODO Do I need to publish the change??
  XLOG(DBG3) << "Deleted nexthops for client " << static_cast<int>(clientId)
             << " from route " << prefix.str();
  if (old->hasNoEntry()) {
    rib->removeRoute(old);
    XLOG(DBG3) << "...and then deleted route " << prefix.str();
  }
}

void RouteUpdater::delRoute(
    RouterID id,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    delRouteImpl(prefix, getRibV4(id, false), clientId);
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    delRouteImpl(prefix, getRibV6(id, false), clientId);
  }
}

template <typename AddrT, typename RibT>
void RouteUpdater::removeAllRoutesForClientImpl(
    RibT* ribCloned,
    ClientID clientId) {
  auto rib = makeClone(ribCloned);

  std::vector<std::shared_ptr<Route<AddrT>>> routesToDelete;

  // make sure rib is cloned before any change
  CHECK(ribCloned->cloned);
  for (auto& routeNode : rib->writableRoutes()->writableNodes()) {
    auto route = routeNode.second;
    if (route->isPublished()) {
      route = route->clone();
      routeNode.second = route;
    }
    route->delEntryForClient(clientId);
    if (route->hasNoEntry()) {
      // The nexthops we removed was the only one.  Delete the route.
      routesToDelete.push_back(routeNode.second);
    }
  }

  // Now, delete whatever routes went from 1 nexthoplist to 0.
  for (std::shared_ptr<Route<AddrT>>& route : routesToDelete) {
    rib->removeRoute(route);
  }
}

void RouteUpdater::removeAllRoutesForClient(RouterID rid, ClientID clientId) {
  removeAllRoutesForClientImpl<IPAddressV4>(getRibV4(rid), clientId);
  removeAllRoutesForClientImpl<IPAddressV6>(getRibV6(rid), clientId);
}

// Some helper functions for recursive weight resolution
// These aren't really usefully reusable, but structuring them
// this way helps with clarifying their meaning.
namespace {
using NextHopForwardInfos =
    boost::container::flat_map<NextHop, RouteNextHopSet>;

struct NextHopCombinedWeightsKey {
  explicit NextHopCombinedWeightsKey(NextHop nhop)
      : ip(nhop.addr()),
        intfId(nhop.intf()), // must be resolved next hop
        action(nhop.labelForwardingAction()) {
    /* "weightless" next hop, consider all attrs of L3 next hop except its
     * weight, this is used in computing number of required paths to next hop,
     * for correct programming of unequal cost multipath */
  }
  bool operator<(const NextHopCombinedWeightsKey& other) const {
    return std::tie(ip, intfId, action) <
        std::tie(other.ip, other.intfId, other.action);
  }
  folly::IPAddress ip;
  InterfaceID intfId;
  std::optional<LabelForwardingAction> action;
};
using NextHopCombinedWeights =
    boost::container::flat_map<NextHopCombinedWeightsKey, NextHopWeight>;

NextHopWeight totalWeightsLcm(const NextHopForwardInfos& nhToFwds) {
  auto lcmAccumFn =
      [](NextHopWeight l,
         std::pair<NextHop, RouteNextHopSet> nhToFwd) -> NextHopWeight {
    auto t = totalWeight(nhToFwd.second);
    return boost::integer::lcm(l, t);
  };
  return std::accumulate(nhToFwds.begin(), nhToFwds.end(), 1, lcmAccumFn);
}

bool hasEcmpNextHop(const NextHopForwardInfos& nhToFwds) {
  for (const auto& nhToFwd : nhToFwds) {
    if (nhToFwd.first.weight() == 0) {
      return true;
    }
  }
  return false;
}

/*
 * In the case that any next hop has weight 0, go through the next hop set
 * of each resolved next hop and bring its next hop set in with weight 0.
 */
RouteNextHopSet mergeForwardInfosEcmp(
    const NextHopForwardInfos& nhToFwds,
    const std::string& routeStr) {
  RouteNextHopSet fwd;
  for (const auto& nhToFwd : nhToFwds) {
    const NextHop& nh = nhToFwd.first;
    const RouteNextHopSet& fw = nhToFwd.second;
    if (nh.weight()) {
      XLOG(WARNING) << "While resolving " << routeStr
                    << " defaulting resolution of weighted next hop " << nh
                    << " to ECMP because another next hop has weight 0";
    }
    for (const auto& fnh : fw) {
      if (fnh.weight()) {
        XLOG(DBG4)
            << "While resolving " << routeStr
            << " defaulting weighted recursively resolved next hop " << fnh
            << " to ECMP because the next hop being resolved has weight 0: "
            << nh;
      }
      fwd.emplace(ResolvedNextHop(
          fnh.addr(), fnh.intf(), ECMP_WEIGHT, fnh.labelForwardingAction()));
    }
  }
  return fwd;
}

/*
 * for each recursively resolved next hop set, combine the next hops into
 * a map from (IP,InterfaceID)->weight, normalizing weights to the scale of
 * the LCM of the total weights of each resolved next hop set. The odd map
 * representation is needed to combine weights for different instances of
 * the same next hop coming from different recursively resolved next hop sets.
 */
NextHopCombinedWeights combineWeights(
    const NextHopForwardInfos& nhToFwds,
    const std::string& routeStr) {
  NextHopCombinedWeights cws;
  NextHopWeight l = totalWeightsLcm(nhToFwds);
  for (const auto& nhToFwd : nhToFwds) {
    const NextHop& unh = nhToFwd.first;
    const RouteNextHopSet& fw = nhToFwd.second;
    if (fw.empty()) {
      continue;
    }
    NextHopWeight normalization;
    NextHopWeight t = totalWeight(fw);
    // If total weight of any resolved next hop set is 0, the LCM will
    // also be 0. This represents the case where one of the recursively
    // resolved routes has ECMP next hops. In this case, we want the
    // ECMP behavior to "propagate up" to this route, so lcm and thus
    // the normalizationg factor being 0 is desirable.
    // We need to check t for 0 for precisely the ecmp
    // recursively resolved next hop sets to not do 0/0.
    normalization = t ? l / t : 0;
    bool loggedEcmpDefault = false;
    for (const auto& fnh : fw) {
      NextHopWeight w = fnh.weight() * normalization * unh.weight();
      // We only want to log this once per recursively resolved next hop
      // to prevent a large ecmp group from spamming pointless logs
      if (!loggedEcmpDefault && !w && unh.weight()) {
        XLOG(WARNING) << "While resolving " << routeStr
                      << " defaulting resolution of weighted next hop " << unh
                      << " to ECMP because another next hop in the resolution"
                      << " tree uses ECMP.";
        loggedEcmpDefault = true;
      }
      cws[NextHopCombinedWeightsKey(fnh)] += w;
    }
  }
  return cws;
}

/*
 * Given a map from (IP,InterfaceID)->weight, take the gcd of the weights
 * and return a next hop set with IP, intf, weight/GCD to minimize the weights
 */
RouteNextHopSet optimizeWeights(const NextHopCombinedWeights& cws) {
  RouteNextHopSet fwd;
  auto gcdAccumFn =
      [](NextHopWeight g,
         const std::pair<NextHopCombinedWeightsKey, NextHopWeight>& cw) {
        NextHopWeight w = cw.second;
        return (g ? boost::integer::gcd(g, w) : w);
      };
  auto fwdWeightGcd = std::accumulate(cws.begin(), cws.end(), 0, gcdAccumFn);
  for (const auto& cw : cws) {
    const folly::IPAddress& addr = cw.first.ip;
    const InterfaceID& intf = cw.first.intfId;
    const auto& action = cw.first.action;
    NextHopWeight w = fwdWeightGcd ? cw.second / fwdWeightGcd : 0;
    fwd.emplace(ResolvedNextHop(addr, intf, w, action));
  }
  return fwd;
}

/*
 * Take the resolved forwarding info (NextHopSet containing ResolvedNextHops)
 * from each recursively resolved next hop and merge them together.
 *
 * There are two cases:
 * 1) Any of the weights of the current routes next hops are 0.
 *    This represents a mix of ECMP and UCMP next hops. In this case,
 *    we default to just ECMP-ing everything from all the next hops.
 * 2) This route's next hops all have weight. In this case, we run through
 *    algorithm to normalize all the weights for the next hops resolved for
 *    each next hop (combineWeights) then minimize the weights
 *    (optimizeWeights). Both of these algorithms will preserve the critical
 *    ratio w_i/w_j for any two weights w_i, w_j of next hops i, j in both
 *    the current route's weights and in the recursively resolved route's
 *    weights.
 *
 * NOTE: case 1) will propagate recursively, so in effect, any ECMP next hop
 *       anywhere in the resolution tree for a route will result in the entire
 *       tree ultimately being treated as an ECMP route.
 *
 * An example to illustrate the requirement:
 * We will represent the recursive route resolution tree for three routes
 * R1, R2, and R3. Numbers on the edges represent weights for next hops,
 * and I1, I2, I3, and I4 are connected interfaces.
 *
 *             R1
 *          3/    2\
 *         R2       R3
 *       5/  4\   3/  2\
 *      I1    I2 I3    I4
 *
 * In resolving R1, resolveOne and getFwdInfoFromNhop will mutually recursively
 * get to a point where we have resolved R2 and R3 to having next hops I1 with
 * weight 5, I2 with weight 4, and I3 with weight 3 and I4 with weight 2
 * respectively: {R2x3:{I1x5, I2x4}, R3x2:{I3x2, I4x2}}
 * We need to preserve the following ratios in the final result:
 * I1/I2=5/4; I3/I4=3/2; (I1+I2)/(I3+I4)=3/2
 * To do this, we normalize the weights by finding the LCM of the total
 * weights of the recursively resolved next hop sets:
 * LCM(9, 5) = 45, then bring in each next hop into the final next hop
 * set with its weight multiplied by LCM/TotalWeight(its next hop)
 * and by the weight of its top level next hop:
 * I1: 5*(45/9)*3=75, I2: 4*(45/9)*3=60, I3: 3*(45/5)*2=54, I4: 2*(45/5)*2=36
 * I1/I2=75/60=5/4, I3/I4=54/36=3/2, (I1+I2)/(I3+I4)=135/90=3/2
 * as required. Finally, we can find the gcds of top level weights to minimize
 * these weights:
 * GCD(75, 60, 54, 36) = 3 so we reach a final next hop set of:
 * {I1x25, I2x20, I3x18, I4x12}
 */
RouteNextHopSet mergeForwardInfos(
    const NextHopForwardInfos& nhToFwds,
    const std::string& routeStr) {
  RouteNextHopSet fwd;
  if (hasEcmpNextHop(nhToFwds)) {
    fwd = mergeForwardInfosEcmp(nhToFwds, routeStr);
  } else {
    NextHopCombinedWeights cws = combineWeights(nhToFwds, routeStr);
    fwd = optimizeWeights(cws);
  }
  return fwd;
}
} // anonymous namespace

template <typename RtRibT, typename AddrT>
void RouteUpdater::getFwdInfoFromNhop(
    RtRibT* nRib,
    ClonedRib* ribCloned,
    const AddrT& nh,
    const std::optional<LabelForwardingAction>& labelAction,
    bool* hasToCpu,
    bool* hasDrop,
    RouteNextHopSet& fwd) {
  auto route = nRib->longestMatch(nh);
  if (route == nullptr) {
    XLOG(DBG3) << " Could not find route for nhop :  " << nh;
    // Un resolvable next hop
    return;
  }
  if (route->needResolve()) {
    resolveOne(route.get(), ribCloned);
  }
  if (route->isResolved()) {
    const auto& fwdInfo = route->getForwardInfo();
    if (fwdInfo.isDrop()) {
      *hasDrop = true;
    } else if (fwdInfo.isToCPU()) {
      *hasToCpu = true;
    } else {
      const auto& nhops = fwdInfo.getNextHopSet();
      // if the route used to resolve the nexthop is directly connected
      if (route->isConnected()) {
        const auto& rtNh = *nhops.begin();
        // NOTE: we need to use a UCMP compatible weight so that this can
        // be a leaf in the recursive resolution defined in the comment
        // describing mergeForwardInfos above.
        fwd.emplace(ResolvedNextHop(
            nh,
            rtNh.intf(),
            UCMP_DEFAULT_WEIGHT,
            LabelForwardingAction::combinePushLabelStack(
                labelAction, rtNh.labelForwardingAction())));
      } else {
        std::for_each(
            nhops.begin(), nhops.end(), [&fwd, labelAction](const auto& nhop) {
              fwd.insert(ResolvedNextHop(
                  nhop.addr(),
                  nhop.intf(),
                  nhop.weight(),
                  LabelForwardingAction::combinePushLabelStack(
                      labelAction, nhop.labelForwardingAction())));
            });
      }
    }
  }
}

template <typename RouteT>
void RouteUpdater::resolveOne(RouteT* route, ClonedRib* ribCloned) {
  // We need to guarantee that route is already cloned before resolve
  CHECK(!route->isPublished());
  // mark this route is in processing. This processing bit shall be cleared
  // in setUnresolvable() or setResolved()
  route->setProcessing();

  bool hasToCpu{false};
  bool hasDrop{false};
  RouteNextHopSet fwd;

  auto bestPair = route->getBestEntry();
  const auto clientId = bestPair.first;
  const auto bestEntry = bestPair.second;
  const auto action = bestEntry->getAction();
  if (action == RouteForwardAction::DROP) {
    hasDrop = true;
  } else if (action == RouteForwardAction::TO_CPU) {
    hasToCpu = true;
  } else {
    NextHopForwardInfos nhToFwds;
    // loop through all nexthops to find out the forward info
    for (const auto& nh : bestEntry->getNextHopSet()) {
      const auto& addr = nh.addr();
      // There are two reasons why InterfaceID is specified in the next hop.
      // 1) The nexthop was generated for interface route.
      //    In this case, the clientId is INTERFACE_ROUTE
      // 2) The nexthop was for v6 link-local address.
      // In both cases, this nexthop is resolved.
      if (nh.intfID().has_value()) {
        // It is either an interface route or v6 link-local
        CHECK(
            clientId == kInterfaceRouteClientId or
            (addr.isV6() and addr.isLinkLocal()));
        nhToFwds[nh].emplace(nh);
        continue;
      }

      // nexthops are a set of IPs
      if (addr.isV4()) {
        auto nRib = ribCloned->v4.rib.get();
        getFwdInfoFromNhop(
            nRib,
            ribCloned,
            nh.addr().asV4(),
            nh.labelForwardingAction(),
            &hasToCpu,
            &hasDrop,
            nhToFwds[nh]);
      } else {
        auto nRib = ribCloned->v6.rib.get();
        getFwdInfoFromNhop(
            nRib,
            ribCloned,
            nh.addr().asV6(),
            nh.labelForwardingAction(),
            &hasToCpu,
            &hasDrop,
            nhToFwds[nh]);
      }
    }
    fwd = mergeForwardInfos(nhToFwds, route->str());
  }

  if (!fwd.empty()) {
    route->setResolved(
        RouteNextHopEntry(std::move(fwd), AdminDistance::MAX_ADMIN_DISTANCE));
    if (clientId == kInterfaceRouteClientId) {
      route->setConnected();
    }
  } else if (hasToCpu) {
    route->setResolved(RouteNextHopEntry(
        RouteForwardAction::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE));
  } else if (hasDrop) {
    route->setResolved(RouteNextHopEntry(
        RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
  } else {
    route->setUnresolvable();
  }

  XLOG(DBG3) << (route->isResolved() ? "Resolved" : "Cannot resolve")
             << " route " << route->str();
}

void RouteUpdater::resolve() {
  // First, we need to make sure every route of the changed rib is cloned
  // and the RadixTree of this changed rib is also generated.
  // The reason why we have to do the same for-loop separately is because the
  // resolveOne needs radixTree_ to find nexthop, so we have to build the tree
  // before resolveOne().
  for (auto& ribCloned : clonedRibs_) {
    if (ribCloned.second.v4.cloned) {
      ribCloned.second.v4.rib->cloneToRadixTreeWithForwardClear();
    }
    if (ribCloned.second.v6.cloned) {
      ribCloned.second.v6.rib->cloneToRadixTreeWithForwardClear();
    }
  }

  // Ideally, just need to resolve the routes that is changed or impacted by
  // the changed routes.
  // However, Without copy-on-write RadixTree, it is O(n) to find out the
  // routes that are changed. In this case, we just simply loop through all
  // routes and resolve those that are not resolved yet.
  // TODO(joseph5wu) T23364375 will try to improve it in the future.
  for (auto& ribCloned : clonedRibs_) {
    if (ribCloned.second.v4.cloned) {
      auto rib = ribCloned.second.v4.rib.get();
      for (auto& routeNode : rib->writableRoutesRadixTree()) {
        if (routeNode.value()->needResolve()) {
          resolveOne(routeNode.value().get(), &ribCloned.second);
        }
      }
    }
    if (ribCloned.second.v6.cloned) {
      auto rib = ribCloned.second.v6.rib.get();
      for (auto& routeNode : rib->writableRoutesRadixTree()) {
        if (routeNode.value()->needResolve()) {
          resolveOne(routeNode.value().get(), &ribCloned.second);
        }
      }
    }
  }
}

std::shared_ptr<RouteTableMap> RouteUpdater::updateDone() {
  // resolve all routes
  resolve();
  RouteTableMap::NodeContainer map;
  bool changed = false;
  for (const auto& ribPair : clonedRibs_) {
    auto id = ribPair.first;
    const auto& rib = ribPair.second;
    changed |= rib.v4.cloned || rib.v6.cloned;
    if (rib.v4.rib->empty() && rib.v6.rib->empty()) {
      continue;
    }

    auto origRt = orig_->getRouteTableIf(ribPair.first);
    if (!rib.v4.cloned && !rib.v6.cloned) {
      CHECK(origRt != nullptr);
      auto iter = map.emplace(id, std::move(origRt));
      CHECK(iter.second);
      continue;
    }
    auto newRt = make_shared<RouteTable>(ribPair.first);
    if (rib.v4.cloned) {
      newRt->setRib(rib.v4.rib);
    } else {
      if (origRt != nullptr) {
        newRt->setRib(origRt->getRibV4());
      }
    }
    if (rib.v6.cloned) {
      newRt->setRib(rib.v6.rib);
    } else {
      if (origRt != nullptr) {
        newRt->setRib(origRt->getRibV6());
      }
    }
    auto iter = map.emplace(id, std::move(newRt));
    CHECK(iter.second);
  }
  if (!changed) {
    return nullptr;
  }
  return deduplicate(&map);
}

void RouteUpdater::addInterfaceAndLinkLocalRoutes(
    const std::shared_ptr<InterfaceMap>& intfs) {
  boost::container::flat_set<RouterID> routers;
  for (auto const& item : intfs->getAllNodes()) {
    const auto& intf = item.second;
    auto routerId = intf->getRouterID();
    routers.insert(routerId);
    for (auto const& addr : intf->getAddresses()) {
      // NOTE: we need to use a UCMP compatible weight so that this can
      // be a leaf in the recursive resolution defined in the comment
      // describing mergeForwardInfos above.
      auto nhop =
          ResolvedNextHop(addr.first, intf->getID(), UCMP_DEFAULT_WEIGHT);
      addRoute(
          routerId,
          addr.first,
          addr.second,
          kInterfaceRouteClientId,
          RouteNextHopEntry(
              std::move(nhop), AdminDistance::DIRECTLY_CONNECTED));
    }
  }
  for (auto id : routers) {
    addLinkLocalRoutes(id);
  }
}

template <typename RibT>
bool RouteUpdater::dedupRoutes(const RibT* oldRib, RibT* newRib) {
  bool isSame = true;
  if (oldRib == newRib) {
    return isSame;
  }
  const auto oldRoutes = oldRib->routes();
  auto& newRoutes = newRib->writableRoutesRadixTree();
  // make sure radixTree_ and nodeMap_ has the same size in newRib
  CHECK_EQ(newRib->size(), newRoutes.size());
  // Copy routes from old route table if they are
  // same. For matching prefixes, which don't have
  // same attributes inherit the generation number
  for (const auto& oldRoute : *oldRoutes) {
    auto newIter = newRoutes.exactMatch(
        oldRoute->prefix().network, oldRoute->prefix().mask);
    if (newIter == newRoutes.end()) {
      isSame = false;
      continue;
    }
    auto& newRoute = newIter->value();
    if (oldRoute->isSame(newRoute.get())) {
      // both routes are completely same, instead of using the new route,
      // we re-use the old route. First update oldRoute to radixTree_
      newIter->value() = oldRoute;
      // we also need to update the nodeMap_
      newRib->updateRoute(oldRoute);
    } else {
      isSame = false;
      newRoute->inheritGeneration(*oldRoute);
      newRib->updateRoute(newRoute);
    }
  }
  if (newRoutes.size() != oldRoutes->size()) {
    isSame = false;
  } else {
    // If sizes are same we would have already caught any
    // difference while looking up all routes from oldRoutes
    // in newRoutes, so do nothing
  }
  // make sure after change nodeMap_ and radixTree_ size still match
  CHECK_EQ(newRib->size(), newRoutes.size());
  return isSame;
}

std::shared_ptr<RouteTableMap> RouteUpdater::deduplicate(
    RouteTableMap::NodeContainer* newTables) {
  bool allSame = true;
  const auto* oldTables = &orig_->getAllNodes();
  auto oldIter = oldTables->begin();
  auto newIter = newTables->begin();
  while (oldIter != oldTables->end() && newIter != newTables->end()) {
    const auto oldVrf = oldIter->first;
    const auto newVrf = newIter->first;
    if (oldVrf < newVrf) {
      allSame = false;
      oldIter++;
      continue;
    }
    if (oldVrf > newVrf) {
      allSame = false;
      newIter++;
      continue;
    }
    bool isSame = true;
    const auto& oldTable = oldIter->second;
    auto& newTable = newIter->second;
    if (oldTable != newTable) {
      // Handle V4 RIB
      const auto& oldV4 = oldTable->getRibV4();
      auto& newV4 = newTable->writableRibV4();
      if (dedupRoutes(oldV4.get(), newV4.get())) {
        newTable->setRib(oldV4);
      } else {
        isSame = false;
        newV4->inheritGeneration(*oldV4);
      }
      // Handle get V6 RIB
      const auto& oldV6 = oldTable->getRibV6();
      auto& newV6 = newTable->writableRibV6();
      if (dedupRoutes(oldV6.get(), newV6.get())) {
        newTable->setRib(oldV6);
      } else {
        isSame = false;
        newV6->inheritGeneration(*oldV6);
      }
    }
    // if both v4 RIB and v6 rib from the new RouteTable are as same as the
    // old one, we will just reuse the old RouteTable
    if (isSame) {
      newIter->second = oldTable;
    } else {
      allSame = false;
      newTable->inheritGeneration(*oldTable);
    }
    oldIter++;
    newIter++;
  }
  if (oldIter != oldTables->end() || newIter != newTables->end()) {
    allSame = false;
  }
  if (allSame) {
    return nullptr;
  }
  return orig_->clone(*newTables);
}

} // namespace facebook::fboss
