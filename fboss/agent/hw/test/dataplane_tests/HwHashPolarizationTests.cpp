/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

class HwHashPolarizationTests : public HwLinkStateDependentTest {
 private:
  static constexpr auto kEcmpWidth = 8;
  static constexpr auto kMaxDeviation = 25;
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }
  void packetReceived(RxPacket* pkt) noexcept override {
    folly::io::Cursor cursor{pkt->buf()};
    pktsReceived_.wlock()->emplace_back(utility::EthFrame{cursor});
  }
  template <typename AddrT>
  void programRoutes() {
    auto ecmpPorts = getEcmpPorts();
    boost::container::flat_set<PortDescriptor> ports;
    std::for_each(ecmpPorts.begin(), ecmpPorts.end(), [&ports](auto ecmpPort) {
      ports.insert(PortDescriptor{ecmpPort});
    });
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(getProgrammedState());
    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), ports));
    ecmpHelper.programRoutes(getRouteUpdater(), ports);
  }
  std::vector<PortID> getEcmpPorts() const {
    auto masterLogicalPorts = masterLogicalPortIds();
    return {
        masterLogicalPorts.begin(), masterLogicalPorts.begin() + kEcmpWidth};
  }
  std::vector<PortDescriptor> getEcmpPortDesc() const {
    auto masterLogicalPorts = masterLogicalPortIds();
    return {
        masterLogicalPorts.begin(), masterLogicalPorts.begin() + kEcmpWidth};
  }

 protected:
  void runTest(
      const cfg::LoadBalancer& firstHash,
      const cfg::LoadBalancer& secondHash) {
    auto setup = [this]() {
      programRoutes<folly::IPAddressV4>();
      programRoutes<folly::IPAddressV6>();
    };
    auto verify = [this, firstHash, secondHash]() {
      auto ecmpPorts = getEcmpPorts();
      HwTestPacketTrapEntry trapPkts(
          getHwSwitch(),
          std::set<PortID>{
              ecmpPorts.begin(), ecmpPorts.begin() + kEcmpWidth / 2});
      // Set first hash
      applyNewState(utility::setLoadBalancer(
          getPlatform(), getProgrammedState(), firstHash));

      auto firstVlan = utility::firstVlanID(getProgrammedState());
      auto mac = utility::getInterfaceMac(getProgrammedState(), firstVlan);
      for (auto isV6 : {true, false}) {
        utility::pumpTraffic(
            isV6,
            getHwSwitch(),
            mac,
            firstVlan,
            masterLogicalPortIds()[kEcmpWidth]);
      }

      EXPECT_TRUE(utility::isLoadBalanced(
          getHwSwitchEnsemble(), getEcmpPortDesc(), kMaxDeviation));
      XLOG(INFO) << " Num captured packets: " << pktsReceived_.rlock()->size();
    };
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  folly::Synchronized<std::vector<utility::EthFrame>> pktsReceived_;
};

TEST_F(HwHashPolarizationTests, fullXfullHash) {
  auto fullHashCfg = utility::getEcmpFullHashConfig(getPlatform());
  runTest(fullHashCfg, fullHashCfg);
}

} // namespace facebook::fboss
