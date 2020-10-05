/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/Utility.h"

#include <folly/IPAddress.h>
#include <folly/container/Array.h>

namespace facebook::fboss {

class HwPacketSendTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
    return cfg;
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

TEST_F(HwPacketSendTest, ArpRequestToFrontPanelPortSwitched) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto randomIP = folly::IPAddressV4("1.1.1.5");
    auto txPacket = utility::makeARPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"),
        folly::IPAddress("1.1.1.2"),
        randomIP,
        ARP_OPER::ARP_OPER_REQUEST,
        std::nullopt);
    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(INFO) << "ARP Packet:"
               << " before pkts:" << *portStatsBefore.outBroadcastPkts__ref()
               << ", after pkts:" << *portStatsAfter.outBroadcastPkts__ref()
               << ", before bytes:" << *portStatsBefore.outBytes__ref()
               << ", after bytes:" << *portStatsAfter.outBytes__ref();
    EXPECT_NE(
        0, *portStatsAfter.outBytes__ref() - *portStatsBefore.outBytes__ref());
    if (getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TAJO) {
      EXPECT_EQ(
          1,
          *portStatsAfter.outBroadcastPkts__ref() -
              *portStatsBefore.outBroadcastPkts__ref());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
