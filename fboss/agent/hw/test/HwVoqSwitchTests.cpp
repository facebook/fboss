// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestFabricUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace {
constexpr uint8_t kDefaultQueue = 0;
} // namespace

using namespace facebook::fb303;
namespace facebook::fboss {
class HwVoqSwitchTest : public HwLinkStateDependentTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);
    // Add ACL Table group before adding any ACLs
    utility::addAclTableGroup(
        &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
    utility::addDefaultAclTable(cfg);
    const auto& cpuStreamTypes =
        getAsic()->getQueueStreamTypes(cfg::PortType::CPU_PORT);
    for (const auto& cpuStreamType : cpuStreamTypes) {
      if (getAsic()->getDefaultNumPortQueues(
              cpuStreamType, cfg::PortType::CPU_PORT)) {
        // cpu queues supported
        utility::setDefaultCpuTrafficPolicyConfig(
            cfg,
            getHwSwitchEnsemble()->getL3Asics(),
            getHwSwitchEnsemble()->isSai());
        utility::addCpuQueueConfig(
            cfg,
            getHwSwitchEnsemble()->getL3Asics(),
            getHwSwitchEnsemble()->isSai());
        break;
      }
    }
    return cfg;
  }
  void SetUp() override {
    // VOQ switches will run SAI from day 1. so enable Multi acl for VOQ tests
    FLAGS_enable_acl_table_group = true;
    HwLinkStateDependentTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::VOQ);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }

 protected:
  void addRemoveNeighbor(
      PortDescriptor port,
      bool add,
      std::optional<int64_t> encapIdx = std::nullopt) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    if (add) {
      applyNewState(ecmpHelper.resolveNextHops(
          getProgrammedState(), {port}, false /*use link local*/, encapIdx));
    } else {
      applyNewState(ecmpHelper.unresolveNextHops(getProgrammedState(), {port}));
    }
  }

  int sendPacket(
      const folly::IPAddressV6& dstIp,
      std::optional<PortID> frontPanelPort,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>(),
      int dscp = 0x24) {
    folly::IPAddressV6 kSrcIp("1::1");
    const auto srcMac = utility::kLocalCpuMac();
    const auto dstMac = utility::kLocalCpuMac();

    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        std::nullopt, // vlanID
        srcMac,
        dstMac,
        kSrcIp,
        dstIp,
        8000, // l4 src port
        8001, // l4 dst port
        dscp << 2, // dscp
        255, // hopLimit
        payload);
    size_t txPacketSize = txPacket->buf()->length();

    XLOG(DBG5) << "\n"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());

    if (frontPanelPort.has_value()) {
      getHwSwitch()->sendPacketOutOfPortAsync(
          std::move(txPacket), *frontPanelPort);
    } else {
      getHwSwitch()->sendPacketSwitchedAsync(std::move(txPacket));
    }
    return txPacketSize;
  }

 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {
        HwSwitchEnsemble::LINKSCAN,
        HwSwitchEnsemble::PACKET_RX,
        HwSwitchEnsemble::TAM_NOTIFY};
  }
};

class HwVoqSwitchWithMultipleDsfNodesTest : public HwVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = HwVoqSwitchTest::initialConfig();
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    return cfg;
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const override {
    return utility::addRemoteDsfNodeCfg(curDsfNodes, 1);
  }

 protected:
  void assertVoqTailDrops(
      const folly::IPAddressV6& nbrIp,
      const SystemPortID& sysPortId) {
    auto sendPkts = [=, this]() {
      for (auto i = 0; i < 1000; ++i) {
        sendPacket(nbrIp, std::nullopt);
      }
    };
    auto voqDiscardBytes = 0;
    WITH_RETRIES_N(100, {
      sendPkts();
      getHwSwitch()->updateStats();
      voqDiscardBytes =
          getLatestSysPortStats(sysPortId).get_queueOutDiscardBytes_().at(
              kDefaultQueue);
      XLOG(INFO) << " VOQ discard bytes: " << voqDiscardBytes;
      EXPECT_EVENTUALLY_GT(voqDiscardBytes, 0);
    });
    if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      auto switchDropStats = getHwSwitch()->getSwitchDropStats();
      CHECK(switchDropStats.voqResourceExhaustionDrops().has_value());
      XLOG(INFO) << " Voq resource exhaustion drops: "
                 << *switchDropStats.voqResourceExhaustionDrops();
      EXPECT_GT(*switchDropStats.voqResourceExhaustionDrops(), 0);
    }
    checkNoStatsChange();
  }
};

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, stressAddRemoveObjects) {
  auto setup = [=, this]() {
    // Disable credit watchdog
    utility::enableCreditWatchdog(getHwSwitch(), false);
  };
  auto verify = [this]() {
    auto numIterations = 500;
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    folly::IPAddressV6 kNeighborIp("100::2");
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores = getAsic()->getNumCores();
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    const InterfaceID kIntfId(remotePortId);
    PortDescriptor kRemotePort(kRemoteSysPortId);
    auto addObjects = [&]() {
      // add local neighbor
      addRemoveNeighbor(kPort, true /* add neighbor*/);
      // Remote objs
      applyNewState(utility::addRemoteSysPort(
          getProgrammedState(),
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(numCores)));
      applyNewState(utility::addRemoteInterface(
          getProgrammedState(),
          scopeResolver(),
          kIntfId,
          // TODO - following assumes we haven't
          // already used up the subnets below for
          // local interfaces. In that sense it
          // has a implicit coupling with how ConfigFactory
          // generates subnets for local interfaces
          {
              {folly::IPAddress("100::1"), 64},
              {folly::IPAddress("100.0.0.1"), 24},
          }));
      // Add neighbor
      applyNewState(utility::addRemoveRemoteNeighbor(
          getProgrammedState(),
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kRemotePort,
          true,
          utility::getDummyEncapIndex(getHwSwitchEnsemble())));
    };
    auto removeObjects = [&]() {
      addRemoveNeighbor(kPort, false /* remove neighbor*/);
      // Remove neighbor
      applyNewState(utility::addRemoveRemoteNeighbor(
          getProgrammedState(),
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kRemotePort,
          false,
          utility::getDummyEncapIndex(getHwSwitchEnsemble())));
      // Remove rif
      applyNewState(
          utility::removeRemoteInterface(getProgrammedState(), kIntfId));
      // Remove sys port
      applyNewState(
          utility::removeRemoteSysPort(getProgrammedState(), kRemoteSysPortId));
    };
    for (auto i = 0; i < numIterations; ++i) {
      addObjects();
      // Delete on all but the last iteration. In the last iteration
      // we will leave the entries intact and then forward pkts
      // to this VOQ
      if (i < numIterations - 1) {
        removeObjects();
      }
    }
    assertVoqTailDrops(kNeighborIp, kRemoteSysPortId);
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
    // CPU send
    sendPacket(ecmpHelper.ip(kPort), std::nullopt);
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    sendPacket(ecmpHelper.ip(kPort), frontPanelPort);
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      EXPECT_EVENTUALLY_EQ(afterPkts, beforePkts + 2);
    });
    // removeObjects before exiting for WB
    removeObjects();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, voqTailDropCounter) {
  folly::IPAddressV6 kNeighborIp("100::2");
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  auto setup = [=, this]() {
    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores = getAsic()->getNumCores();
    // Disable credit watchdog
    utility::enableCreditWatchdog(getHwSwitch(), false);
    applyNewState(utility::addRemoteSysPort(
        getProgrammedState(),
        scopeResolver(),
        kRemoteSysPortId,
        static_cast<SwitchID>(numCores)));
    const InterfaceID kIntfId(remotePortId);
    applyNewState(utility::addRemoteInterface(
        getProgrammedState(),
        scopeResolver(),
        kIntfId,
        {
            {folly::IPAddress("100::1"), 64},
            {folly::IPAddress("100.0.0.1"), 24},
        }));
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState(utility::addRemoveRemoteNeighbor(
        getProgrammedState(),
        scopeResolver(),
        kNeighborIp,
        kIntfId,
        kPort,
        true,
        utility::getDummyEncapIndex(getHwSwitchEnsemble())));
  };

  auto verify = [=, this]() {
    assertVoqTailDrops(kNeighborIp, kRemoteSysPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, verifyDscpToVoqMapping) {
  folly::IPAddressV6 kNeighborIp("100::2");
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  auto setup = [=, this]() {
    auto newCfg{initialConfig()};
    utility::addOlympicQosMaps(newCfg, getHwSwitchEnsemble()->getL3Asics());
    applyNewConfig(newCfg);

    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores = getAsic()->getNumCores();
    applyNewState(utility::addRemoteSysPort(
        getProgrammedState(),
        scopeResolver(),
        kRemoteSysPortId,
        static_cast<SwitchID>(numCores)));
    const InterfaceID kIntfId(remotePortId);
    applyNewState(utility::addRemoteInterface(
        getProgrammedState(),
        scopeResolver(),
        kIntfId,
        {
            {folly::IPAddress("100::1"), 64},
            {folly::IPAddress("100.0.0.1"), 24},
        }));
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState(utility::addRemoveRemoteNeighbor(
        getProgrammedState(),
        scopeResolver(),
        kNeighborIp,
        kIntfId,
        kPort,
        true,
        utility::getDummyEncapIndex(getHwSwitchEnsemble())));
  };

  auto verify = [=, this]() {
    for (const auto& q2dscps : utility::kOlympicQueueToDscp()) {
      auto queueId = q2dscps.first;
      for (auto dscp : q2dscps.second) {
        XLOG(DBG2) << "verify packet with dscp " << dscp << " goes to queue "
                   << queueId;
        auto statsBefore = getLatestSysPortStats(kRemoteSysPortId);
        auto queueBytesBefore = statsBefore.queueOutBytes_()->at(queueId) +
            statsBefore.queueOutDiscardBytes_()->at(queueId);
        sendPacket(
            kNeighborIp,
            std::nullopt,
            std::optional<std::vector<uint8_t>>(),
            dscp);
        WITH_RETRIES_N(10, {
          auto statsAfter = getLatestSysPortStats(kRemoteSysPortId);
          auto queueBytesAfter = statsAfter.queueOutBytes_()->at(queueId) +
              statsAfter.queueOutDiscardBytes_()->at(queueId);
          XLOG(DBG2) << "queue " << queueId
                     << " stats before: " << queueBytesBefore
                     << " stats after: " << queueBytesAfter;
          EXPECT_EVENTUALLY_GT(queueBytesAfter, queueBytesBefore);
        });
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
};

} // namespace facebook::fboss
