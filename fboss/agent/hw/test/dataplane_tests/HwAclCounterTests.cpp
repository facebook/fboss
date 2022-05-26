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
      auto pktCountBefore = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

      auto bytesCountBefore = utility::getAclInOutBytes(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);
      // TTL is configured for value >= 128
      auto ttl = bumpOnHit ? 200 : 10;
      size_t sizeOfPacketSent = sendPacket(frontPanel, ttl);

      auto pktCountAfter = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

      auto bytesCountAfter = utility::getAclInOutBytes(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

      XLOG(INFO) << "\n"
                 << "aclPacketCounter: " << std::to_string(pktCountBefore)
                 << " -> " << std::to_string(pktCountAfter) << "\n"
                 << "aclBytesCounter: " << std::to_string(bytesCountBefore)
                 << " -> " << std::to_string(bytesCountAfter);

      if (bumpOnHit) {
        // Bump by 2, once on the way out and once when it loops back in
        // Bytes count is twice the size of the packet, since the bytes counter
        // is increased on the way out and in.
        EXPECT_EQ(pktCountBefore + 2, pktCountAfter);

        // TODO: Still need to debug. For some test cases, we are getting more
        // bytes in aclCounter. Ex. 4 Bytes extra in Tomahawk4 tests.
        EXPECT_LE(bytesCountBefore + (2 * sizeOfPacketSent), bytesCountAfter);
      } else {
        EXPECT_EQ(pktCountBefore, pktCountAfter);
        EXPECT_EQ(bytesCountBefore, bytesCountAfter);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  size_t sendPacket(bool frontPanel, uint8_t ttl) {
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
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

    size_t txPacketSize = txPacket->buf()->length();
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

    return txPacketSize;
  }

  folly::IPAddressV6 kSrcIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::1");
  }

  folly::IPAddressV6 kDstIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::10");
  }

  void addTtlAclStat(cfg::SwitchConfig* config) const {
    auto acl = utility::addAcl(config, kAclName);
    acl->srcIp() = "2620:0:1cfe:face:b00c::/64";
    acl->proto() = 0x11;
    acl->ipType() = cfg::IpType::IP6;
    acl->ttl() = cfg::Ttl();
    *acl->ttl()->value() = 128;
    *acl->ttl()->mask() = 128;
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::addAclStat(config, kAclName, kCounterName, setCounterTypes);
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
