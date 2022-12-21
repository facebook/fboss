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

#include "fboss/agent/types.h"

#include "fboss/agent/rib/NetworkToRouteMap.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

/**
 * Expected behavior of RibRouteUpdater::resolve():
 *
 * RibRouteUpdater::resolve() resolves the route table forwarding information
 * based on the RIB, by doing recursively route table lookup. At the end of
 * the process, every route will be either unresolved or resolved with a
 * ECMP group.
 *
 * There are clear expectation on resolving FIB for a route, when all
 * nexthops are resolved to actual IPs. However, if is not clearly
 * defined and documented expectation if ECMP group has mix of action(s)
 * (i.e. DROP, TO_CPU) and IP nexthops.
 *
 * The following is the current implentation of resolve():
 * 1. No weighted ECMP. Each entry in the ECMP group is unique and has equal
 *    weight.
 * 2. A ECMP group could have either DROP, TO_CPU, or a set of IP nexthops.
 * 3. If DROP and other types (i.e. TO_CPU and IP nexthops) are part of the
 *    results of route resolve process. The finally FIB will be DROP.
 * 4. If TO_CPU and IP nexthops are part of the results of resolving process,
 *    only IP nexthops will be in the final ECMP group.
 * 5. If and only if TO_CPU is the only nexthop (directly or indirectly) of
 *    a route, TO_CPU action will be only path in the resolved ECMP group.
 */
class RibRouteUpdater {
 public:
  RibRouteUpdater(
      IPv4NetworkToRouteMap* v4Routes,
      IPv6NetworkToRouteMap* v6Routes);

  RibRouteUpdater(
      IPv4NetworkToRouteMap* v4Routes,
      IPv6NetworkToRouteMap* v6Routes,
      LabelToRouteMap* mplsRoutes);

  struct RouteEntry {
    folly::CIDRNetwork prefix;
    RouteNextHopEntry nhopEntry;
    RouteEntry(
        folly::CIDRNetwork inPrefix,
        const RouteNextHopEntry& inNhopEntry)
        : prefix(std::move(inPrefix)), nhopEntry(inNhopEntry.toThrift()) {}
    RouteEntry(const RouteEntry& other)
        : prefix(other.prefix), nhopEntry(other.nhopEntry.toThrift()) {}
  };

  struct MplsRouteEntry {
    LabelID label;
    RouteNextHopEntry nhopEntry;
    MplsRouteEntry(LabelID inLabel, const RouteNextHopEntry& inNhopEntry)
        : label(inLabel), nhopEntry(inNhopEntry.toThrift()) {}
    MplsRouteEntry(const MplsRouteEntry& other)
        : label(other.label), nhopEntry(other.nhopEntry.toThrift()) {}
  };

  /*
   * Update routes for a clients and trigger
   * resolution
   */
  template <typename RouteType, typename RouteIdType>
  void update(
      ClientID client,
      const std::vector<RouteType>& toAdd,
      const std::vector<RouteIdType>& toDel,
      bool resetClientsRoutes) {
    updateImpl(client, toAdd, toDel, resetClientsRoutes);
    updateDone();
  }
  /*
   * Update routes for multiple clients and trigger
   * resolution
   */

  void update(
      const std::map<ClientID, std::vector<RouteEntry>>& toAdd,
      const std::map<ClientID, std::vector<folly::CIDRNetwork>>& toDel,
      const std::set<ClientID>& resetClientsRoutesFor);

 private:
  void updateImpl(
      ClientID client,
      const std::vector<RouteEntry>& toAdd,
      const std::vector<folly::CIDRNetwork>& toDel,
      bool resetClientsRoutes);
  void updateImpl(
      ClientID client,
      const std::vector<MplsRouteEntry>& toAdd,
      const std::vector<LabelID>& toDel,
      bool resetClientsRoutes);
  void addOrReplaceRoute(
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientID,
      const RouteNextHopEntry& entry);
  void addOrReplaceRoute(
      LabelID label,
      ClientID clientID,
      const RouteNextHopEntry& entry);
  void updateDone();

  void
  delRoute(const folly::IPAddress& network, uint8_t mask, ClientID clientID);
  void delRoute(const LabelID& label, const ClientID clientID);
  void removeAllRoutesForClient(ClientID clientID);
  void removeAllMplsRoutesForClient(ClientID clientID);

  template <typename AddressT>
  using Prefix = RoutePrefix<AddressT>;

  template <typename AddressT>
  void addOrReplaceRouteImpl(
      const Prefix<AddressT>& prefix,
      NetworkToRouteMap<AddressT>* routes,
      ClientID clientID,
      const RouteNextHopEntry& entry);
  template <typename AddressT>
  void delRouteImpl(
      const Prefix<AddressT>& prefix,
      NetworkToRouteMap<AddressT>* routes,
      ClientID clientID);
  template <typename AddressT>
  void removeAllRoutesFromClientImpl(
      NetworkToRouteMap<AddressT>* routes,
      ClientID clientID);

  template <typename AddressT>
  void resolve(NetworkToRouteMap<AddressT>* routes);

  template <typename AddressT>
  std::shared_ptr<Route<AddressT>> resolveOne(
      typename NetworkToRouteMap<AddressT>::Iterator ritr);

  template <typename AddressT>
  std::shared_ptr<Route<AddressT>> writableRoute(
      typename NetworkToRouteMap<AddressT>::Iterator ritr);

  template <typename AddressT>
  std::shared_ptr<Route<AddressT>> writableRoute(
      std::shared_ptr<Route<AddressT>> route);

  template <typename AddressT>
  void getFwdInfoFromNhop(
      NetworkToRouteMap<AddressT>* routes,
      const AddressT& nh,
      const std::optional<LabelForwardingAction>& labelAction,
      bool* hasToCpu,
      bool* hasDrop,
      RouteNextHopSet& fwd);

  template <typename AddressT>
  bool needResolve(const std::shared_ptr<Route<AddressT>>& route) const;

  using NextHopIpToForwardInfo =
      std::unordered_map<folly::IPAddress, RouteNextHopSet>;

  IPv4NetworkToRouteMap* v4Routes_{nullptr};
  IPv6NetworkToRouteMap* v6Routes_{nullptr};
  LabelToRouteMap* mplsRoutes_{nullptr};
  std::unordered_set<void*> needsResolution_;
  /*
   * Cache for next hop to FWD informatio. For our use case
   * its pretty common for the same next hops to repeat, so
   * cache resolution
   */
  std::map<RouteNextHopSet, RouteNextHopSet> unresolvedToResolvedNhops_;
};

} // namespace facebook::fboss
