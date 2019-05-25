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

#include <memory>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {
class SwitchState;
}
} // namespace facebook

namespace facebook {
namespace fboss {
namespace utility {

template <typename IPAddrT>
struct EcmpNextHop {
  EcmpNextHop(IPAddrT _ip, PortDescriptor _portDesc, folly::MacAddress _mac)
      : ip(_ip), portDesc(_portDesc), mac(_mac) {}
  IPAddrT ip;
  PortDescriptor portDesc;
  folly::MacAddress mac;
};


template <typename IPAddrT>
class EcmpSetupTargetedPorts {
 public:
  using RouteT = typename Route<IPAddrT>::Prefix;
  using EcmpNextHop = EcmpNextHop<IPAddrT>;
  explicit EcmpSetupTargetedPorts(
      const std::shared_ptr<SwitchState>& inputState,
      RouterID routerId = RouterID(0),
      RouteT routePrefix = RouteT{IPAddrT(), 0})
      : EcmpSetupTargetedPorts(inputState, folly::none, routerId, routePrefix) {
  }

  EcmpSetupTargetedPorts(
      const std::shared_ptr<SwitchState>& inputState,
      folly::Optional<folly::MacAddress> nextHopMac,
      RouterID routerId = RouterID(0),
      RouteT routePrefix = RouteT{IPAddrT(), 0});

  EcmpNextHop nhop(PortDescriptor portDesc) const;
  IPAddrT ip(PortDescriptor portDesc) const {
    return nhop(portDesc).ip;
  }

  RouteT routePrefix() const {
    return routePrefix_;
  }

  RouterID getRouterId() const {
    return routerId_;
  }
  const std::vector<EcmpNextHop>& getNextHops() const {
    return nhops_;
  }
  std::vector<PortDescriptor> ecmpPortDescs(int width) const;

  /*
   * resolveNextHops and unresolveNextHops resolves/unresolves the list
   * of port descriptors. The port descriptors can be a physical port or
   * a trunk.
   */
  std::shared_ptr<SwitchState> resolveNextHops(
          const std::shared_ptr<SwitchState>& inputState,
          const boost::container::flat_set<PortDescriptor>& portDescs) const;
  std::shared_ptr<SwitchState> unresolveNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescs) const;

  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& nhops,
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;


 private:
  void computeNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      folly::Optional<folly::MacAddress> mac = folly::none);

  std::shared_ptr<SwitchState> resolveNextHopsImpl(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescs,
      bool resolve) const;

  std::shared_ptr<SwitchState> resolveNextHop(
      const std::shared_ptr<SwitchState>& inputState,
      const EcmpNextHop& nhop) const;
  std::shared_ptr<SwitchState> unresolveNextHop(
      const std::shared_ptr<SwitchState>& inputState,
      const EcmpNextHop& nhop) const;

  std::vector<EcmpNextHop> nhops_;
  boost::container::flat_map<PortDescriptor, VlanID> portDesc2Vlan_;
  RouterID routerId_;
  const RouteT routePrefix_;
};

using EcmpSetupTargetedPorts4 = EcmpSetupTargetedPorts<folly::IPAddressV4>;
using EcmpSetupTargetedPorts6 = EcmpSetupTargetedPorts<folly::IPAddressV6>;

template <typename IPAddrT>
class EcmpSetupAnyNPorts {
 public:
  using RouteT = typename Route<IPAddrT>::Prefix;
  using EcmpNextHop = EcmpNextHop<IPAddrT>;
  explicit EcmpSetupAnyNPorts(
      const std::shared_ptr<SwitchState>& inputState,
      RouterID routerId = RouterID(0),
      RouteT routePrefix = RouteT{IPAddrT(), 0})
      : EcmpSetupAnyNPorts(inputState, folly::none, routerId, routePrefix) {}

  EcmpSetupAnyNPorts(
      const std::shared_ptr<SwitchState>& inputState,
      const folly::Optional<folly::MacAddress>& nextHopMac,
      RouterID routerId = RouterID(0),
      RouteT routePrefix = RouteT{IPAddrT(), 0})
      : ecmpSetupTargetedPorts_(inputState, nextHopMac, routerId, routePrefix) {
  }

  EcmpNextHop nhop(size_t id) const {
    return getNextHops()[id];
  }
  const std::vector<EcmpNextHop>& getNextHops() const {
    return ecmpSetupTargetedPorts_.getNextHops();
  }
  IPAddrT ip(size_t id) const {
    return nhop(id).ip;
  }

  RouteT routePrefix() const {
    return ecmpSetupTargetedPorts_.routePrefix();
  }

  RouterID getRouterId() const {
    return ecmpSetupTargetedPorts_.getRouterId();
  }
  std::vector<PortDescriptor> ecmpPortDescs(int width) const;
  PortDescriptor ecmpPortDescriptorAt(int index) const {
    return ecmpPortDescs(index + 1)[index];
  }


  /*
   * resolveNextHops and unresolveNextHops resolves/unresolves the first
   * numNextHops stored in setup helper created during the setup.
   */
  std::shared_ptr<SwitchState> resolveNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      size_t numNextHops) const;
  std::shared_ptr<SwitchState> unresolveNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      size_t numNextHops) const;

  /*
   * Targeted resolve. Resolve only a subset of next hops.
   * This is useful, in cases where we don't care which
   * N ports get picked for ECMP group, but later want to
   * selectively control which of the next hops want have
   * their ARP/NDP resolved
   */
  std::shared_ptr<SwitchState> resolveNextHops(
          const std::shared_ptr<SwitchState>& inputState,
          const boost::container::flat_set<PortDescriptor>& portDescs) const;
  /*
   * Setup ECMP group with next hops going over any N ports
   */
  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      size_t width,
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;
 private:
  boost::container::flat_set<PortDescriptor> getPortDescs(int width) const;
  EcmpSetupTargetedPorts<IPAddrT> ecmpSetupTargetedPorts_;
};

using EcmpSetupAnyNPorts4 = EcmpSetupAnyNPorts<folly::IPAddressV4>;
using EcmpSetupAnyNPorts6 = EcmpSetupAnyNPorts<folly::IPAddressV6>;
} // namespace utility
} // namespace fboss
} // namespace facebook
