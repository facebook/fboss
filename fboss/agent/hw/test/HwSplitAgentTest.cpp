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
} // namespace facebook::fboss
