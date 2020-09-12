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

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <memory>
#include <vector>

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;
}

namespace facebook::fboss::utility {

template <typename IPAddrT>
struct EcmpNextHop {
  EcmpNextHop(
      IPAddrT _ip,
      PortDescriptor _portDesc,
      folly::MacAddress _mac,
      InterfaceID _intf)
      : ip(_ip), portDesc(_portDesc), mac(_mac), intf(_intf) {}
  IPAddrT ip;
  PortDescriptor portDesc;
  folly::MacAddress mac;
  InterfaceID intf;
};

template <typename IPAddrT>
struct EcmpMplsNextHop {
  EcmpMplsNextHop(
      IPAddrT _ip,
      PortDescriptor _portDesc,
      folly::MacAddress _mac,
      InterfaceID _intf,
      LabelForwardingAction _action)
      : ip(_ip),
        portDesc(_portDesc),
        mac(_mac),
        intf(_intf),
        action(std::move(_action)) {}
  IPAddrT ip;
  PortDescriptor portDesc;
  folly::MacAddress mac;
  InterfaceID intf;
  LabelForwardingAction action;
};

template <typename AddrT, typename NextHopT>
class BaseEcmpSetupHelper {
 public:
  static auto constexpr kIsV6 = std::is_same<AddrT, folly::IPAddressV6>::value;

  BaseEcmpSetupHelper();
  virtual NextHopT nhop(PortDescriptor portDesc) const = 0;
  const std::vector<NextHopT>& getNextHops() const {
    return nhops_;
  }
  virtual ~BaseEcmpSetupHelper() {}
  virtual std::vector<PortDescriptor> ecmpPortDescs(int width) const;
  PortDescriptor ecmpPortDescriptorAt(int index) const {
    return ecmpPortDescs(index + 1)[index];
  }
  AddrT ip(PortDescriptor portDesc) const {
    return nhop(portDesc).ip;
  }

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

  virtual void computeNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      std::optional<folly::MacAddress> mac) = 0;

  std::shared_ptr<SwitchState> resolveNextHopsImpl(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescs,
      bool resolve) const;

  std::shared_ptr<SwitchState> resolveNextHop(
      const std::shared_ptr<SwitchState>& inputState,
      const NextHopT& nhop) const;

  std::shared_ptr<SwitchState> unresolveNextHop(
      const std::shared_ptr<SwitchState>& inputState,
      const NextHopT& nhop) const;

  std::optional<VlanID> getVlan(const PortDescriptor& port) const;

 protected:
  std::vector<NextHopT> nhops_;
  boost::container::flat_map<PortDescriptor, VlanID> portDesc2Vlan_;
};

template <typename IPAddrT>
class EcmpSetupTargetedPorts
    : public BaseEcmpSetupHelper<IPAddrT, EcmpNextHop<IPAddrT>> {
 public:
  using BaseEcmpSetupHelperT =
      BaseEcmpSetupHelper<IPAddrT, EcmpNextHop<IPAddrT>>;
  using RouteT = typename Route<IPAddrT>::Prefix;
  using EcmpNextHopT = EcmpNextHop<IPAddrT>;
  explicit EcmpSetupTargetedPorts(
      const std::shared_ptr<SwitchState>& inputState,
      RouterID routerId = RouterID(0))
      : EcmpSetupTargetedPorts(inputState, std::nullopt, routerId) {}

  EcmpSetupTargetedPorts(
      const std::shared_ptr<SwitchState>& inputState,
      std::optional<folly::MacAddress> nextHopMac,
      RouterID routerId = RouterID(0));

  virtual ~EcmpSetupTargetedPorts() override {}
  EcmpNextHopT nhop(PortDescriptor portDesc) const override;

  RouterID getRouterId() const {
    return routerId_;
  }

  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& nhops,
      const std::vector<RouteT>& prefixes = {RouteT{IPAddrT(), 0}},
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  std::shared_ptr<SwitchState> setupIp2MplsECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescriptors,
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks,
      const std::vector<RouteT>& prefixes = {RouteT{IPAddrT(), 0}},
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  std::shared_ptr<SwitchState> pruneECMPRoutes(
      const std::shared_ptr<SwitchState>& inputState,
      const std::vector<RouteT>& prefixes = {RouteT{IPAddrT(), 0}}) const;

 private:
  virtual void computeNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      std::optional<folly::MacAddress> mac = std::nullopt) override;

  RouterID routerId_;
};

using BaseEcmpSetupHelper4L3 =
    BaseEcmpSetupHelper<folly::IPAddressV4, EcmpNextHop<folly::IPAddressV4>>;
using BaseEcmpSetupHelper6L3 =
    BaseEcmpSetupHelper<folly::IPAddressV6, EcmpNextHop<folly::IPAddressV6>>;

using EcmpSetupTargetedPorts4 = EcmpSetupTargetedPorts<folly::IPAddressV4>;
using EcmpSetupTargetedPorts6 = EcmpSetupTargetedPorts<folly::IPAddressV6>;

template <typename IPAddrT>
class MplsEcmpSetupTargetedPorts
    : public BaseEcmpSetupHelper<IPAddrT, EcmpMplsNextHop<IPAddrT>> {
 public:
  using BaseEcmpSetupHelperT =
      BaseEcmpSetupHelper<IPAddrT, EcmpMplsNextHop<IPAddrT>>;

  explicit MplsEcmpSetupTargetedPorts(
      const std::shared_ptr<SwitchState>& inputState,
      LabelForwardingEntry::Label topLabel,
      LabelForwardingAction::LabelForwardingType actionType)
      : topLabel_(topLabel), actionType_(actionType) {
    computeNextHops(inputState, std::nullopt);
  }

  virtual EcmpMplsNextHop<IPAddrT> nhop(PortDescriptor portDesc) const override;

  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& nhops,
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  LabelForwardingAction getLabelForwardingAction(PortDescriptor port) const;

 private:
  virtual void computeNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      std::optional<folly::MacAddress> mac = std::nullopt) override;

  LabelForwardingEntry::Label topLabel_;
  LabelForwardingAction::LabelForwardingType actionType_;
};

template <typename IPAddrT>
class EcmpSetupAnyNPorts {
 public:
  using RouteT = typename Route<IPAddrT>::Prefix;
  using EcmpNextHopT = EcmpNextHop<IPAddrT>;
  explicit EcmpSetupAnyNPorts(
      const std::shared_ptr<SwitchState>& inputState,
      RouterID routerId = RouterID(0))
      : EcmpSetupAnyNPorts(inputState, std::nullopt, routerId) {}

  EcmpSetupAnyNPorts(
      const std::shared_ptr<SwitchState>& inputState,
      const std::optional<folly::MacAddress>& nextHopMac,
      RouterID routerId = RouterID(0))
      : ecmpSetupTargetedPorts_(inputState, nextHopMac, routerId) {}

  EcmpNextHopT nhop(size_t id) const {
    return getNextHops()[id];
  }
  const std::vector<EcmpNextHopT>& getNextHops() const {
    return ecmpSetupTargetedPorts_.getNextHops();
  }
  IPAddrT ip(size_t id) const {
    return nhop(id).ip;
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

  // targeted unresolve
  std::shared_ptr<SwitchState> unresolveNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescs) const;
  /*
   * Setup ECMP group with next hops going over any N ports
   */
  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      size_t width,
      const std::vector<RouteT>& prefixes = {RouteT{IPAddrT(), 0}},
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  std::shared_ptr<SwitchState> setupIp2MplsECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      size_t width,
      const std::vector<RouteT>& prefixes = {RouteT{IPAddrT(), 0}},
      std::vector<LabelForwardingAction::LabelStack> stacks = {{10010}},
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  std::shared_ptr<SwitchState> pruneECMPRoutes(
      const std::shared_ptr<SwitchState>& inputState,
      const std::vector<RouteT>& prefixes = {RouteT{IPAddrT(), 0}}) const;
  std::optional<VlanID> getVlan(const PortDescriptor& port) const {
    return ecmpSetupTargetedPorts_.getVlan(port);
  }

 private:
  boost::container::flat_set<PortDescriptor> getPortDescs(int width) const;
  EcmpSetupTargetedPorts<IPAddrT> ecmpSetupTargetedPorts_;
};

template <typename IPAddrT>
class MplsEcmpSetupAnyNPorts {
 public:
  using EcmpNextHopT = EcmpMplsNextHop<IPAddrT>;
  MplsEcmpSetupAnyNPorts(
      const std::shared_ptr<SwitchState>& inputState,
      LabelForwardingEntry::Label topLabel,
      LabelForwardingAction::LabelForwardingType actionType)
      : mplsEcmpSetupTargetedPorts_(inputState, topLabel, actionType) {}

  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      size_t width,
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  EcmpNextHopT nhop(size_t id) const {
    return getNextHops()[id];
  }
  const std::vector<EcmpNextHopT>& getNextHops() const {
    return mplsEcmpSetupTargetedPorts_.getNextHops();
  }
  IPAddrT ip(size_t id) const {
    return nhop(id).ip;
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

  // targeted unresolve
  std::shared_ptr<SwitchState> unresolveNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescs) const;

 private:
  boost::container::flat_set<PortDescriptor> getPortDescs(int width) const;
  MplsEcmpSetupTargetedPorts<IPAddrT> mplsEcmpSetupTargetedPorts_;
};

using EcmpSetupAnyNPorts4 = EcmpSetupAnyNPorts<folly::IPAddressV4>;
using EcmpSetupAnyNPorts6 = EcmpSetupAnyNPorts<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
