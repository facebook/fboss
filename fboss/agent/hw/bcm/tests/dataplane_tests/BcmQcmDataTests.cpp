/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddressV6.h>
#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace {
constexpr int kPktTxCount = 10;
const folly::IPAddressV6 kIPv6Route = folly::IPAddressV6("2401::201:ab00");
const folly::IPAddress kIPv6FlowDstAddress =
    folly::IPAddressV6("2401::201:ab01");
const folly::IPAddress kIPv6FlowSrcAddress1 = folly::IPAddressV6("1::9");
const folly::IPAddress kIPv6FlowSrcAddress2 = folly::IPAddressV6("1::10");
const folly::IPAddress kIPv6FlowSrcAddress3 = folly::IPAddressV6("1::11");
const folly::IPAddress kIPv6FlowSrcAddress4 = folly::IPAddressV6("1::12");
const folly::IPAddress kIPv4FlowSrcAddress = folly::IPAddressV4("10.0.0.1");

const std::string kCollctorSrcIp = "10.0.0.2/32";
const std::string kCollctorDstIp = "11.0.0.2/32";
int kCollectorUDPSrcPort = 20000;
const int kMaskV6 = 120;
} // namespace

namespace facebook::fboss {

class BcmQcmDataTest : public BcmLinkStateDependentTests {
 protected:
  void SetUp() override {
    FLAGS_load_qcm_fw = true;
    BcmLinkStateDependentTests::SetUp();
    ecmpHelper6_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto config = utility::onePortPerVlanConfig(
        getHwSwitch(), std::move(ports), cfg::PortLoopbackMode::MAC, true);
    return config;
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    auto qcmCfg = cfg::QcmConfig();
    // some dummy address
    qcmCfg.collectorDstIp = kCollctorSrcIp;
    qcmCfg.collectorSrcIp = kCollctorDstIp;
    qcmCfg.collectorSrcPort = kCollectorUDPSrcPort;
    newCfg.qcmConfig_ref() = qcmCfg;
    newCfg.switchSettings.qcmEnable = true;
    applyNewConfig(newCfg);
    cfg_ = newCfg;

    getHwSwitch()->getBcmQcmMgr()->updateQcmMonitoredPortsIfNeeded(
        {masterLogicalPortIds()[0], masterLogicalPortIds()[1]},
        true /* attach stat */);
    addRoute(kIPv6Route, kMaskV6, PortDescriptor(masterLogicalPortIds()[0]));
  }

  void setupQcmOnly(const bool qcmEnable) {
    cfg_.switchSettings.qcmEnable = qcmEnable;
    applyNewConfig(cfg_);
    if (qcmEnable) {
      getHwSwitch()->getBcmQcmMgr()->updateQcmMonitoredPortsIfNeeded(
          {masterLogicalPortIds()[0], masterLogicalPortIds()[1]},
          true /* attach stat */);
    }
  }

  uint32_t featuresDesired() const override {
    return (HwSwitch::LINKSCAN_DESIRED | HwSwitch::PACKET_RX_DESIRED);
  }

  void addRoute(folly::IPAddress prefix, uint8_t mask, PortDescriptor port) {
    auto state = ecmpHelper6_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        });

    // current support is for v6 flows only.
    // We could revisit to add support for v4 at a later time
    if (prefix.isV6()) {
      applyNewState(ecmpHelper6_->setupECMPForwarding(
          state, {port}, {RoutePrefixV6{prefix.asV6(), mask}}));
    }
  }

  void sendL3Packet(
      folly::IPAddress dstIp,
      PortID from,
      const std::optional<folly::IPAddress>& srcIp = std::nullopt) {
    auto vlanId = utility::firstVlanID(initialConfig());
    // construct eth hdr
    const auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    EthHdr::VlanTags_t vlans{
        VlanTag(vlanId, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN))};

    EthHdr eth{intfMac, intfMac, std::move(vlans), 0x86DD};
    // construct l3 hdr
    bool isV6 = dstIp.isV6();
    const auto dummySrcIp = isV6 ? kIPv6FlowSrcAddress2 : kIPv4FlowSrcAddress;
    auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);
    auto pkt = utility::makeTCPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        srcIp ? *srcIp : dummySrcIp,
        dstIp,
        60000,
        60001,
        0);

    // send pkt on src port, let it loop back in switch and be l3 switched
    getHwSwitch()->sendPacketOutOfPortSync(std::move(pkt), from);
  }

  bool isQcmSupported() {
#if BCM_VER_SUPPORT_QCM
    return getPlatform()->getAsic()->isSupported(HwAsic::Feature::QCM);
#else
    return false;
#endif
  }
  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper6_;
  cfg::SwitchConfig cfg_;
};

// Intent of this test is to setup QCM tables
// send a flow and verify if the flow learning happens
// Only first pkt should hit the ifp entry, all subsequent pkts should
// not hit the ifp entry
// Since its 1 flow, we should only learn 1 flow
TEST_F(BcmQcmDataTest, FlowLearning) {
  if (!isQcmSupported()) {
    return;
  }
  auto setup = [&]() { setupHelper(); };

  auto verify = [&]() {
    int port0Counter = 0, port1Counter = 0;
    for (int i = 0; i < kPktTxCount; i++) {
      sendL3Packet(kIPv6FlowDstAddress, masterLogicalPortIds()[1]);
      int prevPort0Counter = port0Counter;
      port0Counter = getHwSwitch()->getBcmQcmMgr()->getIfpStatCounter(
          masterLogicalPortIds()[0]);
      port1Counter = getHwSwitch()->getBcmQcmMgr()->getIfpStatCounter(
          masterLogicalPortIds()[1]);
      EXPECT_EQ(port1Counter, 1);
      EXPECT_EQ(port0Counter, prevPort0Counter + 1);
    }
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 1);
  };

  // TODO: rohitpuri
  // Enable warmboot once QCM supports it CS00010434741
  // verifyAcrossWarmBoots(setup, verify);
  setup();
  verify();
}

// Intent of this test is to setup QCM tables
// send multple flows and check that learning happens
// for multiple flows
TEST_F(BcmQcmDataTest, MultiFlows) {
  if (!isQcmSupported()) {
    return;
  }
  auto setup = [&]() { setupHelper(); };

  auto verify = [&]() {
    for (int i = 0; i < kPktTxCount; i++) {
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress1);
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress2);
      auto port1Counter = getHwSwitch()->getBcmQcmMgr()->getIfpStatCounter(
          masterLogicalPortIds()[1]);
      // we learn 2 flows after first hit, so counter never goes higher than 2
      EXPECT_EQ(port1Counter, 2);
    }
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 2);
  };

  // TODO: rohitpuri
  // Enable warmboot once QCM supports it CS00010434741
  // verifyAcrossWarmBoots(setup, verify);
  setup();
  verify();
}

// Intent of this test is to setup QCM tables
// send multple flows, but restrict flow limit to 1
// Ensure that only 1 flow is learned
// also ifp entry should hit multiple times
TEST_F(BcmQcmDataTest, RestrictFlowLearning) {
  if (!isQcmSupported()) {
    return;
  }
  auto setup = [&]() {
    setupHelper();
    // force the flow learning to 1 only
    getHwSwitch()->getBcmQcmMgr()->flowLimitSet(1);
  };

  auto verify = [&]() {
    for (int i = 0; i < kPktTxCount; i++) {
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress1);
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress2);
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress3);
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress4);
    }
    int counter = getHwSwitch()->getBcmQcmMgr()->getIfpStatCounter(
        masterLogicalPortIds()[1]);

    // we learn 4 flows after first hit, but 3 hits after that
    // so its 9x3 + 4
    EXPECT_EQ(counter, 31);
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 1);
  };

  // TODO: rohitpuri
  // Enable warmboot once QCM supports it CS00010434741
  // verifyAcrossWarmBoots(setup, verify);
  setup();
  verify();
}

// Intent of this test is to enable QCM, do flow learning
// // stop qcm and ensure that flow tracker is disabled,
// // enable QCM again and ensure flow learning happens as desired
TEST_F(BcmQcmDataTest, VerifyQcmStopStart) {
  if (!isQcmSupported()) {
    return;
  }
  auto setup = [=]() { setupHelper(); };
  auto setupQcm = [=](const bool qcmEnable) { setupQcmOnly(qcmEnable); };
  auto verify = [&]() {
    for (int i = 0; i < kPktTxCount; i++) {
      sendL3Packet(
          kIPv6FlowDstAddress, masterLogicalPortIds()[1], kIPv6FlowSrcAddress1);
    }
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 1);
  };
  auto verifyStop = [&]() {
    EXPECT_TRUE(getHwSwitch()->getBcmQcmMgr()->isFlowTrackerDisabled());
  };

  // run with qcm enabled
  setup();
  verify();

  // run setup with qcm disabled
  bool qcmEnable = false;
  setupQcm(qcmEnable);
  verifyStop();

  // enable qcm again
  qcmEnable = true;
  setupQcm(qcmEnable);
  verify();
}
} // namespace facebook::fboss
