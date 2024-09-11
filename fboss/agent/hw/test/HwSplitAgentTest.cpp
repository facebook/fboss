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
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class HwSplitAgentCallbackTest : public HwLinkStateDependentTest {
 protected:
  void linkStateChanged(PortID port, bool up) override {
    std::lock_guard<std::mutex> lk(linkEventMutex_);
    if (!portIdToWaitFor_ || port != portIdToWaitFor_ || up != waitForPortUp_) {
      return;
    }
    portIdToWaitFor_ = std::nullopt;
    desiredPortEventOccurred_ = true;
    linkEventCV_.notify_one();
  }

  void setPortIDAndStateToWaitFor(PortID port, bool waitForPortUp) {
    std::lock_guard<std::mutex> lk(linkEventMutex_);
    portIdToWaitFor_ = port;
    waitForPortUp_ = waitForPortUp;
    desiredPortEventOccurred_ = false;
  }

  bool waitForPortEvent() {
    std::unique_lock<std::mutex> lock{linkEventMutex_};
    linkEventCV_.wait_for(lock, std::chrono::seconds(30), [this] {
      return desiredPortEventOccurred_;
    });
    return desiredPortEventOccurred_;
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
  std::condition_variable linkEventCV_;
  std::mutex linkEventMutex_;
  std::optional<PortID> portIdToWaitFor_;
  bool waitForPortUp_{false};
  bool desiredPortEventOccurred_{false};
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
  txPacket.length() = pkt->buf()->computeChainDataLength();
  txPacket.data() = Packet::extractIOBuf(std::move(pkt));

  auto statBefore =
      utility::getPortOutPkts(getLatestPortStats(masterLogicalPortIds()[0]));
  getHwSwitchEnsemble()->enqueueTxPacket(std::move(txPacket));
  WITH_RETRIES({
    auto statAfter =
        utility::getPortOutPkts(getLatestPortStats(masterLogicalPortIds()[0]));
    EXPECT_EVENTUALLY_EQ(statAfter - statBefore, 1);
  });
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
  operDelta.operDelta() = StateDelta(state, newState).getOperDelta();
  getHwSwitchEnsemble()->enqueueOperDelta(operDelta);
  EXPECT_TRUE(waitForPortEvent());

  // Set port to up
  setPortIDAndStateToWaitFor(masterLogicalInterfacePortIds()[0], true);
  multiswitch::StateOperDelta operDelta2;
  operDelta2.operDelta() = StateDelta(newState, state).getOperDelta();
  getHwSwitchEnsemble()->enqueueOperDelta(operDelta2);
  EXPECT_TRUE(waitForPortEvent());
}
} // namespace facebook::fboss
