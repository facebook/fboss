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

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/integer/common_factor.hpp>
#include <folly/logging/xlog.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/Route.h"

#include "fboss/agent/Utils.h"

using boost::container::flat_map;
using boost::container::flat_set;
using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss::rib {

static const PrefixV6 kIPv6LinkLocalPrefix{folly::IPAddressV6("fe80::"), 64};
static const auto kInterfaceRouteClientId = ClientID::INTERFACE_ROUTE;

RouteUpdater::RouteUpdater(
    IPv4NetworkToRouteMap* v4Routes,
    IPv6NetworkToRouteMap* v6Routes)
    : v4Routes_(v4Routes), v6Routes_(v6Routes) {}

template <typename AddressT>
void RouteUpdater::addRouteImpl(
    const Prefix<AddressT>& prefix,
    NetworkToRouteMap<AddressT>* routes,
    ClientID clientID,
    RouteNextHopEntry entry) {
  auto it = routes->exactMatch(prefix.network, prefix.mask);

  if (it != routes->end()) {
    Route<AddressT>* route = &(it->value());
    if (route->has(clientID, entry)) {
      return;
    }

    route->update(clientID, entry);
    return;
  }

  CHECK(it == routes->end());
  routes->insert(
      prefix.network, prefix.mask, Route<AddressT>(prefix, clientID, entry));
}

void RouteUpdater::addRoute(
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientID,
    RouteNextHopEntry entry) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    addRouteImpl(prefix, v4Routes_, clientID, std::move(entry));
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    if (prefix.network.isLinkLocal()) {
      XLOG(DBG2) << "Ignoring v6 link-local interface route: " << prefix.str();
      return;
    }
    addRouteImpl(prefix, v6Routes_, clientID, std::move(entry));
  }
}

void RouteUpdater::addInterfaceRoute(
    const folly::IPAddress& network,
    uint8_t mask,
    const folly::IPAddress& address,
    InterfaceID interface) {
  ResolvedNextHop resolvedNextHop(address, interface, UCMP_DEFAULT_WEIGHT);
  RouteNextHopEntry nextHop(resolvedNextHop, AdminDistance::DIRECTLY_CONNECTED);

  addRoute(network, mask, ClientID::INTERFACE_ROUTE, nextHop);
}

void RouteUpdater::addLinkLocalRoutes() {
  // 169.254/16 is treated as link-local only by convention. Like other vendors,
  // we choose to route 169.254/16.
  addRouteImpl(
      kIPv6LinkLocalPrefix,
      v6Routes_,
      ClientID::LINKLOCAL_ROUTE,
      RouteNextHopEntry(
          RouteForwardAction::TO_CPU, AdminDistance::DIRECTLY_CONNECTED));
}

void RouteUpdater::delLinkLocalRoutes() {
  delRouteImpl(kIPv6LinkLocalPrefix, v6Routes_, ClientID::LINKLOCAL_ROUTE);
}

template <typename AddressT>
void RouteUpdater::delRouteImpl(
    const Prefix<AddressT>& prefix,
    NetworkToRouteMap<AddressT>* routes,
    ClientID clientID) {
  auto it = routes->exactMatch(prefix.network, prefix.mask);
  if (it == routes->end()) {
    XLOG(DBG3) << "Failed to delete route: " << prefix.str()
               << " does not exist";
    return;
  }

  Route<AddressT>& route = it->value();
  route.delEntryForClient(clientID);

  XLOG(DBG3) << "Deleted next-hops for prefix " << prefix.str()
             << "from client " << folly::to<std::string>(clientID);

  if (route.hasNoEntry()) {
    XLOG(DBG3) << "...and then deleted route " << route.str();
    routes->erase(it);
  }
}

void RouteUpdater::delRoute(
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientID) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    delRouteImpl(prefix, v4Routes_, clientID);
  } else {
    CHECK(network.isV6());
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    delRouteImpl(prefix, v6Routes_, clientID);
  }
}

template <typename AddressT>
void RouteUpdater::removeAllRoutesFromClientImpl(
    NetworkToRouteMap<AddressT>* routes,
    ClientID clientID) {
  std::vector<typename NetworkToRouteMap<AddressT>::Iterator> toDelete;

  for (auto it = routes->begin(); it != routes->end(); ++it) {
    auto& route = it->value();
    route.delEntryForClient(clientID);
    if (route.hasNoEntry()) {
      // The nexthops we removed was the only one.  Delete the route.
      toDelete.push_back(it);
    }
  }

  // Now, delete whatever routes went from 1 nexthoplist to 0.
  for (auto it : toDelete) {
    routes->erase(it);
  }
}

void RouteUpdater::removeAllRoutesForClient(ClientID clientID) {
  removeAllRoutesFromClientImpl<IPAddressV4>(v4Routes_, clientID);
  removeAllRoutesFromClientImpl<IPAddressV6>(v6Routes_, clientID);
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

template <typename AddressT>
void RouteUpdater::getFwdInfoFromNhop(
    NetworkToRouteMap<AddressT>* routes,
    const AddressT& nh,
    const std::optional<LabelForwardingAction>& labelAction,
    bool* hasToCpu,
    bool* hasDrop,
    RouteNextHopSet& fwd) {
  auto it = routes->longestMatch(nh, nh.bitCount());
  if (it == routes->end()) {
    XLOG(DBG3) << "Could not find subnet for next-hop:  " << nh;
    // Unresolvable next hop
    return;
  }

  Route<AddressT>* route = &(it->value());
  CHECK(route);

  if (route->needResolve()) {
    resolveOne(route);
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

template <typename AddressT>
void RouteUpdater::resolveOne(Route<AddressT>* route) {
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

      if (addr.isV4()) {
        getFwdInfoFromNhop(
            v4Routes_,
            nh.addr().asV4(),
            nh.labelForwardingAction(),
            &hasToCpu,
            &hasDrop,
            nhToFwds[nh]);
      } else {
        CHECK(addr.isV6());
        getFwdInfoFromNhop(
            v6Routes_,
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

template <typename AddressT>
void RouteUpdater::resolve(NetworkToRouteMap<AddressT>* routes) {
  for (auto& entry : *routes) {
    Route<AddressT>* route = &(entry.value());
    if (route->needResolve()) {
      resolveOne(route);
    }
  }
}

template <typename AddressT>
void RouteUpdater::updateDoneImpl(NetworkToRouteMap<AddressT>* routes) {
  for (auto& entry : *routes) {
    Route<AddressT>& route = entry.value();
    route.clearForward();
  }
  resolve(routes);
}

void RouteUpdater::updateDone() {
  updateDoneImpl(v4Routes_);
  updateDoneImpl(v6Routes_);
}

} // namespace facebook::fboss::rib
