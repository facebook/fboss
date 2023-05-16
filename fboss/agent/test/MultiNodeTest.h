/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentTest.h"
#include "fboss/agent/types.h"

DECLARE_string(config);

namespace facebook::fboss {

class MultiNodeTest : public AgentTest {
 protected:
  void SetUp() override;
  void setupConfigFlag() override;

  std::unique_ptr<FbossCtrlAsyncClient> getRemoteThriftClient() const;

  void checkNeighborResolved(
      const folly::IPAddress& ip,
      VlanID vlan,
      PortDescriptor port) const;
  bool isDUT() const;

  template <typename AddrT>
  std::pair<folly::MacAddress, PortDescriptor> getNeighborEntry(
      AddrT addr,
      InterfaceID intfID) const {
    auto state = sw()->getState();
    auto vlan = state->getInterfaces()->getNodeIf(intfID)->getVlanID();
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      auto entry = sw()->getState()
                       ->getVlans()
                       ->getVlanIf(vlan)
                       ->getNdpTable()
                       ->getEntryIf(addr);
      return {entry->getMac(), entry->getPort()};
    } else {
      auto entry = sw()->getState()
                       ->getVlans()
                       ->getVlanIf(vlan)
                       ->getArpTable()
                       ->getEntryIf(addr);
      return {entry->getMac(), entry->getPort()};
    }
  }

  template <typename AddrT>
  void disableTTLDecrementsForRoute(RoutePrefix<AddrT> prefix) const {
    auto fibContainer =
        sw()->getState()->getFibs()->getFibContainer(RouterID(0));
    auto fib = fibContainer->template getFib<AddrT>();
    auto defaultRoute = fib->getRouteIf(prefix);
    auto nhSet = defaultRoute->getForwardInfo().getNextHopSet();

    for (const auto nhop : nhSet) {
      if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
        auto ecmpNh = utility::EcmpNextHop<AddrT>(
            nhop.addr().asV6(),
            getNeighborEntry(nhop.addr().asV6(), nhop.intf()).second,
            getNeighborEntry(nhop.addr().asV6(), nhop.intf()).first,
            *nhop.intfID());
        utility::disableTTLDecrements(sw()->getHw(), RouterID(0), ecmpNh);
      } else {
        auto ecmpNh = utility::EcmpNextHop<AddrT>(
            nhop.addr().asV4(),
            getNeighborEntry(nhop.addr().asV4(), nhop.intf()).second,
            getNeighborEntry(nhop.addr().asV4(), nhop.intf()).first,
            *nhop.intfID());
        utility::disableTTLDecrements(sw()->getHw(), RouterID(0), ecmpNh);
      }
    }
  }
  std::vector<PortID> testPorts() const;
  std::vector<std::string> testPortNames() const;
  std::map<PortID, PortID> localToRemotePort() const;

 private:
  bool runVerification() const override {
    return isDUT();
  }
  virtual cfg::SwitchConfig initialConfig() const = 0;
  void parseTestPorts(std::string portList);
  std::vector<PortID> testPorts_;
};
int mulitNodeTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
