// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>

namespace facebook::fboss {

class SwSwitch;
class SwitchState;
class Interface;

/**
 * NextHopResolver provides common IP resolution functionality for
 * resolving destination IPs to next hop ports and MAC addresses.
 *
 * This class extracts reusable resolution logic that can be leveraged
 * by both MirrorManager (for SPAN/ERSPAN mirrors) and TamManager
 * (for Mirror on Drop collector resolution).
 *
 * The resolution follows these steps:
 * 1. Route lookup: Find the route to the destination IP via longest prefix
 * match
 * 2. Next hop resolution: Extract the set of next hops from the route
 * 3. Neighbor resolution: Look up the ARP/NDP entry for the next hop
 * 4. Egress port resolution: Determine the physical port (handling LAGs)
 */
template <typename AddrT>
class NextHopResolver {
 public:
  using NeighborEntryT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpEntry,
      NdpEntry>;

  explicit NextHopResolver(SwSwitch* sw);
  ~NextHopResolver() = default;

  /**
   * Resolve the destination IP to a set of next hops via route lookup.
   *
   * @param state The current switch state
   * @param destinationIp The IP address to resolve
   * @param routerId The router ID for the route lookup (default: 0)
   * @return The set of next hops, or empty set if no route exists
   */
  RouteNextHopEntry::NextHopSet resolveNextHops(
      const std::shared_ptr<SwitchState>& state,
      const AddrT& destinationIp,
      const RouterID& routerId = RouterID(0)) const;

  /**
   * Resolve a next hop to its neighbor entry via ARP/NDP lookup.
   *
   * This handles both directly connected destinations and multi-hop
   * destinations. For directly connected destinations, it looks up
   * the destination IP directly. For multi-hop destinations, it looks
   * up the next hop IP.
   *
   * @param state The current switch state
   * @param destinationIp The final destination IP address
   * @param nexthop The next hop to resolve
   * @return The neighbor entry, or nullptr if not resolved
   */
  std::shared_ptr<NeighborEntryT> resolveNextHopNeighbor(
      const std::shared_ptr<SwitchState>& state,
      const AddrT& destinationIp,
      const NextHop& nexthop) const;

  /**
   * Resolve the egress port from a neighbor entry.
   *
   * This handles different port types:
   * - Physical ports: returned directly
   * - System ports: returned directly
   * - Aggregate ports (LAGs): returns the first forwarding member port
   *
   * @param state The current switch state
   * @param neighbor The neighbor entry containing the port
   * @return The resolved port descriptor, or nullopt if unresolved
   */
  std::optional<PortDescriptor> resolveEgressPort(
      const std::shared_ptr<SwitchState>& state,
      const std::shared_ptr<NeighborEntryT>& neighbor) const;

  /**
   * Get the interface for a given interface ID, checking both local
   * and remote interfaces.
   *
   * @param state The current switch state
   * @param interfaceId The interface ID to look up
   * @return The interface, or nullptr if not found
   */
  std::shared_ptr<Interface> getInterface(
      const std::shared_ptr<SwitchState>& state,
      InterfaceID interfaceId) const;

 private:
  template <typename ADDRT = AddrT>
  typename std::
      enable_if<std::is_same<ADDRT, folly::IPAddressV4>::value, ADDRT>::type
      getIPAddress(const folly::IPAddress& ip) const {
    return ip.asV4();
  }

  template <typename ADDRT = AddrT>
  typename std::
      enable_if<std::is_same<ADDRT, folly::IPAddressV6>::value, ADDRT>::type
      getIPAddress(const folly::IPAddress& ip) const {
    return ip.asV6();
  }

  SwSwitch* sw_;
};

using NextHopResolverV4 = NextHopResolver<folly::IPAddressV4>;
using NextHopResolverV6 = NextHopResolver<folly::IPAddressV6>;

} // namespace facebook::fboss
