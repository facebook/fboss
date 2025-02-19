// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

using namespace facebook::fb303;

namespace facebook::fboss {

class AgentVoqSwitchWithMultipleDsfNodesTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    return cfg;
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const {
    return utility::addRemoteIntfNodeCfg(curDsfNodes, 1);
  }
  SwitchID getRemoteVoqSwitchId() const {
    auto dsfNodes = getSw()->getConfig().dsfNodes();
    // We added remote switch Id at the end
    auto [switchId, remoteNode] = *dsfNodes->rbegin();
    CHECK(*remoteNode.type() == cfg::DsfNodeType::INTERFACE_NODE);
    return SwitchID(switchId);
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
    WITH_RETRIES({
      sendPkts();
      voqDiscardBytes =
          getLatestSysPortStats(sysPortId).get_queueOutDiscardBytes_().at(
              utility::getDefaultQueue());
      XLOG(INFO) << " VOQ discard bytes: " << voqDiscardBytes;
      EXPECT_EVENTUALLY_GT(voqDiscardBytes, 0);
    });
    WITH_RETRIES({
      if (utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
              ->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
        auto switchIndices = getSw()->getSwitchInfoTable().getSwitchIndices();
        int totalVoqResourceExhaustionDrops = 0;
        for (const auto& switchIndex : switchIndices) {
          auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
          const auto& voqExhaustionDrop =
              switchStats.switchDropStats()->voqResourceExhaustionDrops();
          CHECK(voqExhaustionDrop.has_value());
          XLOG(INFO) << " Voq resource exhaustion drops for switchIndex "
                     << switchIndex << " : " << *voqExhaustionDrop;
          totalVoqResourceExhaustionDrops += *voqExhaustionDrop;
        }
        EXPECT_EVENTUALLY_GT(totalVoqResourceExhaustionDrops, 0);
      }
    });
    checkStatsStabilize(10);
  }
};

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, twoDsfNodes) {
  verifyAcrossWarmBoots([] {}, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, remoteSystemPort) {
  auto setup = [this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    auto getStats = [] {
      return std::make_tuple(
          fbData->getCounter(kSystemPortsFree), fbData->getCounter(kVoqsFree));
    };
    auto [beforeSysPortsFree, beforeVoqsFree] = getStats();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          SystemPortID(401),
          static_cast<SwitchID>(numCores));
    });
    WITH_RETRIES({
      auto [afterSysPortsFree, afterVoqsFree] = getStats();
      XLOG(INFO) << " Before sysPortsFree: " << beforeSysPortsFree
                 << " voqsFree: " << beforeVoqsFree
                 << " after sysPortsFree: " << afterSysPortsFree
                 << " voqsFree: " << afterVoqsFree;
      EXPECT_EVENTUALLY_EQ(beforeSysPortsFree - 1, afterSysPortsFree);
      EXPECT_EVENTUALLY_EQ(
          isDualStage3Q2QMode() ? beforeVoqsFree - 3 : beforeVoqsFree - 8,
          afterVoqsFree);
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, remoteRouterInterface) {
  auto setup = [this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    auto constexpr remotePortId = 401;
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          SystemPortID(remotePortId),
          static_cast<SwitchID>(numCores));
    });

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
          InterfaceID(remotePortId),
          // TODO - following assumes we haven't
          // already used up the subnets below for
          // local interfaces. In that sense it
          // has a implicit coupling with how ConfigFactory
          // generates subnets for local interfaces
          {
              {folly::IPAddress("100::1"), 64},
              {folly::IPAddress("100.0.0.1"), 24},
          });
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, addRemoveRemoteNeighbor) {
  auto setup = [this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(
              numCores * getAgentEnsemble()->getNumL3Asics()));
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
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
          });
    });
    folly::IPAddressV6 kNeighborIp("100::2");
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          true,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });

    // Remove neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          false,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, voqDelete) {
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  folly::IPAddressV6 kNeighborIp("100::2");
  auto setup = [=, this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(
              numCores * getAgentEnsemble()->getNumL3Asics()));
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
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
          });
    });
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          true,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });
  };
  auto verify = [=, this]() {
    auto getVoQDeletedPkts = [=, this]() {
      if (!isSupportedOnAllAsics(HwAsic::Feature::VOQ_DELETE_COUNTER)) {
        return 0L;
      }
      return getLatestSysPortStats(kRemoteSysPortId)
          .get_queueCreditWatchdogDeletedPackets_()
          .at(utility::getDefaultQueue());
    };

    auto voqDeletedPktsBefore = getVoQDeletedPkts();
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    for (auto i = 0; i < 100; ++i) {
      // Send pkts via front panel
      sendPacket(kNeighborIp, frontPanelPort, std::vector<uint8_t>(1024, 0xff));
    }
    WITH_RETRIES({
      auto voqDeletedPktsAfter = getVoQDeletedPkts();
      XLOG(INFO) << "Voq deleted pkts, before: " << voqDeletedPktsBefore
                 << " after: " << voqDeletedPktsAfter;
      EXPECT_EVENTUALLY_EQ(voqDeletedPktsBefore + 100, voqDeletedPktsAfter);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, stressAddRemoveObjects) {
  auto setup = [=, this]() {
    // Disable credit watchdog
    utility::enableCreditWatchdog(getAgentEnsemble(), false);
  };
  auto verify = [this]() {
    auto numIterations = 500;
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    folly::IPAddressV6 kNeighborIp("100::2");
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    const InterfaceID kIntfId(remotePortId);
    PortDescriptor kRemotePort(kRemoteSysPortId);
    auto addObjects = [&]() {
      // add local neighbor
      addRemoveNeighbor(kPort, true /* add neighbor*/);
      // Remote objs
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return utility::addRemoteSysPort(
            in,
            scopeResolver(),
            kRemoteSysPortId,
            static_cast<SwitchID>(
                numCores * getAgentEnsemble()->getNumL3Asics()));
      });
      const InterfaceID kIntfId(remotePortId);
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return utility::addRemoteInterface(
            in,
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
            });
      });
      PortDescriptor kPort(kRemoteSysPortId);
      // Add neighbor
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return utility::addRemoveRemoteNeighbor(
            in,
            scopeResolver(),
            kNeighborIp,
            kIntfId,
            kPort,
            true,
            utility::getDummyEncapIndex(getAgentEnsemble()));
      });
    };
    auto removeObjects = [&]() {
      addRemoveNeighbor(kPort, false /* remove neighbor*/);
      // Remove neighbor
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return utility::addRemoveRemoteNeighbor(
            in,
            scopeResolver(),
            kNeighborIp,
            kIntfId,
            kPort,
            false,
            utility::getDummyEncapIndex(getAgentEnsemble()));
      });
      // Remove rif
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return utility::removeRemoteInterface(in, kIntfId);
      });
      // Remove sys port
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return utility::removeRemoteSysPort(in, kRemoteSysPortId);
      });
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

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, voqTailDropCounter) {
  folly::IPAddressV6 kNeighborIp("100::2");
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  auto setup = [=, this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    // Disable credit watchdog
    utility::enableCreditWatchdog(getAgentEnsemble(), false);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(
              numCores * getAgentEnsemble()->getNumL3Asics()));
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
          kIntfId,
          {
              {folly::IPAddress("100::1"), 64},
              {folly::IPAddress("100.0.0.1"), 24},
          });
    });
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          true,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });
  };

  auto verify = [=, this]() {
    assertVoqTailDrops(kNeighborIp, kRemoteSysPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(
    AgentVoqSwitchWithMultipleDsfNodesTest,
    sendPktsToRemoteUnresolvedNeighbor) {
  auto constexpr kRemotePortId = 401;
  const SystemPortID kRemoteSysPortId(kRemotePortId);
  auto setup = [=, this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(
              numCores * getAgentEnsemble()->getNumL3Asics()));
    });
    const InterfaceID kIntfId(kRemotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
          kIntfId,
          {
              {folly::IPAddress("100::1"), 64},
              {folly::IPAddress("100.0.0.1"), 24},
          });
    });
  };

  auto verify = [=, this]() {
    PortID portId = masterLogicalInterfacePortIds()[0];
    folly::IPAddressV6 kNeighbor6Ip("100::2");
    folly::IPAddressV4 kNeighbor4Ip("100.0.0.2");
    auto portStatsBefore = getLatestPortStats(portId);
    auto switchDropStatsBefore = getAggregatedSwitchDropStats();
    sendPacket(kNeighbor6Ip, portId);
    sendPacket(kNeighbor4Ip, portId);
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(portId);
      auto switchDropStatsAfter = getAggregatedSwitchDropStats();
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDstNullDiscards_() -
              *portStatsBefore.inDstNullDiscards_());
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.inDiscardsRaw_(),
          *portStatsAfter.inDstNullDiscards_());
      EXPECT_EVENTUALLY_EQ(
          *switchDropStatsAfter.ingressPacketPipelineRejectDrops() -
              *switchDropStatsBefore.ingressPacketPipelineRejectDrops(),
          2);
      // Pipeline reject drop, not a queue resolution drop,
      // which happens say when a pkt comes in with a non router
      // MAC
      EXPECT_EQ(
          *switchDropStatsAfter.queueResolutionDrops() -
              *switchDropStatsBefore.queueResolutionDrops(),
          0);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, verifyDscpToVoqMapping) {
  folly::IPAddressV6 kNeighborIp("100::2");
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  auto setup = [=, this]() {
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(
              numCores * getAgentEnsemble()->getNumL3Asics()));
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
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
          });
    });
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          true,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });
  };

  auto verify = [=, this]() {
    for (const auto& q2dscps : utility::kNetworkAIQueueToDscp()) {
      auto queueId = q2dscps.first;
      for (auto dscp : q2dscps.second) {
        XLOG(DBG2) << "verify packet with dscp " << static_cast<int>(dscp)
                   << " goes to queue " << queueId;
        auto statsBefore = getLatestSysPortStats(kRemoteSysPortId);
        auto queueBytesBefore = statsBefore.queueOutBytes_()->at(queueId) +
            statsBefore.queueOutDiscardBytes_()->at(queueId);
        sendPacket(
            kNeighborIp,
            std::nullopt,
            std::optional<std::vector<uint8_t>>(),
            dscp);
        WITH_RETRIES({
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

class AgentVoqShelSwitchTest : public AgentVoqSwitchWithMultipleDsfNodesTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config =
        AgentVoqSwitchWithMultipleDsfNodesTest::initialConfig(ensemble);
    // Set SHEL configuration
    cfg::SelfHealingEcmpLagConfig shelConfig;
    shelConfig.shelSrcIp() = "2222::1";
    shelConfig.shelDstIp() = "2222::2";
    shelConfig.shelPeriodicIntervalMS() = 5000;
    config.switchSettings()->selfHealingEcmpLagConfig() = shelConfig;
    // Enable selfHealingEcmpLag on Interface Ports
    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT) {
        port.selfHealingECMPLagEnable() = true;
      }
    }
    return config;
  }
};

TEST_F(AgentVoqShelSwitchTest, init) {
  auto setup = []() {};
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if (port.second->getPortType() == cfg::PortType::INTERFACE_PORT) {
          EXPECT_TRUE(port.second->getSelfHealingECMPLagEnable().has_value());
          EXPECT_TRUE(port.second->getSelfHealingECMPLagEnable().value());
        }
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
