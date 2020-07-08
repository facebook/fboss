/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmLogBuffer.h"
#include "fboss/agent/hw/bcm/BcmQcmCollector.h"
#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace {
constexpr int kPktTxCount = 10;
constexpr int kAclPktTxCount = 100;
const folly::IPAddressV6 kIPv6Route = folly::IPAddressV6("2401::201:ab00");
const folly::IPAddress kIPv6FlowDstAddress =
    folly::IPAddressV6("2401::201:ab01");

const std::array<folly::IPAddress, 4> kIPv6FlowSrcAddress{
    folly::IPAddressV6("1::9"),
    folly::IPAddressV6("1::10"),
    folly::IPAddressV6("1::11"),
    folly::IPAddressV6("1::12"),
};

const folly::IPAddress kIPv4FlowSrcAddress = folly::IPAddressV4("10.0.0.1");

const std::string kCollectorSrcIpv4 = "10.0.0.2/32";
const std::string kCollectorDstIpv4 = "11.0.0.2/32";

const std::string kCollectorSrcIpv6 = "2401::301:ab01/128";
const std::string kCollectorDstIpv6 = "2401::401:ab01/128";
int kCollectorUDPSrcPort = 20000;
const int kMaskV6 = 120;

const std::string kCollectorAclCounter = "collectorAclCounter";
const std::string kCollectorAcl = "testCollectorAcl";
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

  cfg::QcmConfig getQcmConfig(bool isIpv6 = false) {
    auto qcmCfg = cfg::QcmConfig();
    // some dummy address
    if (isIpv6) {
      qcmCfg.collectorDstIp = kCollectorDstIpv6;
      qcmCfg.collectorSrcIp = kCollectorSrcIpv6;
    } else {
      qcmCfg.collectorDstIp = kCollectorDstIpv4;
      qcmCfg.collectorSrcIp = kCollectorSrcIpv4;
    }
    qcmCfg.collectorSrcPort = kCollectorUDPSrcPort;
    qcmCfg.agingIntervalInMsecs = 1000;
    qcmCfg.numFlowSamplesPerView = 1;
    return qcmCfg;
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    newCfg.qcmConfig_ref() = getQcmConfig();
    newCfg.switchSettings.qcmEnable = true;
    applyNewConfig(newCfg);
    cfg_ = newCfg;

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

  std::tuple<int, int> createQcmFlows(int numFlows, int pktCount) {
    int port1Counter = 0, port0Counter = 0;
    // monitor following 2 ports
    getHwSwitch()->getBcmQcmMgr()->updateQcmMonitoredPortsIfNeeded(
        {masterLogicalPortIds()[0], masterLogicalPortIds()[1]},
        true /* attach stat */);
    if (numFlows > kIPv6FlowSrcAddress.size()) {
      XLOG(ERR) << "Flow count requested : " << numFlows
                << " more than pool of addresses. Count= "
                << kIPv6FlowSrcAddress.size();
      return {0, 0};
    }
    // send flows
    for (int i = 0; i < pktCount; i++) {
      for (int j = 0; j < numFlows; j++) {
        sendL3Packet(
            kIPv6FlowDstAddress,
            masterLogicalPortIds()[1],
            kIPv6FlowSrcAddress[j]);
      }
    }
    port1Counter = getHwSwitch()->getBcmQcmMgr()->getIfpStatCounter(
        masterLogicalPortIds()[1]);
    port0Counter = getHwSwitch()->getBcmQcmMgr()->getIfpStatCounter(
        masterLogicalPortIds()[0]);
    return {port0Counter, port1Counter};
  }

  void setUpHelperWithAcls(bool isIpv6) {
    auto newCfg{initialConfig()};
    newCfg.switchSettings.qcmEnable = true;

    auto qcmCfg = getQcmConfig(isIpv6);
    // add an acl
    auto* acl = utility::addAcl(&newCfg, kCollectorAcl);
    acl->dstMac_ref() = BcmQcmCollector::getCollectorDstMac().toString();
    if (!isIpv6) {
      acl->dstIp_ref() = kCollectorDstIpv4;
      acl->srcIp_ref() = kCollectorSrcIpv4;
    } else {
      acl->dstIp_ref() = kCollectorDstIpv6;
      acl->srcIp_ref() = kCollectorSrcIpv6;
    }
    acl->l4DstPort_ref() = qcmCfg.collectorDstPort; // pick the default
    utility::addAclStat(&newCfg, kCollectorAcl, kCollectorAclCounter);

    newCfg.qcmConfig_ref() = qcmCfg;
    applyNewConfig(newCfg);

    getHwSwitch()->getBcmQcmMgr()->updateQcmMonitoredPortsIfNeeded(
        {masterLogicalPortIds()[0], masterLogicalPortIds()[1]},
        true /* attach stat */);
    addRoute(kIPv6Route, kMaskV6, PortDescriptor(masterLogicalPortIds()[0]));
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
    const auto dummySrcIp = isV6 ? kIPv6FlowSrcAddress[0] : kIPv4FlowSrcAddress;
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

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper6_;
  cfg::SwitchConfig cfg_;
};

TEST_F(BcmQcmDataTest, VerifyQcmFirmwareInit) {
  if (!BcmQcmManager::isQcmSupported(getHwSwitch())) {
    // This test only applies to ceratin ASIC e.g. TH
    // and to specific sdk versions
    return;
  }

  // intent of this test is to load the QcmFirmware during
  // initialization (using flags above in SetUp) and ensure it goes through fine
  // Verify if the status of firmware is good
  auto setup = [=]() { setupHelper(); };

  auto verify = [this]() {
    BcmLogBuffer logBuffer;
    getHwSwitch()->printDiagCmd("mcsstatus Quick=true");
    std::string mcsStatusStr =
        logBuffer.getBuffer()->moveToFbString().toStdString();
    XLOG(INFO) << "MCStatus:" << mcsStatusStr;
    EXPECT_TRUE(mcsStatusStr.find("uC 0 status: OK") != std::string::npos);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to setup QCM tables
// send a flow and verify if the flow learning happens
// Only first pkt should hit the ifp entry, all subsequent pkts should
// not hit the ifp entry
// Since its 1 flow, we should only learn 1 flow
TEST_F(BcmQcmDataTest, FlowLearning) {
  if (!BcmQcmManager::isQcmSupported(getHwSwitch())) {
    return;
  }
  auto setup = [&]() { setupHelper(); };

  auto verify = [&]() {
    // send 1 flow only
    auto [nonFlowLearnPortCounter, flowLearnPortCounter] =
        createQcmFlows(1, kPktTxCount);
    EXPECT_EQ(flowLearnPortCounter, 1);
    EXPECT_EQ(nonFlowLearnPortCounter, kPktTxCount);
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 1);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to setup QCM tables
// send multple flows and check that learning happens
// for multiple flows
TEST_F(BcmQcmDataTest, MultiFlows) {
  if (!BcmQcmManager::isQcmSupported(getHwSwitch())) {
    return;
  }
  auto setup = [&]() { setupHelper(); };

  auto verify = [&]() {
    // send 2 flows
    auto [nonFlowLearnPortCounter, flowLearnPortCounter] =
        createQcmFlows(2, kPktTxCount);
    EXPECT_EQ(nonFlowLearnPortCounter, 2 * kPktTxCount);
    EXPECT_EQ(flowLearnPortCounter, 2);
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 2);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to setup QCM tables
// send multple flows, but restrict flow limit to 1
// Ensure that only 1 flow is learned
// also ifp entry should hit multiple times
TEST_F(BcmQcmDataTest, RestrictFlowLearning) {
  if (!BcmQcmManager::isQcmSupported(getHwSwitch())) {
    return;
  }
  auto setup = [&]() { setupHelper(); };

  auto verify = [&]() {
    // force the flow learning to 1 only
    getHwSwitch()->getBcmQcmMgr()->flowLimitSet(1);
    // send 4 flows
    auto [nonFlowLearnPortCounter, flowLearnPortCounter] =
        createQcmFlows(4, kPktTxCount);

    // we learn 4 flows after first hit, but 3 hits after that
    // so its 9x3 + 4
    EXPECT_EQ(flowLearnPortCounter, 31);
    EXPECT_EQ(nonFlowLearnPortCounter, 4 * kPktTxCount);
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 1);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to enable QCM, do flow learning
// Stop qcm and ensure that flow tracker is disabled,
// Enable QCM again and ensure flow learning happens as desired
TEST_F(BcmQcmDataTest, VerifyQcmStopStart) {
  if (!BcmQcmManager::isQcmSupported(getHwSwitch())) {
    return;
  }
  auto setup = [=]() { setupHelper(); };
  auto setupQcm = [=](const bool qcmEnable) { setupQcmOnly(qcmEnable); };
  auto verify = [&]() {
    // send 1 flow
    auto [nonFlowLearnPortCounter, flowLearnPortCounter] =
        createQcmFlows(1, kPktTxCount);
    std::ignore = nonFlowLearnPortCounter;
    EXPECT_EQ(flowLearnPortCounter, 1);
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

class BcmQcmDataCollectorParamTest : public BcmQcmDataTest,
                                     public testing::WithParamInterface<bool> {
};

TEST_P(BcmQcmDataCollectorParamTest, VerifyFlowCollector) {
  if (!BcmQcmManager::isQcmSupported(getHwSwitch())) {
    return;
  }
  bool isIpv6 = GetParam();
  auto setup = [this, isIpv6]() { setUpHelperWithAcls(isIpv6); };
  auto verify = [&]() {
    auto [nonFlowLearnPortCounter, flowLearnPortCounter] =
        createQcmFlows(1, kAclPktTxCount);
    std::ignore = nonFlowLearnPortCounter;
    EXPECT_EQ(flowLearnPortCounter, 1);
    EXPECT_EQ(getHwSwitch()->getBcmQcmMgr()->getLearnedFlowCount(), 1);
    auto statHandle = getHwSwitch()
                          ->getAclTable()
                          ->getAclStat(kCollectorAclCounter)
                          ->getHandle();
    auto aclCounter = utility::getAclInOutPackets(getUnit(), statHandle);
    EXPECT_GT(aclCounter, 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

INSTANTIATE_TEST_CASE_P(
    BcmQcmDataCollectorParamTestInstantitation,
    BcmQcmDataCollectorParamTest,
    ::testing::Values(false, true));
} // namespace facebook::fboss
