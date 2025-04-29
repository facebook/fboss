/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/IPAddress.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/QueueTestUtils.h"

#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class Agent2QueueToOlympicQoSTest : public AgentHwTest {
 private:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return cfg;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::L3_QOS,
        production_features::ProductionFeature::OLYMPIC_QOS};
  }

  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
      uint8_t dscpVal) const {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    return utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        static_cast<uint8_t>(dscpVal << 2)); // Trailing 2 bits are for ECN
  }

  void sendPacket(int dscpVal, bool frontPanel) {
    auto txPacket = createUdpPkt(dscpVal);
    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      auto outPort = ecmpHelper.ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getAgentEnsemble()->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), outPort);
    } else {
      getAgentEnsemble()->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }
  }

  void _verifyDscpQueueMappingHelper(
      const std::map<int, std::vector<uint8_t>>& queueToDscp,
      bool frontPanel) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    auto portId = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
    std::optional<SystemPortID> sysPortId;
    auto switchId = getSw()->getScopeResolver()->scope(portId).switchId();
    auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);
    if (asic->getSwitchType() == cfg::SwitchType::VOQ) {
      sysPortId =
          getSystemPortID(portId, getProgrammedState(), SwitchID(switchId));
    }
    XLOG(DBG2) << "verify send packets "
               << (frontPanel ? "out of port" : "switched");
    utility::sendPktAndVerifyQueueHit(
        queueToDscp,
        getSw(),
        [this, frontPanel](int dscp) { sendPacket(dscp, frontPanel); },
        portId,
        sysPortId);
  }

 protected:
  void runTest() {
    auto setup = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      auto portId = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
      auto switchId = getSw()->getScopeResolver()->scope(portId).switchId();
      auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);
      resolveNeighborAndProgramRoutes(ecmpHelper, kEcmpWidth);
      auto newCfg{initialConfig(*getAgentEnsemble())};
      auto streamType =
          *(asic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin());
      utility::add2QueueConfig(
          &newCfg,
          streamType,
          asic->scalingFactorBasedDynamicThresholdSupported());
      utility::add2QueueQosMaps(newCfg, asic);
      applyNewConfig(newCfg);
    };

    auto verify = [=, this]() {
      XLOG(DBG2) << "verify send packets switched";
      _verifyDscpQueueMappingHelper(utility::k2QueueToDscp(), false);
      XLOG(DBG2) << "verify send packets out of port";
      _verifyDscpQueueMappingHelper(utility::k2QueueToDscp(), true);
    };

    auto setupPostWarmboot = [=, this]() {
      auto newCfg{initialConfig(*getAgentEnsemble())};
      utility::addOlympicQueueConfig(
          &newCfg, getSw()->getHwAsicTable()->getL3Asics());
      utility::addOlympicQosMaps(
          newCfg, getSw()->getHwAsicTable()->getL3Asics());
      applyNewConfig(newCfg);
    };

    auto verifyPostWarmboot = [=, this]() {
      XLOG(DBG2) << "verify send packets switched";
      _verifyDscpQueueMappingHelper(utility::kOlympicQueueToDscp(), false);
      XLOG(DBG2) << "verify send packets out of port";
      _verifyDscpQueueMappingHelper(utility::kOlympicQueueToDscp(), true);
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }
  static inline constexpr auto kEcmpWidth = 1;
};

TEST_F(Agent2QueueToOlympicQoSTest, verifyDscpToQueueMapping) {
  runTest();
}

} // namespace facebook::fboss
