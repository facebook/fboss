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
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook::fboss {

class HwAclCounterTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void counterBumpOnHitHelper(bool bumpOnHit, bool frontPanel) {
    auto setup = [this]() {
      applyNewState(helper_->resolveNextHops(getProgrammedState(), 2));
      helper_->programRoutes(getRouteUpdater(), kEcmpWidth);
      auto newCfg{initialConfig()};
      addTtlAclStat(&newCfg);
      applyNewConfig(newCfg);
    };

    auto verify = [this, bumpOnHit, frontPanel]() {
      auto statBefore = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

      // TTL is configured for value >= 128
      auto ttl = bumpOnHit ? 200 : 10;
      sendPacket(frontPanel, ttl);

      auto statAfter = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

      if (bumpOnHit) {
        // Bump by 2, once on the way out and once when it loops back in
        EXPECT_EQ(statBefore + 2, statAfter);
      } else {
        EXPECT_EQ(statBefore, statAfter);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPacket(bool frontPanel, uint8_t ttl) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        kSrcIP(),
        kDstIP(),
        8000, // l4 src port
        8001, // l4 dst port
        0,
        ttl);

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

  folly::IPAddressV6 kSrcIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::1");
  }

  folly::IPAddressV6 kDstIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::10");
  }

  void addTtlAclStat(cfg::SwitchConfig* config) const {
    auto acl = utility::addAcl(config, kAclName);
    acl->srcIp_ref() = "2620:0:1cfe:face:b00c::/64";
    acl->proto_ref() = 0x11;
    acl->ipType_ref() = cfg::IpType::IP6;
    acl->ttl_ref() = cfg::Ttl();
    *acl->ttl_ref()->value_ref() = 128;
    *acl->ttl_ref()->mask_ref() = 128;
    utility::addAclStat(config, kAclName, kCounterName);
  }

  static inline constexpr auto kEcmpWidth = 1;
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
  static constexpr auto kAclName = "ttld-acl";
  static constexpr auto kCounterName = "ttld-stat";
};

// Verify that traffic arrive on a front panel port increments ACL counter.
TEST_F(HwAclCounterTest, VerifyCounterBumpOnHitFrontPanel) {
  counterBumpOnHitHelper(true /* bump on hit */, true /* front panel port */);
}

// Verify that traffic originating on the CPU increments ACL counter.
TEST_F(HwAclCounterTest, VerifyCounterBumpOnHitCpu) {
  counterBumpOnHitHelper(true /* bump on hit */, false /* cpu port */);
}

// Verify that traffic arrive on a front panel port increments ACL counter.
TEST_F(HwAclCounterTest, VerifyCounterNoHitNoBumpFrontPanel) {
  counterBumpOnHitHelper(
      false /* no hit, no bump */, true /* front panel port */);
}

// Verify that traffic originating on the CPU increments ACL counter.
TEST_F(HwAclCounterTest, VerifyCounterNoHitNoBumpCpu) {
  counterBumpOnHitHelper(false /* no hit, no bump */, false /* cpu port */);
}

} // namespace facebook::fboss
