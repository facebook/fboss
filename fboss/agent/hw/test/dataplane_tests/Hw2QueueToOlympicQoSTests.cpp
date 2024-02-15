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
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTest2QueueUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"

#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class Hw2QueueToOlympicQoSTest : public HwLinkStateDependentTest {
 private:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
      uint8_t dscpVal) const {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    return utility::makeUDPTxPacket(
        getHwSwitch(),
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
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getHwSwitchEnsemble()->getHwSwitch()->sendPacketOutOfPortSync(
          std::move(txPacket), outPort);
    } else {
      getHwSwitchEnsemble()->getHwSwitch()->sendPacketSwitchedSync(
          std::move(txPacket));
    }
  }

  void _verifyDscpQueueMappingHelper(
      const std::map<int, std::vector<uint8_t>>& queueToDscp,
      bool frontPanel) {
    auto portId = helper_->ecmpPortDescriptorAt(0).phyPortID();
    std::optional<SystemPortID> sysPortId;
    if (getHwSwitch()->getSwitchType() == cfg::SwitchType::VOQ) {
      auto switchId = *getHwSwitch()->getSwitchId();
      sysPortId =
          getSystemPortID(portId, getProgrammedState(), SwitchID(switchId));
    }
    XLOG(DBG2) << "verify send packets "
               << (frontPanel ? "out of port" : "switched");
    utility::sendPktAndVerifyQueueHit(
        queueToDscp,
        getHwSwitchEnsemble(),
        [this, frontPanel](int dscp) { sendPacket(dscp, frontPanel); },
        portId,
        sysPortId);
  }

 protected:
  void runTest() {
    auto setup = [=, this]() {
      resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);
      auto newCfg{initialConfig()};
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::add2QueueConfig(
          &newCfg,
          streamType,
          getPlatform()
              ->getAsic()
              ->scalingFactorBasedDynamicThresholdSupported());
      utility::add2QueueQosMaps(newCfg, getPlatform()->getAsic());
      applyNewConfig(newCfg);
    };

    auto verify = [=, this]() {
      XLOG(DBG2) << "verify send packets switched";
      _verifyDscpQueueMappingHelper(utility::k2QueueToDscp(), false);
      XLOG(DBG2) << "verify send packets out of port";
      _verifyDscpQueueMappingHelper(utility::k2QueueToDscp(), true);
    };

    auto setupPostWarmboot = [=, this]() {
      auto newCfg{initialConfig()};
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addOlympicQueueConfig(
          &newCfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(newCfg, getPlatform()->getAsic());
      applyNewConfig(newCfg);
    };

    auto verifyPostWarmboot = [=, this]() {
      XLOG(DBG2) << "verify send packets switched";
      _verifyDscpQueueMappingHelper(
          utility::kOlympicQueueToDscp(getPlatform()->getAsic()), false);
      XLOG(DBG2) << "verify send packets out of port";
      _verifyDscpQueueMappingHelper(
          utility::kOlympicQueueToDscp(getPlatform()->getAsic()), true);
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }
  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

TEST_F(Hw2QueueToOlympicQoSTest, verifyDscpToQueueMapping) {
  runTest();
}

} // namespace facebook::fboss
