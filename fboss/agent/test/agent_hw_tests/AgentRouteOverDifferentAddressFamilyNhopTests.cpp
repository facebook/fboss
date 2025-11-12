/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/packet/PktFactory.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>
#include "fboss/agent/TxPacket.h"

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class AgentRouteOverDifferentAddressFamilyNhopTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

 private:
  folly::IPAddressV6 kIpv6(bool ecmp) const {
    return ecmp ? folly::IPAddressV6{"200::1"} : folly::IPAddressV6{"201::1"};
  }
  folly::IPAddressV4 kIpv4(bool ecmp) const {
    return ecmp ? folly::IPAddressV4{"200.0.0.1"}
                : folly::IPAddressV4{"201.0.0.1"};
  }
  folly::CIDRNetwork kPrefixV6(bool ecmp) const {
    return {kIpv6(ecmp), 64};
  }
  folly::CIDRNetwork kPrefixV4(bool ecmp) const {
    return {kIpv4(ecmp), 24};
  }
  template <typename AddrT>
  folly::CIDRNetwork getPrefix(bool ecmp) const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return kPrefixV6(ecmp);
    } else {
      return kPrefixV4(ecmp);
    }
  }
  template <typename AddrT>
  AddrT getDstIp(bool ecmp) const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return kIpv6(ecmp);
    } else {
      return kIpv4(ecmp);
    }
  }
  std::unique_ptr<TxPacket> makeTxPacket(const folly::IPAddress& dstIp) const {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto srcIp = dstIp.isV6() ? folly::IPAddress("100::1")
                              : folly::IPAddress("100.0.0.1");
    return utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        srcIp,
        dstIp,
        8000, // l4 src port
        8001 // l4 dst port
    );
  }

 protected:
  template <typename RouteAddrT, typename NhopAddrT>
  void runTest(bool useLinkLocalNhop = false) {
    if (useLinkLocalNhop) {
      if constexpr (std::is_same_v<NhopAddrT, folly::IPAddressV4>) {
        CHECK(false) << "Link local hop only supported for V6";
      }
    }
    auto setup = [this, useLinkLocalNhop] {
      std::set<cfg::PortType> portTypes;
      std::vector<PortID> portIdsForTest;
      if (FLAGS_hyper_port) {
        portIdsForTest = masterLogicalHyperPortIds();
        portTypes = {cfg::PortType::HYPER_PORT};
      } else {
        portIdsForTest = masterLogicalInterfacePortIds();
        portTypes = {cfg::PortType::INTERFACE_PORT};
      }
      utility::EcmpSetupTargetedPorts<NhopAddrT> ecmpHelper(
          getProgrammedState(),
          getSw()->needL2EntryForNeighbor(),
          std::nullopt,
          RouterID(0),
          false,
          portTypes);
      auto addRoute = [this, &ecmpHelper, useLinkLocalNhop](
                          auto nw, const auto& nhopPorts) {
        RouteNextHopEntry::NextHopSet nhops;
        for (auto nhopPort : nhopPorts) {
          auto nhop = ecmpHelper.nhop(nhopPort);
          if (useLinkLocalNhop) {
            nhops.insert(
                ResolvedNextHop(*nhop.linkLocalNhopIp, nhop.intf, ECMP_WEIGHT));
          } else {
            nhops.insert(UnresolvedNextHop(nhop.ip, ECMP_WEIGHT));
          }
        }
        auto updater = getSw()->getRouteUpdater();
        updater.addRoute(
            RouterID(0),
            nw.first,
            nw.second,
            ClientID::BGPD,
            RouteNextHopEntry(nhops, AdminDistance::EBGP));
        updater.program();
      };
      boost::container::flat_set<PortDescriptor> ecmpNhopPorts(
          {PortDescriptor(portIdsForTest[0]),
           PortDescriptor(portIdsForTest[1])});
      boost::container::flat_set<PortDescriptor> nonEcmpNhopPorts(
          {PortDescriptor(portIdsForTest[2])});
      for (auto ecmp : {true, false}) {
        auto nhopPorts = ecmp ? ecmpNhopPorts : nonEcmpNhopPorts;
        applyNewState([&](const std::shared_ptr<SwitchState>& in) {
          auto out = in->clone();
          return ecmpHelper.resolveNextHops(out, nhopPorts, useLinkLocalNhop);
        });
        addRoute(getPrefix<RouteAddrT>(ecmp), nhopPorts);
      }
    };
    auto verify = [this]() {
      for (auto ecmp : {true, false}) {
        getAgentEnsemble()->ensureSendPacketSwitched(
            makeTxPacket(getDstIp<RouteAddrT>(ecmp)));
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  };
};

TEST_F(AgentRouteOverDifferentAddressFamilyNhopTest, v4RouteV6Nhops) {
  runTest<folly::IPAddressV4, folly::IPAddressV6>();
}

TEST_F(AgentRouteOverDifferentAddressFamilyNhopTest, v4RouteV6LinkLocalNhops) {
  runTest<folly::IPAddressV4, folly::IPAddressV6>(
      true /*use link local nhops*/);
}

TEST_F(AgentRouteOverDifferentAddressFamilyNhopTest, v6RouteV4Nhops) {
  runTest<folly::IPAddressV6, folly::IPAddressV4>();
}
} // namespace facebook::fboss
