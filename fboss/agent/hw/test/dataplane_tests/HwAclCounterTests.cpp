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
#include "fboss/agent/hw/test/HwTestFlowletSwitchingUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/lib/CommonUtils.h"

namespace {
enum AclType {
  TCP_TTLD,
  UDP_TTLD,
  SRC_PORT,
  UDF,
  FLOWLET,
};
}

namespace facebook::fboss {

template <bool enableMultiAclTable>
struct EnableMultiAclTableT {
  static constexpr auto multiAclTableEnabled = enableMultiAclTable;
};

using TestTypes =
    ::testing::Types<EnableMultiAclTableT<false>, EnableMultiAclTableT<true>>;

template <typename EnableMultiAclTableT>
class HwAclCounterTest : public HwLinkStateDependentTest {
  static auto constexpr isMultiAclEnabled =
      EnableMultiAclTableT::multiAclTableEnabled;

 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = isMultiAclEnabled;
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (isMultiAclEnabled) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
      utility::addDefaultAclTable(cfg);
    }
    return cfg;
  }

  std::string getAclName(AclType aclType) const {
    std::string aclName{};
    switch (aclType) {
      case AclType::SRC_PORT:
        aclName = "test-acl";
        break;
      case AclType::TCP_TTLD:
        aclName = "test-tcp-acl";
        break;
      case AclType::UDP_TTLD:
        aclName = "test-udp-acl";
        break;
      case AclType::UDF:
        aclName = "test-udf-acl";
        break;
      case AclType::FLOWLET:
        aclName = "test-flowlet-acl";
        break;
    }
    return aclName;
  }

  std::string getCounterName(AclType aclType) const {
    std::string counterName{};
    switch (aclType) {
      case AclType::SRC_PORT:
        counterName = "test-acl-stats";
        break;
      case AclType::TCP_TTLD:
        counterName = "test-tcp-acl-stats";
        break;
      case AclType::UDP_TTLD:
        counterName = "test-udp-acl-stats";
        break;
      case AclType::UDF:
        counterName = "test-udf-acl-stats";
        break;
      case AclType::FLOWLET:
        counterName = "test-flowlet-acl-stats";
        break;
    }
    return counterName;
  }

  void counterBumpOnHitHelper(
      bool bumpOnHit,
      bool frontPanel,
      std::vector<AclType> aclTypes) {
    auto setup = [this, aclTypes]() {
      applyNewState(helper_->resolveNextHops(getProgrammedState(), 2));
      helper_->programRoutes(getRouteUpdater(), kEcmpWidth);
      auto newCfg{initialConfig()};
      for (auto aclType : aclTypes) {
        // FLOWLET Acl config already added in addFlowletConfigs() as part of
        // initial config setup. Hence skipping it here.
        if (aclType != AclType::FLOWLET) {
          addAclAndStat(&newCfg, aclType);
        }
      }
      applyNewConfig(newCfg);
    };

    auto verifyAclType = [this, bumpOnHit, frontPanel](AclType aclType) {
      auto egressPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
      auto pktsBefore = *getLatestPortStats(egressPort).outUnicastPkts__ref();
      auto aclPktCountBefore = utility::getAclInOutPackets(
          getHwSwitch(),
          getProgrammedState(),
          getAclName(aclType),
          getCounterName(aclType));

      auto aclBytesCountBefore = utility::getAclInOutBytes(
          getHwSwitch(),
          getProgrammedState(),
          getAclName(aclType),
          getCounterName(aclType));
      size_t sizeOfPacketSent = 0;
      auto sendRoce = false;
      if (aclType == AclType::FLOWLET && FLAGS_flowletSwitchingEnable) {
        XLOG(DBG3) << "setting ECMP Member Status: ";
        utility::setEcmpMemberStatus(getHwSwitch());
        sendRoce = true;
      }
      if (aclType == AclType::UDF) {
        sendRoce = true;
      }
      // for udf or flowlet testing, send roce packets
      if (sendRoce) {
        auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
        sizeOfPacketSent = sendRoceTraffic(outPort);
      } else {
        sizeOfPacketSent = sendPacket(frontPanel, bumpOnHit, aclType);
      }
      WITH_RETRIES({
        auto aclPktCountAfter = utility::getAclInOutPackets(
            getHwSwitch(),
            getProgrammedState(),
            getAclName(aclType),
            getCounterName(aclType));

        auto aclBytesCountAfter = utility::getAclInOutBytes(
            getHwSwitch(),
            getProgrammedState(),
            getAclName(aclType),
            getCounterName(aclType));

        auto pktsAfter = *getLatestPortStats(egressPort).outUnicastPkts__ref();
        XLOG(DBG2) << "\n"
                   << "PacketCounter: " << pktsBefore << " -> " << pktsAfter
                   << "\n"
                   << "aclPacketCounter(" << getCounterName(aclType)
                   << "): " << aclPktCountBefore << " -> " << (aclPktCountAfter)
                   << "\n"
                   << "aclBytesCounter(" << getCounterName(aclType)
                   << "): " << aclBytesCountBefore << " -> "
                   << aclBytesCountAfter;

        if (bumpOnHit) {
          EXPECT_EVENTUALLY_GT(pktsAfter, pktsBefore);
          // On some ASICs looped back pkt hits the ACL before being
          // dropped in the ingress pipeline, hence GE
          EXPECT_EVENTUALLY_GE(aclPktCountAfter, aclPktCountBefore + 1);
          // At most we should get a pkt bump of 2
          EXPECT_EVENTUALLY_LE(aclPktCountAfter, aclPktCountBefore + 2);
          EXPECT_EVENTUALLY_GE(
              aclBytesCountAfter, aclBytesCountBefore + sizeOfPacketSent);
          // On native BCM we see 4 extra bytes in the acl counter. This is
          // likely due to ingress vlan getting imposed and getting counted
          // when packet hits acl in ingress pipeline
          EXPECT_EVENTUALLY_LE(
              aclBytesCountAfter,
              aclBytesCountBefore + (2 * sizeOfPacketSent) + 4);
        } else {
          EXPECT_EVENTUALLY_EQ(aclPktCountBefore, aclPktCountAfter);
          EXPECT_EVENTUALLY_EQ(aclBytesCountBefore, aclBytesCountAfter);
        }
      });
    };

    auto verify = [aclTypes, verifyAclType]() {
      for (auto aclType : aclTypes) {
        // since FLOWLET Acl presents ahead of UDF Acl in TCAM
        // the packet always hit the FLOWLET Acl. Hence verify the FLOWLET Acl
        if (aclTypes[0] == AclType::FLOWLET) {
          aclType = AclType::FLOWLET;
        }
        verifyAclType(aclType);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  size_t sendRoceTraffic(const PortID frontPanelEgrPort) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    return utility::pumpRoCETraffic(
        true,
        getHwSwitch(),
        intfMac,
        vlanId,
        frontPanelEgrPort,
        utility::kUdfL4DstPort,
        255,
        std::nullopt,
        1 /* one packet */);
  }

  size_t sendPacket(bool frontPanel, bool bumpOnHit, AclType aclType) {
    // TTL is configured for value >= 128
    auto ttl = bumpOnHit &&
            (aclType == AclType::UDP_TTLD || aclType == AclType::TCP_TTLD)
        ? 200
        : 10;
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    auto txPacket = aclType == AclType::UDP_TTLD ? utility::makeUDPTxPacket(
                                                       getHwSwitch(),
                                                       vlanId,
                                                       srcMac, // src mac
                                                       intfMac, // dst mac
                                                       kSrcIP(),
                                                       kDstIP(),
                                                       8000, // l4 src port
                                                       8001, // l4 dst port
                                                       0,
                                                       ttl)
                                                 : utility::makeTCPTxPacket(
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
    auto aclName = getAclName(aclType);
    auto counterName = getCounterName(aclType);
    auto acl = utility::addAcl(config, aclName);
    switch (aclType) {
      case AclType::TCP_TTLD:
      case AclType::UDP_TTLD:
        acl->srcIp() = "2620:0:1cfe:face:b00c::/64";
        acl->proto() = aclType == AclType::UDP_TTLD ? 17 : 6;
        acl->ipType() = cfg::IpType::IP6;
        acl->ttl() = cfg::Ttl();
        *acl->ttl()->value() = 128;
        *acl->ttl()->mask() = 128;
        break;
      case AclType::SRC_PORT:
        acl->srcPort() = helper_->ecmpPortDescriptorAt(0).phyPortID();
        break;
      case AclType::UDF:
        acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
        acl->roceOpcode() = utility::kUdfRoceOpcode;
        break;
      case AclType::FLOWLET:
        break;
    }
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::addAclStat(config, aclName, counterName, setCounterTypes);
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

TYPED_TEST_SUITE(HwAclCounterTest, TestTypes);

// Verify that traffic arrive on a front panel port increments ACL counter.
TYPED_TEST(HwAclCounterTest, VerifyCounterBumpOnTtlHitFrontPanel) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

TYPED_TEST(HwAclCounterTest, VerifyCounterBumpOnSportHitFrontPanel) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::SRC_PORT});
}
// Verify that traffic originating on the CPU increments ACL counter.
TYPED_TEST(HwAclCounterTest, VerifyCounterBumpOnTtlHitCpu) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */,
      false /* cpu port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

TYPED_TEST(HwAclCounterTest, VerifyCounterBumpOnSportHitCpu) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */, false /* cpu port */, {AclType::SRC_PORT});
}

// Verify that traffic arrive on a front panel port increments ACL counter.
TYPED_TEST(HwAclCounterTest, VerifyCounterNoTtlHitNoBumpFrontPanel) {
  this->counterBumpOnHitHelper(
      false /* no hit, no bump */,
      true /* front panel port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

// Verify that traffic originating on the CPU increments ACL counter.
TYPED_TEST(HwAclCounterTest, VerifyCounterNoHitNoBumpCpu) {
  this->counterBumpOnHitHelper(
      false /* no hit, no bump */,
      false /* cpu port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

/*
 * UDF Acls are not supported on SAI and multi ACL. So we only test with
 * multi acl disabled for now.
 */
class HwUdfAclCounterTest
    : public HwAclCounterTest<EnableMultiAclTableT<false>> {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    cfg.udfConfig() = utility::addUdfAclConfig();
    return cfg;
  }
};

TEST_F(HwUdfAclCounterTest, VerifyUdf) {
  counterBumpOnHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::UDF});
}

TEST_F(HwUdfAclCounterTest, VerifyUdfWithOtherAcls) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF, AclType::SRC_PORT});
}

/*
 * Flowlet Acls are not supported on SAI and multi ACL. So we only test with
 * multi acl disabled for now.
 */
class HwFlowletAclCounterTest
    : public HwAclCounterTest<EnableMultiAclTableT<false>> {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    utility::addFlowletConfigs(cfg, masterLogicalPortIds());
    cfg.udfConfig() = utility::addUdfAclConfig();
    return cfg;
  }

  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    HwAclCounterTest::SetUp();
  }
};

TEST_F(HwFlowletAclCounterTest, VerifyFlowlet) {
  counterBumpOnHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::FLOWLET});
}

TEST_F(HwFlowletAclCounterTest, VerifyFlowletWithOtherAcls) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::FLOWLET, AclType::SRC_PORT});
}

// Verifying the FLOWLET Acl always hit ahead of UDF Acl
// when FLOWLET Acl present before UDF Acl
TEST_F(HwFlowletAclCounterTest, VerifyFlowletWithUdf) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::FLOWLET, AclType::UDF});
}

} // namespace facebook::fboss
