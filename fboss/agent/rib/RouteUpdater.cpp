/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/rib/RouteUpdater.h"

#include <numeric>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/integer/common_factor.hpp>
#include <folly/logging/xlog.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Route.h"

#include <algorithm>
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTypes.h"
#include "folly/IPAddressV4.h"

using boost::container::flat_map;
using boost::container::flat_set;
using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

static const RoutePrefixV6 kIPv6LinkLocalPrefix{
    folly::IPAddressV6("fe80::"),
    64};
static const auto kInterfaceRouteClientId = ClientID::INTERFACE_ROUTE;

RibRouteUpdater::RibRouteUpdater(
    IPv4NetworkToRouteMap* v4Routes,
    IPv6NetworkToRouteMap* v6Routes)
    : v4Routes_(v4Routes), v6Routes_(v6Routes) {}

RibRouteUpdater::RibRouteUpdater(
    IPv4NetworkToRouteMap* v4Routes,
    IPv6NetworkToRouteMap* v6Routes,
    LabelToRouteMap* mplsRoutes)
    : v4Routes_(v4Routes), v6Routes_(v6Routes), mplsRoutes_(mplsRoutes) {}

void RibRouteUpdater::update(
    const std::map<ClientID, std::vector<RouteEntry>>& toAdd,
    const std::map<ClientID, std::vector<folly::CIDRNetwork>>& toDel,
    const std::set<ClientID>& resetClientsRoutesFor) {
  std::set<ClientID> clients;
  std::for_each(toAdd.begin(), toAdd.end(), [&clients](const auto& entry) {
    clients.insert(entry.first);
  });
  std::for_each(toDel.begin(), toDel.end(), [&clients](const auto& entry) {
    clients.insert(entry.first);
  });
  clients.insert(resetClientsRoutesFor.begin(), resetClientsRoutesFor.end());
  for (auto client : clients) {
    auto addItr = toAdd.find(client);
    auto delItr = toDel.find(client);
    updateImpl(
        client,
        (addItr == toAdd.end() ? std::vector<RouteEntry>() : addItr->second),
        (delItr == toDel.end() ? std::vector<folly::CIDRNetwork>()
                               : delItr->second),
        resetClientsRoutesFor.find(client) != resetClientsRoutesFor.end());
  }
  updateDone();
}

void RibRouteUpdater::updateImpl(
    ClientID client,
    const std::vector<RouteEntry>& toAdd,
    const std::vector<folly::CIDRNetwork>& toDel,
    bool resetClientsRoutes) {
  if (resetClientsRoutes) {
    removeAllRoutesForClient(client);
  }
  std::for_each(
      toAdd.begin(), toAdd.end(), [this, client](const auto& routeEntry) {
        addOrReplaceRoute(
            routeEntry.prefix.first,
            routeEntry.prefix.second,
            client,
            routeEntry.nhopEntry);
      });
  std::for_each(toDel.begin(), toDel.end(), [this, client](const auto& prefix) {
    delRoute(prefix.first, prefix.second, client);
  });
}

void RibRouteUpdater::updateImpl(
    ClientID client,
    const std::vector<MplsRouteEntry>& toAdd,
    const std::vector<LabelID>& toDel,
    bool resetClientsRoutes) {
  if (resetClientsRoutes) {
    removeAllMplsRoutesForClient(client);
  }
  std::for_each(
      toAdd.begin(), toAdd.end(), [this, client](const auto& routeEntry) {
        addOrReplaceRoute(routeEntry.label, client, routeEntry.nhopEntry);
      });
  std::for_each(toDel.begin(), toDel.end(), [this, client](const auto& label) {
    delRoute(label, client);
  });
}

template <typename AddressT>
void RibRouteUpdater::addOrReplaceRouteImpl(
    const Prefix<AddressT>& prefix,
    NetworkToRouteMap<AddressT>* routes,
    ClientID clientID,
    const RouteNextHopEntry& entry) {
  auto it = routes->exactMatch(prefix.network(), prefix.mask());

  if (it != routes->end()) {
    auto route = it->value();
    auto existingRouteForClient = route->getEntryForClient(clientID);
    if (!existingRouteForClient || !(*existingRouteForClient == entry)) {
      route = writableRoute<AddressT>(it);
      route->update(clientID, entry);
    }
    return;
  }

  routes->insert(
      prefix, std::make_shared<Route<AddressT>>(prefix, clientID, entry));
}

void RibRouteUpdater::addOrReplaceRoute(
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientID,
    const RouteNextHopEntry& entry) {
  if (network.isV4()) {
    RoutePrefixV4 prefix{network.asV4().mask(mask), mask};
    addOrReplaceRouteImpl(prefix, v4Routes_, clientID, entry);
  } else {
    RoutePrefixV6 prefix{network.asV6().mask(mask), mask};
    addOrReplaceRouteImpl(prefix, v6Routes_, clientID, entry);
  }
}

void RibRouteUpdater::addOrReplaceRoute(
    LabelID label,
    ClientID clientID,
    const RouteNextHopEntry& entry) {
  XLOG(DBG3) << "Add mpls route for label " << label << " nh " << entry.str();
  auto iter = mplsRoutes_->find(label);
  if (iter == mplsRoutes_->end()) {
    mplsRoutes_->emplace(std::make_pair(
        label,
        std::make_shared<Route<LabelID>>(
            Route<LabelID>::makeThrift(label, clientID, entry))));
  } else {
    auto& route = iter->second;
    auto existingRouteForClient = route->getEntryForClient(clientID);
    if (!existingRouteForClient || !(*existingRouteForClient == entry)) {
      route = writableRoute<LabelID>(route);
      route->update(clientID, entry);
    }
  }
}

template <typename AddressT>
void RibRouteUpdater::delRouteImpl(
    const Prefix<AddressT>& prefix,
    NetworkToRouteMap<AddressT>* routes,
    ClientID clientID) {
  auto it = routes->exactMatch(prefix.network(), prefix.mask());
  if (it == routes->end()) {
    XLOG(DBG3) << "Failed to delete route: " << prefix.str()
               << " does not exist";
    return;
  }

  auto& route = it->value();
  auto clientNhopEntry = route->getEntryForClient(clientID);
  if (!clientNhopEntry) {
    return;
  }
  if (route->numClientEntries() == 1) {
    // If this client's the only entry, simply erase
    XLOG(DBG3) << "Deleting route: " << route->str();
    routes->erase(it);
  } else {
    route = writableRoute<AddressT>(it);
    route->delEntryForClient(clientID);

    XLOG(DBG3) << "Deleted next-hops for prefix " << prefix.str()
               << "from client " << folly::to<std::string>(clientID);
  }
}

void RibRouteUpdater::delRoute(
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientID) {
  if (network.isV4()) {
    RoutePrefixV4 prefix{network.asV4().mask(mask), mask};
    delRouteImpl(prefix, v4Routes_, clientID);
  } else {
    CHECK(network.isV6());
    RoutePrefixV6 prefix{network.asV6().mask(mask), mask};
    delRouteImpl(prefix, v6Routes_, clientID);
  }
}

void RibRouteUpdater::delRoute(const LabelID& label, const ClientID clientID) {
  XLOG(DBG3) << "Delete mpls route for " << label;
  auto it = mplsRoutes_->find(label);
  if (it == mplsRoutes_->end()) {
    XLOG(DBG3) << "Failed to delete mpls route: label " << label
               << " does not exist";
    return;
  }

  auto& route = it->second;
  auto clientNhopEntry = route->getEntryForClient(clientID);
  if (!clientNhopEntry) {
    return;
  }
  if (route->numClientEntries() == 1) {
    // If this client's the only entry, simply erase
    XLOG(DBG3) << "Deleting route: " << route->str();
    mplsRoutes_->erase(it);
  } else {
    route = writableRoute<LabelID>(route);
    route->delEntryForClient(clientID);

    XLOG(DBG3) << "Deleted next-hops for label " << label << "from client "
               << folly::to<std::string>(clientID);
  }
}

template <typename AddressT>
void RibRouteUpdater::removeAllRoutesFromClientImpl(
    NetworkToRouteMap<AddressT>* routes,
    ClientID clientID) {
  std::vector<typename NetworkToRouteMap<AddressT>::Iterator> toDelete;

  for (auto it = routes->begin(); it != routes->end(); ++it) {
    auto route = value<AddressT>(it);
    auto nhopEntry = route->getEntryForClient(clientID);
    if (!nhopEntry) {
      continue;
    }
    if (route->numClientEntries() == 1) {
      // This client's is the only entry avoid unnecessary cloning
      // we are going to prune the route anyways
      toDelete.push_back(it);
    } else {
      route = writableRoute<AddressT>(it);
      route->delEntryForClient(clientID);
      if (route->hasNoEntry()) {
        // The nexthops we removed was the only one.  Delete the route->
        toDelete.push_back(it);
      }
    }
  }

  // Now, delete whatever routes went from 1 nexthoplist to 0.
  for (auto it : toDelete) {
    routes->erase(it);
  }
}

void RibRouteUpdater::removeAllRoutesForClient(ClientID clientID) {
  removeAllRoutesFromClientImpl<IPAddressV4>(v4Routes_, clientID);
  removeAllRoutesFromClientImpl<IPAddressV6>(v6Routes_, clientID);
}

void RibRouteUpdater::removeAllMplsRoutesForClient(ClientID clientID) {
  removeAllRoutesFromClientImpl<LabelID>(mplsRoutes_, clientID);
}

// Some helper functions for recursive weight resolution
// These aren't really usefully reusable, but structuring them
// this way helps with clarifying their meaning.
namespace {
using NextHopForwardInfos = std::map<NextHop, RouteNextHopSet>;

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
template <typename AddressT>
RouteNextHopSet mergeForwardInfosEcmp(
    const NextHopForwardInfos& nhToFwds,
    const std::shared_ptr<Route<AddressT>>& route) {
  RouteNextHopSet fwd;
  for (const auto& nhToFwd : nhToFwds) {
    const NextHop& nh = nhToFwd.first;
    const RouteNextHopSet& fw = nhToFwd.second;
    if (nh.weight()) {
      XLOG(WARNING) << "While resolving " << route->str()
                    << " defaulting resolution of weighted next hop " << nh
                    << " to ECMP because another next hop has weight 0";
    }
    for (const auto& fnh : fw) {
      if (fnh.weight()) {
        XLOG(DBG4)
            << "While resolving " << route->str()
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
template <typename AddressT>
NextHopCombinedWeights combineWeights(
    const NextHopForwardInfos& nhToFwds,
    const std::shared_ptr<Route<AddressT>>& route) {
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
        XLOG(WARNING) << "While resolving " << route->str()
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
template <typename AddressT>
RouteNextHopSet mergeForwardInfos(
    const NextHopForwardInfos& nhToFwds,
    const std::shared_ptr<Route<AddressT>>& route) {
  RouteNextHopSet fwd;
  if (hasEcmpNextHop(nhToFwds)) {
    fwd = mergeForwardInfosEcmp(nhToFwds, route);
  } else {
    NextHopCombinedWeights cws = combineWeights(nhToFwds, route);
    fwd = optimizeWeights(cws);
  }
  return fwd;
}
} // anonymous namespace

template <typename AddressT>
void RibRouteUpdater::getFwdInfoFromNhop(
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

  auto route = it->value();
  CHECK(route);

  if (needResolve(route)) {
    route = resolveOne<AddressT>(it);
    CHECK(route);
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
std::shared_ptr<Route<AddressT>> RibRouteUpdater::resolveOne(
    typename NetworkToRouteMap<AddressT>::Iterator ritr) {
  auto route = value<AddressT>(ritr);
  // Starting resolution for this route, remove from resolution queue
  needsResolution_.erase(route.get());

  bool hasToCpu{false};
  bool hasDrop{false};
  RouteNextHopSet* fwd{nullptr};

  auto bestPair = route->getBestEntry();
  const auto clientId = bestPair.first;
  const auto& bestEntryVal = *bestPair.second;
  const auto bestEntry = &bestEntryVal;
  const auto action = bestEntry->getAction();
  const auto counterID = bestEntry->getCounterID();
  const auto classID = bestEntry->getClassID();
  if (action == RouteForwardAction::DROP) {
    hasDrop = true;
  } else if (action == RouteForwardAction::TO_CPU) {
    hasToCpu = true;
  } else {
    auto fwItr = unresolvedToResolvedNhops_.find(bestEntry->getNextHopSet());
    if (fwItr == unresolvedToResolvedNhops_.end()) {
      NextHopForwardInfos nhToFwds;
      bool labelPopandLookup = false;
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

        // For pop and lookup, forwarding is based on inner
        // header. There should be only one nhop in this case.
        if (nh.labelForwardingAction().has_value() &&
            nh.labelForwardingAction().value().type() ==
                MplsActionCode::POP_AND_LOOKUP) {
          if (bestEntry->getNextHopSet().size() > 1) {
            throw FbossError(
                "MPLS pop and lookup forwarding action has more than one nexthop");
          }
          labelPopandLookup = true;
          nhToFwds[nh].emplace(nh);
          break;
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

      // For label pop and lookup, the label lookup result instructs
      // the hw to perform another lookup on inner header and
      // forward packet based on inner header result. This means
      // that label pop and lookup will not have a valid nhop ip
      // or interface and any merge operation has to be skipped.
      RouteNextHopSet nhSet = labelPopandLookup
          ? bestEntry->getNextHopSet()
          : mergeForwardInfos(nhToFwds, route);

      fwItr = unresolvedToResolvedNhops_
                  .insert({bestEntry->getNextHopSet(), std::move(nhSet)})
                  .first;
    }
    fwd = &(fwItr->second);
  }

  std::shared_ptr<Route<AddressT>> updatedRoute;
  auto updateRoute = [this, clientId, &updatedRoute, classID, &route](
                         typename NetworkToRouteMap<AddressT>::Iterator ritr,
                         std::optional<RouteNextHopEntry> nhop) {
    updatedRoute = writableRoute<AddressT>(ritr);
    if (nhop) {
      updatedRoute->setResolved(*nhop);
      if (clientId == kInterfaceRouteClientId &&
          !nhop->getNextHopSet().empty()) {
        updatedRoute->setConnected();
      }
    } else {
      updatedRoute->setUnresolvable();
    }
    updatedRoute->updateClassID(classID);
    updatedRoute->publish();
    XLOG(DBG3) << (updatedRoute->isResolved() ? "Resolved" : "Cannot resolve")
               << " route " << updatedRoute->str();
  };
  if (fwd && !fwd->empty()) {
    if (route->getForwardInfo().getNextHopSet() != *fwd ||
        route->getForwardInfo().getCounterID() != counterID ||
        route->getForwardInfo().getClassID() != classID) {
      updateRoute(
          ritr,
          std::make_optional<RouteNextHopEntry>(
              *fwd, bestEntry->getAdminDistance(), counterID, classID));
    }
  } else if (hasToCpu) {
    if (!route->isToCPU() ||
        route->getForwardInfo().getCounterID() != counterID ||
        route->getForwardInfo().getClassID() != classID) {
      updateRoute(
          ritr,
          std::make_optional<RouteNextHopEntry>(
              RouteForwardAction::TO_CPU,
              AdminDistance::MAX_ADMIN_DISTANCE,
              counterID,
              classID));
    }
  } else if (hasDrop) {
    if (!route->isDrop() ||
        route->getForwardInfo().getCounterID() != counterID ||
        route->getForwardInfo().getClassID() != classID) {
      updateRoute(
          ritr,
          std::make_optional<RouteNextHopEntry>(
              RouteForwardAction::DROP,
              AdminDistance::MAX_ADMIN_DISTANCE,
              counterID,
              classID));
    }
  } else {
    updateRoute(ritr, std::nullopt);
  }
  if (!updatedRoute) {
    route->publish();
    XLOG(DBG3) << " Retained resolution :"
               << (route->isResolved() ? "Resolved" : "Cannot resolve")
               << " route " << route->str();
  }
  return updatedRoute ? updatedRoute : route;
}

template <typename AddressT>
std::shared_ptr<Route<AddressT>> RibRouteUpdater::writableRoute(
    typename NetworkToRouteMap<AddressT>::Iterator ritr) {
  if (value<AddressT>(ritr)->isPublished()) {
    value<AddressT>(ritr) = value<AddressT>(ritr)->clone();
  }
  return value<AddressT>(ritr);
}

template <typename AddressT>
std::shared_ptr<Route<AddressT>> RibRouteUpdater::writableRoute(
    std::shared_ptr<Route<AddressT>> route) {
  if (route->isPublished()) {
    route = route->clone();
  }
  return route;
}

template <typename AddressT>
void RibRouteUpdater::resolve(NetworkToRouteMap<AddressT>* routes) {
  for (auto ritr = routes->begin(); ritr != routes->end(); ++ritr) {
    if (needResolve(value(*ritr))) {
      resolveOne<AddressT>(ritr);
    }
  }
}

template <typename AddressT>
bool RibRouteUpdater::needResolve(
    const std::shared_ptr<Route<AddressT>>& route) const {
  return needsResolution_.find(route.get()) != needsResolution_.end();
}

void RibRouteUpdater::updateDone() {
  // Record all routes as needing resolution
  auto markForResolution = [this](const auto& routes) {
    std::for_each(routes->begin(), routes->end(), [this](auto& route) {
      needsResolution_.insert(value(route).get());
    });
  };
  markForResolution(v4Routes_);
  markForResolution(v6Routes_);
  if (mplsRoutes_) {
    markForResolution(mplsRoutes_);
  }
  SCOPE_EXIT {
    needsResolution_.clear();
    unresolvedToResolvedNhops_.clear();
  };
  resolve(v4Routes_);
  resolve(v6Routes_);
  if (mplsRoutes_) {
    resolve(mplsRoutes_);
  }
}
} // namespace facebook::fboss
