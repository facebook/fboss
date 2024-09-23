/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(flowletSwitchingEnable);

namespace {
enum AclType {
  TCP_TTLD,
  UDP_TTLD,
  SRC_PORT,
  SRC_PORT_DENY,
  L4_DST_PORT,
  UDF_OPCODE_ACK,
  UDF_OPCODE_WRITE_IMMEDIATE,
  BTH_OPCODE,
  FLOWLET,
  // UDF flowlet also matches on udf in addtion to FLOWLET fields
  UDF_FLOWLET,
};
}

namespace facebook::fboss {

class AgentAclCounterTest : public AgentHwTest {
 public:
  cfg::AclActionType aclActionType_ = cfg::AclActionType::PERMIT;
  uint8_t roceReservedByte_ = utility::kRoceReserved;

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if (!FLAGS_enable_acl_table_group) {
      return {
          production_features::ProductionFeature::ACL_COUNTER,
          production_features::ProductionFeature::SINGLE_ACL_TABLE};
    } else {
      return {
          production_features::ProductionFeature::ACL_COUNTER,
          production_features::ProductionFeature::MULTI_ACL_TABLE};
    }
  }

 protected:
  void SetUp() override {
    AgentHwTest::SetUp();
    if (IsSkipped()) {
      return;
    }
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    if (FLAGS_enable_acl_table_group) {
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
      case AclType::SRC_PORT_DENY:
        aclName = "test-deny-acl";
        break;
      case AclType::TCP_TTLD:
        aclName = "test-tcp-acl";
        break;
      case AclType::UDP_TTLD:
        aclName = "test-udp-acl";
        break;
      case AclType::L4_DST_PORT:
        aclName = "test-l4-port-acl";
        break;
      case AclType::UDF_OPCODE_ACK:
        aclName = "test-udf-opcode-ack-acl";
        break;
      case AclType::UDF_OPCODE_WRITE_IMMEDIATE:
        aclName = "test-udf-opcode-write-immediate-acl";
        break;
      case AclType::BTH_OPCODE:
        aclName = "test-bth-opcode-acl";
        break;
      case AclType::FLOWLET:
        aclName = "test-flowlet-acl";
        break;
      case AclType::UDF_FLOWLET:
        aclName = utility::kFlowletAclName;
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
      case AclType::SRC_PORT_DENY:
        counterName = "test-deny-acl-stats";
        break;
      case AclType::TCP_TTLD:
        counterName = "test-tcp-acl-stats";
        break;
      case AclType::UDP_TTLD:
        counterName = "test-udp-acl-stats";
        break;
      case AclType::L4_DST_PORT:
        counterName = "test-l4-port-acl-stats";
        break;
      case AclType::UDF_OPCODE_ACK:
        counterName = "test-udf-opcode-ack-acl-stats";
        break;
      case AclType::UDF_OPCODE_WRITE_IMMEDIATE:
        counterName = "test-udf-opcode-write-immediate-acl-stats";
        break;
      case AclType::BTH_OPCODE:
        counterName = "test-bth-opcode-acl-stats";
        break;
      case AclType::FLOWLET:
        counterName = "test-flowlet-acl-stats";
        break;
      case AclType::UDF_FLOWLET:
        counterName = utility::kFlowletAclCounterName;
        break;
    }
    return counterName;
  }

  void counterBumpOnHitHelper(
      bool bumpOnHit,
      bool frontPanel,
      std::vector<AclType> aclTypes) {
    auto setup = [this, aclTypes]() {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return helper_->resolveNextHops(in, 2);
      });
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, kEcmpWidth);
      auto newCfg{initialConfig(*getAgentEnsemble())};
      for (auto aclType : aclTypes) {
        addAclAndStat(&newCfg, aclType);
      }
      applyNewConfig(newCfg);
    };

    auto verify = [this, bumpOnHit, frontPanel, aclTypes]() {
      for (auto aclType : aclTypes) {
        verifyAclType(bumpOnHit, frontPanel, aclType);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void counterBumpOnFlowletAclHitHelper(
      bool bumpOnHit,
      bool frontPanel,
      std::vector<AclType> aclTypes) {
    auto setup = [this, aclTypes]() {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return helper_->resolveNextHops(in, 2);
      });
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, kEcmpWidth);
      auto newCfg{initialConfig(*getAgentEnsemble())};
      for (auto aclType : aclTypes) {
        switch (aclType) {
          case AclType::FLOWLET:
          case AclType::UDF_FLOWLET:
            utility::addFlowletAcl(
                newCfg,
                getAclName(aclType),
                getCounterName(aclType),
                aclType != AclType::FLOWLET);
            break;
          default:
            addAclAndStat(&newCfg, aclType);
            break;
        }
      }
      applyNewConfig(newCfg);

      XLOG(DBG3) << "setting ECMP Member Status: ";
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        for (const auto& [_, switchSetting] :
             std::as_const(*out->getSwitchSettings())) {
          auto newSwitchSettings = switchSetting->modify(&out);
          newSwitchSettings->setForceEcmpDynamicMemberUp(true);
        }
        return out;
      });
    };

    auto verify = [this, bumpOnHit, frontPanel, aclTypes]() {
      // since FLOWLET Acl presents ahead of UDF Acl in TCAM
      // the packet always hit the FLOWLET Acl. Hence verify the FLOWLET Acl
      verifyAclType(bumpOnHit, frontPanel, aclTypes[0]);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  size_t sendRoceTraffic(const PortID frontPanelEgrPort, AclType aclType) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    uint8_t opcode = (aclType == AclType::UDF_OPCODE_WRITE_IMMEDIATE)
        ? utility::kUdfRoceOpcodeWriteImmediate
        : utility::kUdfRoceOpcodeAck;
    return utility::pumpRoCETraffic(
        true,
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        intfMac,
        vlanId,
        frontPanelEgrPort,
        utility::kUdfL4DstPort,
        255,
        std::nullopt,
        1 /* one packet */,
        opcode,
        this->roceReservedByte_);
  }

  size_t sendPacket(bool frontPanel, bool bumpOnHit, AclType aclType) {
    // TTL is configured for value >= 128
    auto ttl = bumpOnHit &&
            (aclType == AclType::UDP_TTLD || aclType == AclType::TCP_TTLD)
        ? 200
        : 10;
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    int l4DstPort = kL4DstPort();
    if (aclType == AclType::L4_DST_PORT) {
      l4DstPort = kL4DstPort2();
    }

    auto txPacket = aclType == AclType::UDP_TTLD ? utility::makeUDPTxPacket(
                                                       getSw(),
                                                       vlanId,
                                                       srcMac, // src mac
                                                       intfMac, // dst mac
                                                       kSrcIP(),
                                                       kDstIP(),
                                                       kL4SrcPort(),
                                                       l4DstPort,
                                                       0,
                                                       ttl)
                                                 : utility::makeTCPTxPacket(
                                                       getSw(),
                                                       vlanId,
                                                       srcMac, // src mac
                                                       intfMac, // dst mac
                                                       kSrcIP(),
                                                       kDstIP(),
                                                       kL4SrcPort(),
                                                       l4DstPort,
                                                       0,
                                                       ttl);

    size_t txPacketSize = txPacket->buf()->length();
    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }

    return txPacketSize;
  }

  folly::IPAddressV6 kSrcIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::1");
  }

  folly::IPAddressV6 kDstIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::10");
  }

  int kL4SrcPort() const {
    return 8000;
  }

  int kL4DstPort() const {
    return 8001;
  }

  int kL4DstPort2() const {
    return 8002;
  }

  // This test verifies if the ACL priorities are taking effect as expected.
  // ACLs are processed in the priority in which they are listed in the config
  // 1. Install PERMIT ACL matching on SRC_PORT
  // 2. Install DENY ACL matching on SRC_PORT
  //
  // The expectation here is both ACLs are hit and PERMIT ACL gets priority.
  void aclPriorityTestHelper() {
    auto setup = [this]() {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return helper_->resolveNextHops(in, 2);
      });
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, kEcmpWidth);
      auto newCfg{initialConfig(*getAgentEnsemble())};
      this->aclActionType_ = cfg::AclActionType::PERMIT;
      addAclAndStat(&newCfg, AclType::SRC_PORT);
      this->aclActionType_ = cfg::AclActionType::DENY;
      addAclAndStat(&newCfg, AclType::SRC_PORT_DENY);
      applyNewConfig(newCfg);
    };

    auto verify = [this]() {
      // The first parameter in both invocations is bumpOnHit.
      // True means the verifier checks if counter increment for the PERMIT ACL
      // False means the DENY ACL counter did not change.
      //
      // Higher priority PERMIT ACL counter went up
      verifyAclType(true, true, AclType::SRC_PORT);
      // Lower priority DENY ACL counter remains same
      verifyAclType(false, true, AclType::SRC_PORT_DENY);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void aclPriorityTestHelper2() {
    auto setup = [this]() {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return helper_->resolveNextHops(in, 2);
      });
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, kEcmpWidth);
      auto newCfg{initialConfig(*getAgentEnsemble())};
      // match on SRC_PORT=1 + L4_DST_PORT=8002
      this->aclActionType_ = cfg::AclActionType::PERMIT;
      addAclAndStat(&newCfg, AclType::L4_DST_PORT);
      // match on SRC_PORT=1
      this->aclActionType_ = cfg::AclActionType::DENY;
      addAclAndStat(&newCfg, AclType::SRC_PORT);
      applyNewConfig(newCfg);
    };

    auto verify = [this]() {
      // Sends a packet with dst port 8002
      verifyAclType(true, true, AclType::L4_DST_PORT);
      // Sends a packet with dst port 8001
      verifyAclType(true, true, AclType::SRC_PORT);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyAclType(bool bumpOnHit, bool frontPanel, AclType aclType) {
    auto egressPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
    auto pktsBefore =
        *getNextUpdatedPortStats(egressPort).outUnicastPkts__ref();
    auto aclPktCountBefore =
        utility::getAclInOutPackets(getSw(), getCounterName(aclType));
    auto aclBytesCountBefore = utility::getAclInOutPackets(
        getSw(), getCounterName(aclType), true /* bytes */);
    size_t sizeOfPacketSent = 0;

    // for udf or bth_opcode testing, send roce packets
    if (aclType == AclType::UDF_OPCODE_ACK ||
        aclType == AclType::UDF_OPCODE_WRITE_IMMEDIATE ||
        aclType == AclType::BTH_OPCODE || aclType == AclType::FLOWLET ||
        aclType == AclType::UDF_FLOWLET) {
      sizeOfPacketSent = sendRoceTraffic(egressPort, aclType);
    } else {
      sizeOfPacketSent = sendPacket(frontPanel, bumpOnHit, aclType);
    }
    WITH_RETRIES({
      auto aclPktCountAfter =
          utility::getAclInOutPackets(getSw(), getCounterName(aclType));

      auto aclBytesCountAfter = utility::getAclInOutPackets(
          getSw(), getCounterName(aclType), true /* bytes */);

      auto pktsAfter =
          *getNextUpdatedPortStats(egressPort).outUnicastPkts__ref();
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
        if (isSupportedOnAllAsics(HwAsic::Feature::ACL_BYTE_COUNTER)) {
          EXPECT_EVENTUALLY_GE(
              aclBytesCountAfter, aclBytesCountBefore + sizeOfPacketSent);
          //  On native BCM we see 4 extra bytes in the acl counter. This is
          //  likely due to ingress vlan getting imposed and getting counted
          //  when packet hits acl in ingress pipeline
          EXPECT_EVENTUALLY_LE(
              aclBytesCountAfter,
              aclBytesCountBefore + (2 * sizeOfPacketSent) + 4);
        }
      } else {
        EXPECT_EVENTUALLY_EQ(aclPktCountBefore, aclPktCountAfter);
        if (isSupportedOnAllAsics(HwAsic::Feature::ACL_BYTE_COUNTER)) {
          EXPECT_EVENTUALLY_EQ(aclBytesCountBefore, aclBytesCountAfter);
        }
      }
    });
  }

  void addAclAndStat(cfg::SwitchConfig* config, AclType aclType) const {
    auto aclName = getAclName(aclType);
    auto counterName = getCounterName(aclType);
    auto acl = utility::addAcl(config, aclName, aclActionType_);
    switch (aclType) {
      case AclType::TCP_TTLD:
      case AclType::UDP_TTLD:
        acl->srcIp() = "2620:0:1cfe:face:b00c::/64";
        acl->proto() = aclType == AclType::UDP_TTLD ? 17 : 6;
        acl->ipType() = cfg::IpType::IP6;
        acl->ttl() = cfg::Ttl();
        *acl->ttl()->value() = 128;
        *acl->ttl()->mask() = 128;
        if (isSupportedOnAllAsics(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
          acl->etherType() = cfg::EtherType::IPv6;
        }
        break;
      case AclType::SRC_PORT:
      case AclType::SRC_PORT_DENY:
        acl->srcPort() = helper_->ecmpPortDescriptorAt(0).phyPortID();
        break;
      case AclType::L4_DST_PORT:
        acl->srcPort() = helper_->ecmpPortDescriptorAt(0).phyPortID();
        acl->l4DstPort() = kL4DstPort2();
        break;
      case AclType::UDF_OPCODE_ACK:
        acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
        acl->roceOpcode() = utility::kUdfRoceOpcodeAck;
        break;
      case AclType::UDF_OPCODE_WRITE_IMMEDIATE:
        acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
        acl->roceOpcode() = utility::kUdfRoceOpcodeWriteImmediate;
        break;
      case AclType::BTH_OPCODE:
        acl->etherType() = cfg::EtherType::IPv6;
        acl->roceOpcode() = utility::kUdfRoceOpcodeAck;
        break;
      case AclType::FLOWLET:
      case AclType::UDF_FLOWLET:
        break;
    }
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::addAclStat(config, aclName, counterName, setCounterTypes);
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

// Verify that traffic arrive on a front panel port increments ACL counter.
TEST_F(AgentAclCounterTest, VerifyCounterBumpOnTtlHitFrontPanel) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

TEST_F(AgentAclCounterTest, VerifyCounterBumpOnSportHitFrontPanel) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::SRC_PORT});
}
TEST_F(AgentAclCounterTest, VerifyCounterBumpOnL4DstportHitFrontPanel) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::L4_DST_PORT});
}
TEST_F(AgentAclCounterTest, VerifyCounterBumpOnSportHitFrontPanelWithDrop) {
  this->aclActionType_ = cfg::AclActionType::DENY;
  this->counterBumpOnHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::SRC_PORT});
}
// Verify that traffic originating on the CPU increments ACL counter.
TEST_F(AgentAclCounterTest, VerifyCounterBumpOnTtlHitCpu) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */,
      false /* cpu port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

TEST_F(AgentAclCounterTest, VerifyCounterBumpOnSportHitCpu) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */, false /* cpu port */, {AclType::SRC_PORT});
}

// Verify that traffic arrive on a front panel port increments ACL counter.
TEST_F(AgentAclCounterTest, VerifyCounterNoTtlHitNoBumpFrontPanel) {
  this->counterBumpOnHitHelper(
      false /* no hit, no bump */,
      true /* front panel port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

// Verify that traffic originating on the CPU increments ACL counter.
TEST_F(AgentAclCounterTest, VerifyCounterNoHitNoBumpCpu) {
  this->counterBumpOnHitHelper(
      false /* no hit, no bump */,
      false /* cpu port */,
      {AclType::TCP_TTLD, AclType::UDP_TTLD});
}

TEST_F(AgentAclCounterTest, VerifyAclPrioritySportHitFrontPanel) {
  this->aclPriorityTestHelper();
}

TEST_F(AgentAclCounterTest, VerifyAclPriorityL4DstportHitFrontPanel) {
  this->aclPriorityTestHelper2();
}

/*
 * UDF Acls are not supported on SAI and multi ACL. So we only test with
 * multi acl disabled for now.
 */
class AgentUdfAclCounterTest : public AgentAclCounterTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    cfg.udfConfig() = utility::addUdfAclConfig();
    return cfg;
  }
};

TEST_F(AgentUdfAclCounterTest, VerifyUdf) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_OPCODE_ACK, AclType::UDF_OPCODE_WRITE_IMMEDIATE});
}

TEST_F(AgentUdfAclCounterTest, VerifyUdfWithOtherAcls) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_OPCODE_ACK,
       AclType::UDF_OPCODE_WRITE_IMMEDIATE,
       AclType::SRC_PORT});
}

TEST_F(AgentUdfAclCounterTest, VerifyAddRemoveUdfAcls) {
  auto setup = [this]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper_->resolveNextHops(in, 2);
    });
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, kEcmpWidth);
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    XLOG(DBG2) << "verify roce ack packets are matched";
    verifyAclType(true, true, AclType::UDF_OPCODE_ACK);
    XLOG(DBG2) << "verify roce write immediate packets are not matched";
    verifyAclType(false, true, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    applyNewConfig(newCfg);
    XLOG(DBG2) << "verify roce ack packets are not matched";
    verifyAclType(false, true, AclType::UDF_OPCODE_ACK);
    XLOG(DBG2) << "verify roce write immediate packets are matched";
    verifyAclType(true, true, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    newCfg = initialConfig(*getAgentEnsemble());
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    applyNewConfig(newCfg);
  };

  auto setupPostWarmboot = [this]() {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    applyNewConfig(newCfg);
  };

  auto verifyPostWarmboot = [this]() {
    XLOG(DBG2) << "verify roce ack packets are matched";
    verifyAclType(true, true, AclType::UDF_OPCODE_ACK);
    XLOG(DBG2) << "verify roce write immediate packets are matched";
    verifyAclType(true, true, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    applyNewConfig(newCfg);
    XLOG(DBG2) << "verify roce ack packets are matched";
    verifyAclType(true, true, AclType::UDF_OPCODE_ACK);
    XLOG(DBG2) << "verify roce write immediate packets are not matched";
    verifyAclType(false, true, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

// Add UDF for hash config after warmboot
TEST_F(AgentUdfAclCounterTest, VerifyUdfPlusUdfHash) {
  auto setup = [this]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper_->resolveNextHops(in, 2);
    });
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, kEcmpWidth);
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    verifyAclType(true, true, AclType::UDF_OPCODE_ACK);
    verifyAclType(true, true, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
  };

  auto setupPostWarmboot = [this]() {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    newCfg.udfConfig() = utility::addUdfHashAclConfig();
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verify);
}

// Remove UDF for hash config after warmboot
TEST_F(AgentUdfAclCounterTest, VerifyUdfMinusUdfHash) {
  auto setup = [this]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper_->resolveNextHops(in, 2);
    });
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, kEcmpWidth);
    auto newCfg{initialConfig(*getAgentEnsemble())};
    newCfg.udfConfig() = utility::addUdfHashAclConfig();
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    verifyAclType(true, true, AclType::UDF_OPCODE_ACK);
    verifyAclType(true, true, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
  };

  auto setupPostWarmboot = [this]() {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_ACK);
    addAclAndStat(&newCfg, AclType::UDF_OPCODE_WRITE_IMMEDIATE);
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verify);
}

class AgentBthOpcodeAclCounterTest : public AgentAclCounterTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::BTH_OPCODE_ACL,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }
};

TEST_F(
    AgentBthOpcodeAclCounterTest,
    VerifyCounterBumpOnBthOpcodeHitFrontPanel) {
  this->counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::BTH_OPCODE});
}

/*
 * Flowlet Acls are not supported on SAI and multi ACL. So we only test with
 * multi acl disabled for now.
 */
class AgentFlowletAclCounterTest : public AgentAclCounterTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    cfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved);
    utility::addFlowletConfigs(cfg, ensemble.masterLogicalPortIds());
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
  }
};

TEST_F(AgentFlowletAclCounterTest, VerifyFlowlet) {
  counterBumpOnFlowletAclHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::FLOWLET});
}

TEST_F(AgentFlowletAclCounterTest, VerifyUdfFlowlet) {
  counterBumpOnFlowletAclHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET});
}

TEST_F(AgentFlowletAclCounterTest, VerifyFlowletNegative) {
  this->roceReservedByte_ = 0x0;
  counterBumpOnFlowletAclHitHelper(
      false /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET});
}

TEST_F(AgentFlowletAclCounterTest, VerifyFlowletWithOtherAcls) {
  counterBumpOnFlowletAclHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::FLOWLET, AclType::SRC_PORT});
}

TEST_F(AgentFlowletAclCounterTest, VerifyUdfFlowletWithOtherAcls) {
  counterBumpOnFlowletAclHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET, AclType::SRC_PORT});
}

TEST_F(AgentFlowletAclCounterTest, VerifyFlowletWithUdf) {
  counterBumpOnFlowletAclHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::FLOWLET,
       AclType::UDF_OPCODE_ACK,
       AclType::UDF_OPCODE_WRITE_IMMEDIATE});
}

// Verifying the FLOWLET Acl always hit ahead of UDF Acl
// when FLOWLET Acl present before UDF Acl
TEST_F(AgentFlowletAclCounterTest, VerifyUdfFlowletWithUdf) {
  counterBumpOnFlowletAclHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET,
       AclType::UDF_OPCODE_ACK,
       AclType::UDF_OPCODE_WRITE_IMMEDIATE});
}
} // namespace facebook::fboss
