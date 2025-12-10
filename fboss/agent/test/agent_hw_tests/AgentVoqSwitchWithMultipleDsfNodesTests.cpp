// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/RouteTestUtils.h"
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
    const auto config = getSw()->getConfig();
    const auto dsfNodes = config.dsfNodes();
    // We added remote switch Id at the end
    const auto& [switchId, remoteNode] = *dsfNodes->crbegin();
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
      if (checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
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

  cfg::SwitchConfig getShelEnabledConfig(
      const cfg::SwitchConfig& inputConfig) const {
    auto config = inputConfig;
    // Set SHEL configuration
    cfg::SelfHealingEcmpLagConfig shelConfig;
    shelConfig.shelSrcIp() = "2222::1";
    shelConfig.shelDstIp() = "2222::2";
    shelConfig.shelPeriodicIntervalMS() = 5000;
    config.switchSettings()->selfHealingEcmpLagConfig() = shelConfig;
    // Enable selfHealingEcmpLag on Interface Ports
    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT ||
          port.portType() == cfg::PortType::HYPER_PORT) {
        port.selfHealingECMPLagEnable() = true;
      }
    }
    return config;
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
    auto getStats = [this] {
      auto switchID = *getSw()->getHwAsicTable()->getSwitchIDs().begin();
      return std::make_tuple(
          getAgentEnsemble()->getFb303Counter(
              std::string(kSystemPortsFree), switchID),
          getAgentEnsemble()->getFb303Counter(
              std::string(kVoqsFree), switchID));
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
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
      return folly::copy(getLatestSysPortStats(kRemoteSysPortId)
                             .queueCreditWatchdogDeletedPackets_()
                             .value())
          .at(utility::getDefaultQueue());
    };

    auto voqDeletedPktsBefore = getVoQDeletedPkts();
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
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
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    // in addRemoteIntfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    const InterfaceID kIntfId(remotePortId);
    PortDescriptor kRemotePort(kRemoteSysPortId);
    auto addObjects = [&]() {
      // add local neighbor
      addRemoveNeighbor(kPort, NeighborOp::ADD);
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
      addRemoveNeighbor(kPort, NeighborOp::DEL);
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
    auto beforePkts = folly::copy(
        getLatestPortStats(kPort.phyPortID()).outUnicastPkts_().value());
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
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
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
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
      auto tc = q2dscps.first;
      auto voqId = utility::getTrafficClassToVoqId(
          checkSameAndGetAsic(getAgentEnsemble()->getL3Asics()), tc);
      for (auto dscp : q2dscps.second) {
        XLOG(DBG2) << "verify packet with dscp " << static_cast<int>(dscp)
                   << " goes to voq " << voqId;
        auto statsBefore = getLatestSysPortStats(kRemoteSysPortId);
        auto queueBytesBefore = statsBefore.queueOutBytes_()->at(voqId) +
            statsBefore.queueOutDiscardBytes_()->at(voqId);
        sendPacket(
            kNeighborIp,
            std::nullopt,
            std::optional<std::vector<uint8_t>>(),
            dscp);
        WITH_RETRIES_N(10, {
          auto statsAfter = getLatestSysPortStats(kRemoteSysPortId);
          auto queueBytesAfter = statsAfter.queueOutBytes_()->at(voqId) +
              statsAfter.queueOutDiscardBytes_()->at(voqId);
          XLOG(DBG2) << "voq " << voqId << " stats before: " << queueBytesBefore
                     << " stats after: " << queueBytesAfter;
          EXPECT_EVENTUALLY_GT(queueBytesAfter, queueBytesBefore);
        });
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, resolveRouteToLoopbackIp) {
  const auto ipAddr = folly::IPAddressV6("42::42");
  const auto prefixLen = 128;
  // Only one remote system port should be created by the remote interface
  // node's Loopback.
  auto getRemoteLoopbackSysPort = [&]() {
    auto remoteSysPorts =
        getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
    CHECK_EQ(remoteSysPorts->size(), 1);
    return remoteSysPorts->cbegin()->first;
  };

  auto getLoopbackNeighborIp = [&]() {
    auto remoteIntf = getProgrammedState()->getRemoteInterfaces()->getNode(
        getRemoteLoopbackSysPort());
    auto nbrTable = remoteIntf->getNdpTable();
    CHECK_GE(nbrTable->size(), 1);
    return folly::IPAddress(nbrTable->cbegin()->first);
  };

  auto setup = [&, this]() {
    auto remoteIntf = getProgrammedState()->getRemoteInterfaces()->getNode(
        getRemoteLoopbackSysPort());
    auto neighborIp = getLoopbackNeighborIp();
    auto routeUpdater = getSw()->getRouteUpdater();
    RouteNextHopSet nextHopSet{
        ResolvedNextHop(neighborIp, remoteIntf->getID(), 1)};
    routeUpdater.addRoute(
        RouterID(0),
        ipAddr,
        prefixLen,
        ClientID::BGPD,
        RouteNextHopEntry(nextHopSet, AdminDistance::EBGP));
    routeUpdater.addRoute(
        RouterID(0),
        neighborIp,
        prefixLen,
        ClientID::BGPD,
        RouteNextHopEntry(nextHopSet, AdminDistance::EBGP));
    routeUpdater.program();
  };
  auto verify = [&]() {
    auto sendPacketAndVerifyFwding = [&]() {
      auto neighborIp = getLoopbackNeighborIp();
      auto sysPortID = SystemPortID(getRemoteLoopbackSysPort());
      auto beforeStats = getLatestSysPortStats(sysPortID);
      sendPacket(ipAddr, std::nullopt /* frontPanelPort */);
      sendPacket(neighborIp, std::nullopt /* frontPanelPort */);

      auto getWatchdogDeletePkts = [](const auto& stats) {
        return stats.queueCreditWatchdogDeletedPackets_()->at(
            utility::getGlobalRcyDefaultQueue());
      };
      WITH_RETRIES({
        auto afterStats = getLatestSysPortStats(sysPortID);
        XLOG(DBG2) << "Before: " << getWatchdogDeletePkts(beforeStats)
                   << " After: " << getWatchdogDeletePkts(afterStats);
        EXPECT_EVENTUALLY_EQ(
            getWatchdogDeletePkts(afterStats),
            getWatchdogDeletePkts(beforeStats) + 2);
      });
    };

    sendPacketAndVerifyFwding();

    // Remove route that has the same IP as neighbor
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.delRoute(
        RouterID(0), getLoopbackNeighborIp(), prefixLen, ClientID::BGPD);
    routeUpdater.program();

    sendPacketAndVerifyFwding();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, resolveRouteToRemoteNeighbor) {
  constexpr auto kRemotePortId = 401;
  const SystemPortID kRemoteSysPortId(kRemotePortId);
  const InterfaceID kRemoteIntfId(kRemotePortId);

  const auto intfAddrV6 = std::make_pair(folly::IPAddress("100::1"), 64);
  const auto intfAddrV4 = std::make_pair(folly::IPAddress("100.0.0.1"), 24);

  const auto ipAddr = folly::IPAddressV6("42::42");
  const auto prefixLen = 128;

  auto makeSwitchId2SystemPorts = [=](auto& remoteSwitchID) {
    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
    auto remoteSysPort =
        utility::makeRemoteSysPort(kRemoteSysPortId, remoteSwitchID);
    auto remoteSysPorts = std::make_shared<SystemPortMap>();
    remoteSysPorts->addNode(remoteSysPort);
    switchId2SystemPorts[remoteSwitchID] = std::move(remoteSysPorts);
    return switchId2SystemPorts;
  };

  auto makeSwitchId2Rifs = [=](auto& remoteSwitchID) {
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs;
    auto remoteIntf = utility::makeRemoteInterface(
        kRemoteIntfId,
        {
            intfAddrV6,
            intfAddrV4,
        });
    auto remoteIntfs = std::make_shared<InterfaceMap>();
    remoteIntfs->addNode(remoteIntf);
    switchId2Rifs[remoteSwitchID] = std::move(remoteIntfs);
    return switchId2Rifs;
  };

  auto addRemoteSysPortAndIntf = [&](auto swSwitch, auto& remoteSwitchID) {
    auto updateDsfStateFn = [=](const std::shared_ptr<SwitchState>& in) {
      auto switchId2SystemPorts = makeSwitchId2SystemPorts(remoteSwitchID);
      auto switchId2Rifs = makeSwitchId2Rifs(remoteSwitchID);

      return DsfStateUpdaterUtil::getUpdatedState(
          in,
          swSwitch->getScopeResolver(),
          swSwitch->getRib(),
          switchId2SystemPorts,
          switchId2Rifs);
    };
    swSwitch->getRib()->updateStateInRibThread([swSwitch, updateDsfStateFn]() {
      swSwitch->updateStateWithHwFailureProtection(
          folly::sformat("Update state for node: {}", 0), updateDsfStateFn);
    });
  };

  auto setup = [&, this]() {
    auto remoteSwitchID = getRemoteVoqSwitchId();
    addRemoteSysPortAndIntf(getSw(), remoteSwitchID);

    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto routeUpdater = getSw()->getRouteUpdater();
    auto sysPortDescs =
        utility::resolveRemoteNhops(getAgentEnsemble(), ecmpHelper);
    CHECK_EQ(sysPortDescs.size(), 1);
    ecmpHelper.programRoutes(
        &routeUpdater, sysPortDescs, {RoutePrefixV6{ipAddr, prefixLen}});
  };
  auto verify = [&, this]() {
    auto routeInfo =
        utility::getRouteInfo(ipAddr, prefixLen, *getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.exists() && !*routeInfo.isProgrammedToDrop());

    auto beforeStats = getLatestSysPortStats(kRemoteSysPortId);
    sendPacket(ipAddr, std::nullopt /* frontPanelPort */);
    auto getWatchdogDeletePkts = [](const auto& stats) {
      return stats.queueCreditWatchdogDeletedPackets_()->at(
          utility::getDefaultQueue());
    };
    WITH_RETRIES({
      auto afterStats = getLatestSysPortStats(kRemoteSysPortId);
      XLOG(DBG2) << "Before: " << getWatchdogDeletePkts(beforeStats)
                 << " After: " << getWatchdogDeletePkts(afterStats);
      EXPECT_EVENTUALLY_GT(
          getWatchdogDeletePkts(afterStats),
          getWatchdogDeletePkts(beforeStats));
    });

    // Unresolve neighbor
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    utility::unresolveRemoteNhops(getAgentEnsemble(), ecmpHelper);

    routeInfo = utility::getRouteInfo(ipAddr, prefixLen, *getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.exists() && *routeInfo.isProgrammedToDrop());

    // Resolve neighbor
    utility::resolveRemoteNhops(getAgentEnsemble(), ecmpHelper);
    routeInfo = utility::getRouteInfo(ipAddr, prefixLen, *getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.exists() && !*routeInfo.isProgrammedToDrop());
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentVoqShelSwitchTest : public AgentVoqSwitchWithMultipleDsfNodesTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config =
        AgentVoqSwitchWithMultipleDsfNodesTest::initialConfig(ensemble);
    return getShelEnabledConfig(config);
  }

 protected:
  void verifyShelEnabled(bool desiredShelEnable, bool shelEnabled) {
    auto state = getProgrammedState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if (port.second->getPortType() == cfg::PortType::INTERFACE_PORT ||
            port.second->getPortType() == cfg::PortType::HYPER_PORT) {
          if (desiredShelEnable) {
            EXPECT_TRUE(
                port.second->getDesiredSelfHealingECMPLagEnable().has_value());
            if (port.second->getDesiredSelfHealingECMPLagEnable().has_value()) {
              EXPECT_EQ(
                  port.second->getDesiredSelfHealingECMPLagEnable().value(),
                  desiredShelEnable);
            }
          }
          if (shelEnabled) {
            EXPECT_TRUE(port.second->getSelfHealingECMPLagEnable().has_value());
            if (port.second->getSelfHealingECMPLagEnable().has_value()) {
              EXPECT_EQ(
                  port.second->getSelfHealingECMPLagEnable().value(),
                  shelEnabled);
            }
          }
        }
      }
    }
  }

  void verifyShelPortState(bool enabled) {
    WITH_RETRIES({
      auto stats = getHwSwitchStats();
      auto state = getProgrammedState();
      for (const auto& portMap : std::as_const(*state->getPorts())) {
        for (const auto& port : std::as_const(*portMap.second)) {
          if (port.second->getPortType() == cfg::PortType::INTERFACE_PORT ||
              port.second->getPortType() == cfg::PortType::HYPER_PORT) {
            auto switchId = scopeResolver().scope(port.second).switchId();
            EXPECT_EVENTUALLY_TRUE(stats.contains(switchId));
            auto globalSystemPortOffset = *getSw()
                                               ->getSwitchInfoTable()
                                               .getSwitchInfo(switchId)
                                               .globalSystemPortOffset();
            if (stats.contains(switchId)) {
              auto sysPortShelState = stats.at(switchId).sysPortShelState();
              auto systemPortId = globalSystemPortOffset + port.first;
              EXPECT_EVENTUALLY_TRUE(sysPortShelState->contains(systemPortId));
              if (sysPortShelState->contains(systemPortId)) {
                EXPECT_EVENTUALLY_EQ(
                    sysPortShelState->at(systemPortId),
                    (enabled ? cfg::PortState::ENABLED
                             : cfg::PortState::DISABLED));
              }
            }
          }
        }
      }
    })
  }
};

TEST_F(AgentVoqShelSwitchTest, init) {
  auto setup = []() {};
  auto verify = [&, this]() {
    verifyShelEnabled(true /*desiredShelEnable*/, false /*shelEnabled*/);

    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto localSysPortDescs = resolveLocalNhops(ecmpHelper);
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        flat_set<PortDescriptor>(
            std::make_move_iterator(localSysPortDescs.begin()),
            std::make_move_iterator(localSysPortDescs.end())));

    verifyShelEnabled(true /*desiredShelEnable*/, true /*shelEnabled*/);

    // Toggle ports to trigger callback
    auto state = getProgrammedState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if ((port.second->getPortType() == cfg::PortType::INTERFACE_PORT ||
             port.second->getPortType() == cfg::PortType::HYPER_PORT) &&
            port.second->getSelfHealingECMPLagEnable()) {
          bringDownPort(port.second->getID());
          bringUpPort(port.second->getID());
        }
      }
    }

    verifyShelPortState(true /*enabled*/);
  };
  auto setupPostWarmboot = [&, this]() {
    // Verify SHEL port state is reconstructed after WB
    verifyShelPortState(true /*enabled*/);

    // Remove default route to disable shelState
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.unprogramRoutes(&routeUpdater);
    verifyShelEnabled(true /*desiredShelEnable*/, false /*shelEnabled*/);

    // Disable selfHealingEcmpLag on Interface Ports
    auto config = getSw()->getConfig();
    config.switchSettings()->selfHealingEcmpLagConfig().reset();

    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT ||
          port.portType() == cfg::PortType::HYPER_PORT) {
        port.selfHealingECMPLagEnable() = false;
      }
    }
    applyNewConfig(config);
  };
  auto verifyPostWarmboot = [&, this]() {
    verifyShelEnabled(false /*desiredShelEnable*/, false /*shelEnabled*/);
  };
  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(AgentVoqShelSwitchTest, routeChurn) {
  const auto numChurn = 100;
  auto setup = []() {};
  auto verify = [&, this]() {
    verifyShelEnabled(true /*desiredShelEnable*/, false /*shelEnabled*/);

    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto localSysPortDescs = resolveLocalNhops(ecmpHelper);
    auto portDescriptors = flat_set<PortDescriptor>(
        std::make_move_iterator(localSysPortDescs.begin()),
        std::make_move_iterator(localSysPortDescs.end()));
    auto routeUpdater = getSw()->getRouteUpdater();

    for (int i = 0; i < numChurn; i++) {
      ecmpHelper.programRoutes(&routeUpdater, portDescriptors);
      verifyShelEnabled(true /*desiredShelEnable*/, true /*shelEnabled*/);
      ecmpHelper.unprogramRoutes(&routeUpdater);
      verifyShelEnabled(true /*desiredShelEnable*/, false /*shelEnabled*/);
    }
    ecmpHelper.programRoutes(&routeUpdater, portDescriptors);

    // Toggle ports to trigger callback
    auto state = getProgrammedState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if ((port.second->getPortType() == cfg::PortType::INTERFACE_PORT ||
             port.second->getPortType() == cfg::PortType::HYPER_PORT) &&
            port.second->getSelfHealingECMPLagEnable()) {
          bringDownPort(port.second->getID());
          bringUpPort(port.second->getID());
        }
      }
    }

    verifyShelPortState(true /*enabled*/);
    ecmpHelper.unprogramRoutes(&routeUpdater);
  };

  verifyAcrossWarmBoots(setup, verify);
}

class AgentVoqShelWBEnableTest : public AgentVoqShelSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return AgentVoqSwitchWithMultipleDsfNodesTest::initialConfig(ensemble);
  }
};

TEST_F(AgentVoqShelWBEnableTest, wbEnable) {
  auto setup = []() {};
  auto verify = [&, this]() {
    verifyShelEnabled(false /*desiredShelEnable*/, false /*shelEnabled*/);
  };
  auto setupPostWarmboot = [&, this]() {
    applyNewConfig(getShelEnabledConfig(getSw()->getConfig()));
    verifyShelEnabled(true /*desiredShelEnable*/, false /*shelEnabled*/);

    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto localSysPortDescs = resolveLocalNhops(ecmpHelper);
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        flat_set<PortDescriptor>(
            std::make_move_iterator(localSysPortDescs.begin()),
            std::make_move_iterator(localSysPortDescs.end())));

    verifyShelEnabled(true /*desiredShelEnable*/, true /*shelEnabled*/);

    // Toggle ports to trigger callback
    auto state = getProgrammedState();
    for (const auto& portMap : std::as_const(*state->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if ((port.second->getPortType() == cfg::PortType::INTERFACE_PORT ||
             port.second->getPortType() == cfg::PortType::HYPER_PORT) &&
            port.second->getSelfHealingECMPLagEnable()) {
          bringDownPort(port.second->getID());
          bringUpPort(port.second->getID());
        }
      }
    }

    verifyShelPortState(true /*enabled*/);
  };
  auto verifyPostWarmboot = []() {};
  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
