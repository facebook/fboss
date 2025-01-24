/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

namespace facebook::fboss {

class AgentOlympicQosTests : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addOlympicQueueConfig(&cfg, ensemble.getL3Asics());
    utility::addOlympicQosMaps(cfg, ensemble.getL3Asics());
    return cfg;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_QOS};
  }

  void verifyDscpQueueMapping() {
    auto setup = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      resolveNeigborAndProgramRoutes(ecmpHelper, kEcmpWidth);
    };

    auto verify = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      auto portId = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
      std::optional<SystemPortID> sysPortId;
      if (getSw()->getSwitchInfoTable().haveVoqSwitches()) {
        auto switchId = switchIdForPort(portId);
        sysPortId = getSystemPortID(portId, getProgrammedState(), switchId);
      }
      for (bool frontPanel : {false, true}) {
        XLOG(DBG2) << "verify send packets "
                   << (frontPanel ? "out of port" : "switched");
        utility::sendPktAndVerifyQueueHit(
            utility::kOlympicQueueToDscp(),
            getSw(),
            [this, frontPanel](int dscp) { sendPacket(dscp, frontPanel); },
            portId,
            sysPortId);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPacket(uint8_t dscp, bool frontPanel) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
        8000, // l4 src port
        8001, // l4 dst port
        dscp << 2, // shifted by 2 bits for ECN
        255 // ttl
    );
    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      auto outPort = ecmpHelper.ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }
  }

  static inline constexpr auto kEcmpWidth = 1;
};

// Verify that traffic arriving on a front panel/cpu port is qos mapped to the
// correct queue for each olympic dscp value.
TEST_F(AgentOlympicQosTests, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping();
}

} // namespace facebook::fboss
