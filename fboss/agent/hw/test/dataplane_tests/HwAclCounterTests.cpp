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
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class HwAclCounterTest : public HwLinkStateDependentTest {
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
  enum AclType {
    TTLD,
    SRC_PORT,
  };

  void counterBumpOnHitHelper(
      bool bumpOnHit,
      bool frontPanel,
      AclType aclType = AclType::TTLD) {
    auto setup = [this, aclType]() {
      applyNewState(helper_->resolveNextHops(getProgrammedState(), 2));
      helper_->programRoutes(getRouteUpdater(), kEcmpWidth);
      auto newCfg{initialConfig()};
      addAclAndStat(&newCfg, aclType);
      applyNewConfig(newCfg);
    };

    auto verify = [this, bumpOnHit, frontPanel, aclType]() {
      auto egressPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
      auto pktsBefore = *getLatestPortStats(egressPort).outUnicastPkts__ref();
      auto aclPktCountBefore = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

      auto aclBytesCountBefore = utility::getAclInOutBytes(
          getHwSwitch(), getProgrammedState(), kAclName, kCounterName);
      size_t sizeOfPacketSent = sendPacket(frontPanel, bumpOnHit, aclType);
      WITH_RETRIES({
        auto aclPktCountAfter = utility::getAclInOutPackets(
            getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

        auto aclBytesCountAfter = utility::getAclInOutBytes(
            getHwSwitch(), getProgrammedState(), kAclName, kCounterName);

        auto pktsAfter = *getLatestPortStats(egressPort).outUnicastPkts__ref();
        XLOG(DBG2) << "\n"
                   << "PacketCounter: " << pktsBefore << " -> " << pktsAfter
                   << "\n"
                   << "aclPacketCounter: " << aclPktCountBefore << " -> "
                   << (aclPktCountAfter) << "\n"
                   << "aclBytesCounter: " << aclBytesCountBefore << " -> "
                   << aclBytesCountAfter;

        EXPECT_EVENTUALLY_GT(pktsAfter, pktsBefore);
        if (bumpOnHit) {
          // On VOQ switches, we see a counter bump by 1, for the time the
          // packet is routed out. The looped packet which gets dropped does
          // not seem to incur a counter bump. On non VOQ switches OTOH,
          // we see a bump by 2 once on the way out and once when it loops back
          // in.
          auto pktsHit =
              getAsic()->getSwitchType() == cfg::SwitchType::VOQ ? 1 : 2;
          EXPECT_EVENTUALLY_EQ(aclPktCountBefore + pktsHit, aclPktCountAfter);

          // TODO: Still need to debug. For some test cases, we are getting more
          // bytes in aclCounter. Ex. 4 Bytes extra in Tomahawk4 tests.
          EXPECT_EVENTUALLY_LE(
              aclBytesCountBefore + (pktsHit * sizeOfPacketSent),
              aclBytesCountAfter);
        } else {
          EXPECT_EVENTUALLY_EQ(aclPktCountBefore, aclPktCountAfter);
          EXPECT_EVENTUALLY_EQ(aclBytesCountBefore, aclBytesCountAfter);
        }
      });
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  size_t sendPacket(bool frontPanel, bool bumpOnHit, AclType aclType) {
    // TTL is configured for value >= 128
    auto ttl = bumpOnHit && aclType == AclType::TTLD ? 200 : 10;
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
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

  void addAclAndStat(cfg::SwitchConfig* config, AclType aclType) const {
    auto acl = utility::addAcl(config, kAclName);
    switch (aclType) {
      case AclType::TTLD:
        acl->srcIp() = "2620:0:1cfe:face:b00c::/64";
        acl->proto() = 0x11;
        acl->ipType() = cfg::IpType::IP6;
        acl->ttl() = cfg::Ttl();
        *acl->ttl()->value() = 128;
        *acl->ttl()->mask() = 128;
        break;
      case AclType::SRC_PORT:
        // TODO
        break;
    }
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::addAclStat(config, kAclName, kCounterName, setCounterTypes);
  }

  static inline constexpr auto kEcmpWidth = 1;
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
  static constexpr auto kAclName = "test-acl";
  static constexpr auto kCounterName = "test-acl-stat";
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
