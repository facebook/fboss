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
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook::fboss {

class HwOlympicQosTests : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    return cfg;
  }

  void verifyDscpQueueMapping(bool frontPanel) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [=]() {
      resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);

      auto newCfg{initialConfig()};
      utility::addOlympicQosMaps(newCfg, getAsic());
      applyNewConfig(newCfg);
    };

    auto verify = [=]() {
      auto portId = helper_->ecmpPortDescriptorAt(0).phyPortID();
      auto portStatsBefore = getLatestPortStats(portId);
      for (const auto& q2dscps : utility::kOlympicQueueToDscp(getAsic())) {
        for (auto dscp : q2dscps.second) {
          sendPacket(dscp, frontPanel);
        }
      }
      EXPECT_TRUE(utility::verifyQueueMappings(
          portStatsBefore,
          utility::kOlympicQueueToDscp(getAsic()),
          getHwSwitchEnsemble(),
          portId));
    };
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPacket(uint8_t dscp, bool frontPanel) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
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
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), outPort);
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    }
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

// Verify that traffic arriving on a front panel port is qos mapped to the
// correct queue for each olympic dscp value.
TEST_F(HwOlympicQosTests, VerifyDscpQueueMappingFrontPanel) {
  verifyDscpQueueMapping(true);
}

// Verify that traffic originating on the CPU is qos mapped to the correct
// queue for each olympic dscp value.
TEST_F(HwOlympicQosTests, VerifyDscpQueueMappingCpu) {
  verifyDscpQueueMapping(false);
}

} // namespace facebook::fboss
