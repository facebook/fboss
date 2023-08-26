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
#include <chrono>

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook::fboss {

class HwSplitAgentCallbackTest : public HwLinkStateDependentTest {
 public:
  class TestToggler : public HwLinkStateToggler {
   public:
    TestToggler() : HwLinkStateToggler(nullptr) {}
    void invokeLinkScanIfNeeded(PortID /*port*/, bool /*isUp*/) override {}
    void setPortPreemphasis(
        const std::shared_ptr<Port>& /*port*/,
        int /*preemphasis*/) override {}
    void setLinkTraining(const std::shared_ptr<Port>& /*port*/, bool /*enable*/)
        override {}
  };

  void SetUp() override {
    toggler_ = std::make_unique<TestToggler>();
    HwLinkStateDependentTest::SetUp();
  }

 protected:
  void linkStateChanged(PortID port, bool up) override {
    toggler_->linkStateChanged(port, up);
  }

  void setPortIDAndStateToWaitFor(PortID port, bool waitForPortUp) {
    toggler_->setPortIDAndStateToWaitFor(port, waitForPortUp);
  }

  bool waitForPortEvent() {
    return toggler_->waitForPortEvent();
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {
        HwSwitchEnsemble::PACKET_RX,
        HwSwitchEnsemble::MULTISWITCH_THRIFT_SERVER,
        HwSwitchEnsemble::LINKSCAN};
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }

  bool skipTest() {
    return (
        getPlatform()->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_FAKE ||
        getPlatform()->getAsic()->getAsicType() ==
            cfg::AsicType::ASIC_TYPE_MOCK);
  }

 private:
  std::unique_ptr<HwLinkStateToggler> toggler_;
};

TEST_F(HwSplitAgentCallbackTest, linkCallback) {
  if (skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  EXPECT_NE(getHwSwitchEnsemble()->getTestServer()->getPort(), 0);
  setPortIDAndStateToWaitFor(masterLogicalInterfacePortIds()[0], false);
  bringDownPort(masterLogicalInterfacePortIds()[0]);
  EXPECT_TRUE(waitForPortEvent());
  setPortIDAndStateToWaitFor(masterLogicalInterfacePortIds()[0], true);
  bringUpPort(masterLogicalInterfacePortIds()[0]);
  EXPECT_TRUE(waitForPortEvent());
}

TEST_F(HwSplitAgentCallbackTest, txPacket) {
  if (skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  resolveNeigborAndProgramRoutes(
      utility::EcmpSetupAnyNPorts4(getProgrammedState(), RouterID(0)), 1);

  auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
  auto vlanId = utility::firstVlanID(initialConfig());
  auto pkt = utility::makeIpTxPacket(
      [hwSwitch = getHwSwitch()](uint32_t size) {
        return hwSwitch->allocatePacket(size);
      },
      vlanId,
      intfMac,
      intfMac,
      folly::IPAddressV4("1.0.0.1"),
      folly::IPAddressV4("1.0.0.2"));
  multiswitch::TxPacket txPacket;
  txPacket.data() = Packet::extractIOBuf(std::move(pkt));

  auto statBefore =
      getPortOutPkts(getLatestPortStats(masterLogicalPortIds()[0]));
  getHwSwitchEnsemble()->enqueueTxPacket(std::move(txPacket));
  auto statAfter =
      getPortOutPkts(getLatestPortStats(masterLogicalPortIds()[0]));
  EXPECT_EQ(statAfter - statBefore, 1);
}

TEST_F(HwSplitAgentCallbackTest, operDeltaUpdate) {
  if (skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  setPortIDAndStateToWaitFor(masterLogicalInterfacePortIds()[0], false);
  auto state = getProgrammedState();
  auto newState = state->clone();
  auto port = newState->getPorts()
                  ->getNodeIf(PortID(masterLogicalInterfacePortIds()[0]))
                  ->modify(&newState);

  // set port to down
  port->setOperState(false);
  port->setLoopbackMode(cfg::PortLoopbackMode::NONE);
  multiswitch::StateOperDelta operDelta;
  operDelta.transaction() = true;
  operDelta.operDelta() = StateDelta(state, newState).getOperDelta();
  getHwSwitchEnsemble()->enqueueOperDelta(operDelta);
  EXPECT_TRUE(waitForPortEvent());

  // Set port to up
  setPortIDAndStateToWaitFor(masterLogicalInterfacePortIds()[0], true);
  multiswitch::StateOperDelta operDelta2;
  operDelta2.transaction() = true;
  operDelta2.operDelta() = StateDelta(newState, state).getOperDelta();
  getHwSwitchEnsemble()->enqueueOperDelta(operDelta2);
  EXPECT_TRUE(waitForPortEvent());
}
} // namespace facebook::fboss
