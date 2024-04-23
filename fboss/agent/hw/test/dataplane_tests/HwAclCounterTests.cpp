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
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
enum AclType {
  TCP_TTLD,
  UDP_TTLD,
  SRC_PORT,
  SRC_PORT_DENY,
  L4_DST_PORT,
  L4_DST_PORT_VLAN,
  UDF,
  FLOWLET,
  UDF_FLOWLET,
  BTH_OPCODE,
  VLAN,
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

 public:
  cfg::AclActionType aclActionType_ = cfg::AclActionType::DENY;
  uint8_t roceReservedByte_ = utility::kRoceReserved;

 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = isMultiAclEnabled;
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), getPlatform()->getLocalMac(), RouterID(0));
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
      case AclType::SRC_PORT_DENY:
        aclName = "test-deny-acl";
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
      case AclType::UDF_FLOWLET:
        aclName = utility::kFlowletAclName;
        break;
      case AclType::L4_DST_PORT:
        aclName = "test-l4-port-acl";
        break;
      case AclType::BTH_OPCODE:
        aclName = "test-bth-opcode-acl";
        break;
      case AclType::L4_DST_PORT_VLAN:
        aclName = "test-l4-port-vlan-acl";
        break;
      case AclType::VLAN:
        aclName = "test-vlan-acl";
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
      case AclType::VLAN:
        counterName = "test-vlan-acl-stats";
        break;
      case AclType::UDF:
        counterName = "test-udf-acl-stats";
        break;
      case AclType::FLOWLET:
        counterName = "test-flowlet-acl-stats";
        break;
      case AclType::UDF_FLOWLET:
        counterName = utility::kFlowletAclCounterName;
        break;
      case AclType::L4_DST_PORT:
        counterName = "test-l4-port-acl-stats";
        break;
      case AclType::L4_DST_PORT_VLAN:
        counterName = "test-l4-port-vlan-acl-stats";
        break;
      case AclType::BTH_OPCODE:
        counterName = "test-bth-opcode-acl-stats";
        break;
    }
    return counterName;
  }

  void counterBumpOnHitHelper(
      bool bumpOnHit,
      bool frontPanel,
      std::vector<AclType> aclTypes) {
    auto setup = [this, aclTypes]() {
      applyNewState(helper_->resolveNextHops(getProgrammedState(), kEcmpWidth));
      helper_->programRoutes(getRouteUpdater(), kEcmpWidth);
      auto newCfg{initialConfig()};
      for (auto aclType : aclTypes) {
        // existing flowlet will not have UDF configuration
        switch (aclType) {
          case AclType::UDF_FLOWLET:
          case AclType::FLOWLET:
            newCfg.udfConfig() = utility::addUdfAckAndFlowletAclConfig();
            utility::addFlowletConfigs(
                newCfg,
                masterLogicalPortIds(),
                cfg::SwitchingMode::FLOWLET_QUALITY,
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
    };

    auto verify = [this, bumpOnHit, frontPanel, aclTypes]() {
      for (auto aclType : aclTypes) {
        // since FLOWLET Acl presents ahead of UDF Acl in TCAM
        // the packet always hit the FLOWLET Acl. Hence verify the FLOWLET Acl
        if (aclTypes[0] == AclType::FLOWLET) {
          aclType = AclType::FLOWLET;
        }
        if (aclTypes[0] == AclType::UDF_FLOWLET) {
          aclType = AclType::UDF_FLOWLET;
        }
        verifyAclType(bumpOnHit, frontPanel, aclType);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  size_t sendRoceTraffic(const PortID frontPanelEgrPort) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    return utility::pumpRoCETraffic(
        true,
        utility::getAllocatePktFn(getHwSwitchEnsemble()),
        utility::getSendPktFunc(getHwSwitchEnsemble()),
        intfMac,
        vlanId,
        frontPanelEgrPort,
        utility::kUdfL4DstPort,
        255,
        std::nullopt,
        1 /* one packet */,
        this->roceReservedByte_);
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
    int l4DstPort = kL4DstPort();
    if (aclType == AclType::L4_DST_PORT ||
        aclType == AclType::L4_DST_PORT_VLAN) {
      l4DstPort = kL4DstPort2();
    }

    auto txPacket = aclType == AclType::UDP_TTLD ? utility::makeUDPTxPacket(
                                                       getHwSwitch(),
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
                                                       getHwSwitch(),
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
      auto outPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
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

  int kL4SrcPort() const {
    return 8000;
  }

  int kL4DstPort() const {
    return 8001;
  }

  int kL4DstPort2() const {
    return 8002;
  }

  auto verifyAclType(bool bumpOnHit, bool frontPanel, AclType aclType) {
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
    if ((aclType == AclType::FLOWLET || aclType == AclType::UDF_FLOWLET) &&
        FLAGS_flowletSwitchingEnable) {
      XLOG(DBG3) << "setting ECMP Member Status: ";
      utility::setEcmpMemberStatus(getHwSwitchEnsemble());
      sendRoce = true;
    }
    if (aclType == AclType::UDF || aclType == AclType::BTH_OPCODE) {
      sendRoce = true;
    }
    // for udf or flowlet testing, send roce packets
    if (sendRoce) {
      auto outPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
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
        if (frontPanel) {
          // if sent from cpu port, packet might be dropped by ACL before
          // sending out of front panel port
          EXPECT_EVENTUALLY_GT(pktsAfter, pktsBefore);
        }
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
  }

  void addAclAndStat(cfg::SwitchConfig* config, AclType aclType) const {
    auto aclName = getAclName(aclType);
    auto counterName = getCounterName(aclType);
    auto acl = utility::addAcl(config, aclName, aclActionType_);
    auto vlanId = utility::firstVlanID(initialConfig());
    switch (aclType) {
      case AclType::TCP_TTLD:
      case AclType::UDP_TTLD:
        acl->srcIp() = "2620:0:1cfe:face:b00c::/64";
        acl->proto() = aclType == AclType::UDP_TTLD ? 17 : 6;
        if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_JERICHO3) {
          // TODO(daiweix): remove after J3 ACL supports IP_TYPE
          acl->ipType() = cfg::IpType::IP6;
        }
        acl->ttl() = cfg::Ttl();
        *acl->ttl()->value() = 128;
        *acl->ttl()->mask() = 128;
        break;
      case AclType::SRC_PORT:
      case AclType::SRC_PORT_DENY:
        acl->srcPort() = helper_->ecmpPortDescriptorAt(0).phyPortID();
        break;
      case AclType::UDF:
        acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
        acl->roceOpcode() = utility::kUdfRoceOpcode;
        break;
      case AclType::FLOWLET:
      case AclType::UDF_FLOWLET:
        break;
      case AclType::VLAN:
        acl->vlanID() = vlanId.value();
        break;
      case AclType::L4_DST_PORT:
        acl->srcPort() = helper_->ecmpPortDescriptorAt(0).phyPortID();
        acl->l4DstPort() = kL4DstPort2();
        break;
      case AclType::BTH_OPCODE:
        acl->roceOpcode() = utility::kUdfRoceOpcode;
        break;
      case AclType::L4_DST_PORT_VLAN:
        acl->vlanID() = vlanId.value();
        acl->l4DstPort() = kL4DstPort2();
        break;
    }
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::addAclStat(config, aclName, counterName, setCounterTypes);
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

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
    return cfg;
  }

  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_enable_acl_table_group = false;
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
};

TEST_F(HwFlowletAclCounterTest, VerifyFlowlet) {
  counterBumpOnHitHelper(
      true /* bump on hit */, true /* front panel port */, {AclType::FLOWLET});
}

TEST_F(HwFlowletAclCounterTest, VerifyUdfFlowlet) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET});
}

TEST_F(HwFlowletAclCounterTest, VerifyFlowletNegative) {
  this->roceReservedByte_ = 0x0;
  counterBumpOnHitHelper(
      false /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET});
}

TEST_F(HwFlowletAclCounterTest, VerifyFlowletWithOtherAcls) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::FLOWLET, AclType::SRC_PORT});
}

TEST_F(HwFlowletAclCounterTest, VerifyUdfFlowletWithOtherAcls) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET, AclType::SRC_PORT});
}

// Verifying the FLOWLET Acl always hit ahead of UDF Acl
// when FLOWLET Acl present before UDF Acl
TEST_F(HwFlowletAclCounterTest, VerifyFlowletWithUdfAck) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::FLOWLET, AclType::UDF});
}

// Verifying the UDF_FLOWLET Acl always hit ahead of UDF Acl
// when UDF_FLOWLET Acl present before UDF Acl
TEST_F(HwFlowletAclCounterTest, VerifyUdfFlowletWithUdfAck) {
  counterBumpOnHitHelper(
      true /* bump on hit */,
      true /* front panel port */,
      {AclType::UDF_FLOWLET, AclType::UDF});
}

} // namespace facebook::fboss
