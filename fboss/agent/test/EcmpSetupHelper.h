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
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/Route.h"
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
struct EcmpMplsNextHop {
  EcmpMplsNextHop(
      IPAddrT _ip,
      PortDescriptor _portDesc,
      folly::MacAddress _mac,
      LabelForwardingAction _action)
      : ip(_ip), portDesc(_portDesc), mac(_mac), action(std::move(_action)) {}
  IPAddrT ip;
  PortDescriptor portDesc;
  folly::MacAddress mac;
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
      folly::Optional<folly::MacAddress> mac) = 0;

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

  folly::Optional<VlanID> getVlan(const PortDescriptor& port) const;

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

  virtual ~EcmpSetupTargetedPorts() override {}
  EcmpNextHop nhop(PortDescriptor portDesc) const;

  RouteT routePrefix() const {
    return routePrefix_;
  }

  RouterID getRouterId() const {
    return routerId_;
  }

  std::shared_ptr<SwitchState> setupECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& nhops,
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

  std::shared_ptr<SwitchState> setupIp2MplsECMPForwarding(
      const std::shared_ptr<SwitchState>& inputState,
      const boost::container::flat_set<PortDescriptor>& portDescriptors,
      std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks,
      const std::vector<NextHopWeight>& weights =
          std::vector<NextHopWeight>()) const;

 private:
  virtual void computeNextHops(
      const std::shared_ptr<SwitchState>& inputState,
      folly::Optional<folly::MacAddress> mac = folly::none) override;

  RouterID routerId_;
  const RouteT routePrefix_;
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
        computeNextHops(inputState, folly::none);
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
      folly::Optional<folly::MacAddress> mac = folly::none) override;

  LabelForwardingEntry::Label topLabel_;
  LabelForwardingAction::LabelForwardingType actionType_;
};

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
