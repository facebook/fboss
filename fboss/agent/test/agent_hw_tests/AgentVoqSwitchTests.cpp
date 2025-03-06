// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(enable_stats_update_thread);

namespace {
constexpr auto kDefaultEgressQueue = 0;
} // namespace

using namespace facebook::fb303;
namespace facebook::fboss {

cfg::SwitchConfig AgentVoqSwitchTest::initialConfig(
    const AgentEnsemble& ensemble) const {
  // Increase the query timeout to be 5sec
  FLAGS_hwswitch_query_timeout = 5000;
  auto config = utility::onePortPerInterfaceConfig(
      ensemble.getSw(),
      ensemble.masterLogicalPortIds(),
      true /*interfaceHasSubnet*/);
  utility::addNetworkAIQosMaps(config, ensemble.getL3Asics());
  utility::setDefaultCpuTrafficPolicyConfig(
      config, ensemble.getL3Asics(), ensemble.isSai());
  utility::addCpuQueueConfig(config, ensemble.getL3Asics(), ensemble.isSai());
  return config;
}

void AgentVoqSwitchTest::rxPacketToCpuHelper(
    uint16_t l4SrcPort,
    uint16_t l4DstPort,
    uint8_t queueId) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);

  auto verify = [this, ecmpHelper, kPortDesc, l4SrcPort, l4DstPort, queueId]() {
    // TODO(skhare)
    // Send to only one IPv6 address for ease of debugging.
    // Once SAI implementation bugs are fixed, send to ALL interface
    // addresses.
    auto ipAddrs =
        *(initialConfig(*getAgentEnsemble()).interfaces()[0].ipAddresses());
    auto ipv6Addr =
        std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
          auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
          return ip.isV6();
        });

    auto dstIp =
        folly::IPAddress::createNetwork(*ipv6Addr, -1, false).first.asV6();
    folly::IPAddressV6 kSrcIp("1::1");
    const auto srcMac = folly::MacAddress("00:00:01:02:03:04");
    const auto dstMac = utility::kLocalCpuMac();

    auto createTxPacket =
        [this, srcMac, dstMac, kSrcIp, dstIp, l4SrcPort, l4DstPort]() {
          return utility::makeTCPTxPacket(
              getSw(),
              std::nullopt, // vlanID
              srcMac,
              dstMac,
              kSrcIp,
              dstIp,
              l4SrcPort,
              l4DstPort);
        };

    const PortID port = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    auto switchId =
        scopeResolver().scope(getProgrammedState(), port).switchId();
    auto [beforeQueueOutPkts, beforeQueueOutBytes] =
        utility::getCpuQueueOutPacketsAndBytes(getSw(), queueId, switchId);

    auto txPacket = createTxPacket();
    size_t txPacketSize = txPacket->buf()->length();

    getSw()->sendPacketOutOfPortAsync(std::move(txPacket), port);
    utility::SwSwitchPacketSnooper snooper(getSw(), "snoop");
    txPacket = createTxPacket();
    std::unique_ptr<folly::IOBuf> rxBuf;
    WITH_RETRIES({
      auto frameRx = snooper.waitForPacket(1);
      ASSERT_EVENTUALLY_TRUE(frameRx.has_value());
      CHECK(frameRx.has_value());
      rxBuf = std::move(*frameRx);
    });
    WITH_RETRIES({
      XLOG(DBG3) << "TX Packet Dump::" << std::endl
                 << folly::hexDump(
                        txPacket->buf()->data(), txPacket->buf()->length());
      XLOG(DBG3) << "RX Packet Dump::" << std::endl
                 << folly::hexDump(rxBuf->data(), rxBuf->length());

      XLOG(DBG2) << "TX Packet Length: " << txPacket->buf()->length()
                 << " RX Packet Length: " << rxBuf->length();
      EXPECT_EVENTUALLY_EQ(txPacket->buf()->length(), rxBuf->length());
      EXPECT_EVENTUALLY_EQ(
          0, memcmp(txPacket->buf()->data(), rxBuf->data(), rxBuf->length()));

      auto [afterQueueOutPkts, afterQueueOutBytes] =
          utility::getCpuQueueOutPacketsAndBytes(getSw(), queueId, switchId);

      XLOG(DBG2) << "Stats:: queueId: " << static_cast<int>(queueId)
                 << " beforeQueueOutPkts: " << beforeQueueOutPkts
                 << " beforeQueueOutBytes: " << beforeQueueOutBytes
                 << " txPacketSize: " << txPacketSize
                 << " afterQueueOutPkts: " << afterQueueOutPkts
                 << " afterQueueOutBytes: " << afterQueueOutBytes;

      EXPECT_EVENTUALLY_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
      // CS00012267635: debug why queue counter is 362, when txPacketSize is
      // 322
      EXPECT_EVENTUALLY_GE(afterQueueOutBytes, beforeQueueOutBytes);

      for (auto i = 0;
           i < utility::getCoppHighPriQueueId(getAgentEnsemble()->getL3Asics());
           i++) {
        auto [outPkts, outBytes] =
            utility::getCpuQueueOutPacketsAndBytes(getSw(), i, switchId);
        XLOG(DBG2) << "QueueID: " << i << " outPkts: " << outPkts
                   << " outBytes: " << outBytes;
      }
    });
  };

  verifyAcrossWarmBoots([] {}, verify);
}

void AgentVoqSwitchTest::sendLocalServiceDiscoveryMulticastPacket(
    const PortID outPort,
    const int numPackets) {
  auto vlanId = utility::firstVlanID(getProgrammedState());
  auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
  auto srcIp = folly::IPAddressV6("fe80::ff:fe00:f0b");
  auto dstIp = folly::IPAddressV6("ff15::efc0:988f");
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
  std::vector<uint8_t> serviceDiscoveryPayload = {
      0x42, 0x54, 0x2d, 0x53, 0x45, 0x41, 0x52, 0x43, 0x48, 0x20, 0x2a, 0x20,
      0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, 0x0d, 0x0a, 0x48, 0x6f,
      0x73, 0x74, 0x3a, 0x20, 0x5b, 0x66, 0x66, 0x31, 0x35, 0x3a, 0x3a, 0x65,
      0x66, 0x63, 0x30, 0x3a, 0x39, 0x38, 0x38, 0x66, 0x5d, 0x3a, 0x36, 0x37,
      0x37, 0x31, 0x0d, 0x0a, 0x50, 0x6f, 0x72, 0x74, 0x3a, 0x20, 0x36, 0x38,
      0x38, 0x31, 0x0d, 0x0a, 0x49, 0x6e, 0x66, 0x6f, 0x68, 0x61, 0x73, 0x68,
      0x3a, 0x20, 0x61, 0x66, 0x31, 0x37, 0x34, 0x36, 0x35, 0x39, 0x64, 0x37,
      0x31, 0x31, 0x38, 0x64, 0x32, 0x34, 0x34, 0x61, 0x30, 0x36, 0x31, 0x33};
  for (int i = 0; i < numPackets; i++) {
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        srcIp,
        dstIp,
        6771,
        6771,
        0,
        254,
        serviceDiscoveryPayload);
    getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
  }
}

int AgentVoqSwitchTest::sendPacket(
    const folly::IPAddress& dstIp,
    std::optional<PortID> frontPanelPort,
    std::optional<std::vector<uint8_t>> payload,
    int dscp) {
  folly::IPAddress kSrcIp(dstIp.isV6() ? "1::1" : "1.0.0.1");
  const auto srcMac = utility::kLocalCpuMac();
  const auto dstMac = utility::kLocalCpuMac();

  auto txPacket = utility::makeUDPTxPacket(
      getSw(),
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
    getSw()->sendPacketOutOfPortAsync(std::move(txPacket), *frontPanelPort);
  } else {
    getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }
  return txPacketSize;
}

SystemPortID AgentVoqSwitchTest::getSystemPortID(
    const PortDescriptor& port,
    cfg::Scope portScope) {
  auto switchId = scopeResolver().scope(getProgrammedState(), port).switchId();
  const auto& dsfNode =
      getProgrammedState()->getDsfNodes()->getNodeIf(switchId);
  auto sysPortOffset = portScope == cfg::Scope::GLOBAL
      ? dsfNode->getGlobalSystemPortOffset()
      : dsfNode->getLocalSystemPortOffset();
  CHECK(sysPortOffset.has_value());
  return SystemPortID(port.intID() + *sysPortOffset);
}

void AgentVoqSwitchTest::addDscpAclWithCounter() {
  auto newCfg = initialConfig(*getAgentEnsemble());
  auto* acl = utility::addAcl(&newCfg, kDscpAclName());
  auto asic = utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
  acl->dscp() = 0x24;
  utility::addEtherTypeToAcl(asic, acl, cfg::EtherType::IPv6);
  utility::addAclStat(
      &newCfg,
      kDscpAclName(),
      kDscpAclCounterName(),
      utility::getAclCounterTypes(getAgentEnsemble()->getL3Asics()));
  applyNewConfig(newCfg);
}

void AgentVoqSwitchTest::addRemoveNeighbor(PortDescriptor port, bool add) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  if (add) {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, {port});
    });
  } else {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.unresolveNextHops(in, {port});
    });
  }
}

void AgentVoqSwitchTest::setForceTrafficOverFabric(bool force) {
  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    auto out = in->clone();
    for (const auto& [_, switchSetting] :
         std::as_const(*out->getSwitchSettings())) {
      auto newSwitchSettings = switchSetting->modify(&out);
      newSwitchSettings->setForceTrafficOverFabric(force);
    }
    return out;
  });
}

std::vector<PortDescriptor> AgentVoqSwitchTest::getInterfacePortSysPortDesc() {
  auto ports = getProgrammedState()->getPorts()->getAllNodes();
  std::vector<PortDescriptor> portDescs;
  std::for_each(
      ports->begin(), ports->end(), [this, &portDescs](const auto& idAndPort) {
        const auto port = idAndPort.second;
        if (port->getPortType() == cfg::PortType::INTERFACE_PORT) {
          portDescs.push_back(PortDescriptor(getSystemPortID(
              PortDescriptor(port->getID()), cfg::Scope::GLOBAL)));
        }
      });
  return portDescs;
}

// Resolve and return list of local nhops (only NIF ports)
std::vector<PortDescriptor> AgentVoqSwitchTest::resolveLocalNhops(
    utility::EcmpSetupTargetedPorts6& ecmpHelper) {
  std::vector<PortDescriptor> portDescs = getInterfacePortSysPortDesc();

  applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    auto out = in->clone();
    for (const auto& portDesc : portDescs) {
      out = ecmpHelper.resolveNextHops(out, {portDesc});
    }
    return out;
  });
  return portDescs;
}

TEST_F(AgentVoqSwitchTest, fdrRciAndCoreRciWatermarks) {
  auto verify = [this]() {
    std::string out;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      getAgentEnsemble()->runDiagCommand(
          "setreg CIG_RCI_DEVICE_MAPPING 0\nsetreg CIG_RCI_CORE_MAPPING 0\n",
          out,
          switchId);
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
    }

    uint64_t fdrRciWatermarkBytes{};
    uint64_t coreRciWatermarkBytes{};

    WITH_RETRIES({
      getSw()->updateStats();
      for (const auto& switchWatermarksIter : getAllSwitchWatermarkStats()) {
        auto switchWatermarks = switchWatermarksIter.second;
        if (switchWatermarks.fdrRciWatermarkBytes().has_value()) {
          fdrRciWatermarkBytes +=
              switchWatermarks.fdrRciWatermarkBytes().value();
        }
        if (switchWatermarks.coreRciWatermarkBytes().has_value()) {
          coreRciWatermarkBytes +=
              switchWatermarks.coreRciWatermarkBytes().value();
        }
      }
      // Make sure that both counters have non zero values
      EXPECT_EVENTUALLY_GT(fdrRciWatermarkBytes, 0);
      EXPECT_EVENTUALLY_GT(coreRciWatermarkBytes, 0);
      XLOG(DBG2) << "FDR RCI watermark bytes : " << fdrRciWatermarkBytes
                 << ", Core DRM RCI watermark bytes : "
                 << coreRciWatermarkBytes;
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchTest, addRemoveNeighbor) {
  auto setup = [this]() {
    const PortDescriptor kPortDesc(getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[0]);
    // Add neighbor
    addRemoveNeighbor(kPortDesc, true);
    // Remove neighbor
    addRemoveNeighbor(kPortDesc, false);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchTest, sendPacketCpuAndFrontPanel) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);

  auto setup = [this, kPortDesc, ecmpHelper]() {
    if (isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)) {
      addDscpAclWithCounter();
    }
    addRemoveNeighbor(kPortDesc, true /* add neighbor*/);
  };

  auto verify = [this, kPortDesc, ecmpHelper]() {
    auto sendPacketCpuFrontPanelHelper = [this, kPortDesc, ecmpHelper](
                                             bool isFrontPanel) {
      auto getPortOutPktsBytes = [this](PortID port) {
        return std::make_pair(
            getLatestPortStats(port).get_outUnicastPkts_(),
            getLatestPortStats(port).get_outBytes_());
      };

      auto getAllQueueOutPktsBytes = [kPortDesc, this]() {
        return std::make_pair(
            getLatestPortStats(kPortDesc.phyPortID()).get_queueOutPackets_(),
            getLatestPortStats(kPortDesc.phyPortID()).get_queueOutBytes_());
      };
      auto getAllVoQOutBytes = [kPortDesc, this]() {
        return getLatestSysPortStats(
                   getSystemPortID(kPortDesc, cfg::Scope::GLOBAL))
            .get_queueOutBytes_();
      };
      auto getAclPackets = [this]() {
        return utility::getAclInOutPackets(getSw(), kDscpAclCounterName());
      };

      auto printQueueStats = [](std::string queueStatDesc,
                                std::string packetsOrBytes,
                                std::map<int16_t, int64_t> queueStatsMap) {
        for (const auto& [queueId, pktsOrBytes] : queueStatsMap) {
          XLOG(DBG2) << queueStatDesc << ": Queue ID: " << queueId << ", "
                     << packetsOrBytes << ": " << pktsOrBytes;
        }
      };
      auto getRecyclePortPkts = [this]() {
        return *getLatestPortStats(PortID(1)).inUnicastPkts_();
      };

      int64_t beforeQueueOutPkts = 0, beforeQueueOutBytes = 0;
      int64_t afterQueueOutPkts = 0, afterQueueOutBytes = 0;
      int64_t beforeVoQOutBytes = 0, afterVoQOutBytes = 0;
      int64_t egressCoreWatermarkBytes = 0;
      // Get SRAM size per core as thats the highest possible free SRAM
      const uint64_t kSramSize =
          utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
              ->getSramSizeBytes() /
          utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
              ->getNumCores();
      int64_t sramMinBufferWatermarkBytes = kSramSize + 1;

      if (isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
        auto beforeAllQueueOut = getAllQueueOutPktsBytes();
        beforeQueueOutPkts = beforeAllQueueOut.first.at(kDefaultEgressQueue);
        beforeQueueOutBytes = beforeAllQueueOut.second.at(kDefaultEgressQueue);
        printQueueStats("Before Queue Out", "Packets", beforeAllQueueOut.first);
        printQueueStats("Before Queue Out", "Bytes", beforeAllQueueOut.second);
        auto beforeAllVoQOutBytes = getAllVoQOutBytes();
        beforeVoQOutBytes = beforeAllVoQOutBytes.at(utility::getDefaultQueue());
        printQueueStats("Before VoQ Out", "Bytes", beforeAllVoQOutBytes);
      }

      auto [beforeOutPkts, beforeOutBytes] =
          getPortOutPktsBytes(kPortDesc.phyPortID());
      auto beforeAclPkts =
          isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)
          ? getAclPackets()
          : 0;
      std::optional<PortID> frontPanelPort;
      uint64_t beforeFrontPanelOutBytes{0}, beforeFrontPanelOutPkts{0};
      if (isFrontPanel) {
        frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
        std::tie(beforeFrontPanelOutPkts, beforeFrontPanelOutBytes) =
            getPortOutPktsBytes(*frontPanelPort);
      }
      auto beforeRecyclePkts = getRecyclePortPkts();
      auto beforeSwitchDropStats = getAggregatedSwitchDropStats();
      auto txPacketSize = sendPacket(ecmpHelper.ip(kPortDesc), frontPanelPort);

      auto [maxRetryCount, sleepTimeMsecs] =
          utility::getRetryCountAndDelay(getSw()->getHwAsicTable());
      WITH_RETRIES_N_TIMED(
          maxRetryCount, std::chrono::milliseconds(sleepTimeMsecs), {
            if (isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
              auto afterAllQueueOut = getAllQueueOutPktsBytes();
              afterQueueOutPkts =
                  afterAllQueueOut.first.at(kDefaultEgressQueue);
              afterQueueOutBytes =
                  afterAllQueueOut.second.at(kDefaultEgressQueue);
              auto afterAllVoQOutBytes = getAllVoQOutBytes();
              afterVoQOutBytes =
                  afterAllVoQOutBytes.at(utility::getDefaultQueue());
              printQueueStats("After VoQ Out", "Bytes", afterAllVoQOutBytes);
            }
            auto afterAclPkts =
                isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)
                ? getAclPackets()
                : 0;
            auto portOutPktsAndBytes =
                getPortOutPktsBytes(kPortDesc.phyPortID());
            auto afterOutPkts = portOutPktsAndBytes.first;
            auto afterOutBytes = portOutPktsAndBytes.second;
            uint64_t afterFrontPanelOutBytes{0}, afterFrontPanelOutPkts{0};
            if (isFrontPanel) {
              std::tie(afterFrontPanelOutPkts, afterFrontPanelOutBytes) =
                  getPortOutPktsBytes(*frontPanelPort);
            }
            auto afterRecyclePkts = getRecyclePortPkts();
            for (const auto& switchWatermarksIter :
                 getAllSwitchWatermarkStats()) {
              if (switchWatermarksIter.second.egressCoreBufferWatermarkBytes()
                      .has_value()) {
                egressCoreWatermarkBytes +=
                    switchWatermarksIter.second.egressCoreBufferWatermarkBytes()
                        .value();
              }
              if (switchWatermarksIter.second.sramMinBufferWatermarkBytes()
                      .has_value()) {
                sramMinBufferWatermarkBytes = std::min(
                    sramMinBufferWatermarkBytes,
                    *switchWatermarksIter.second.sramMinBufferWatermarkBytes());
              }
            }
            XLOG(DBG2) << "Verifying: "
                       << (isFrontPanel ? "Send Packet from Front Panel Port"
                                        : "Send Packet from CPU Port")
                       << " Stats:: beforeOutPkts: " << beforeOutPkts
                       << " beforeOutBytes: " << beforeOutBytes
                       << " beforeQueueOutPkts: " << beforeQueueOutPkts
                       << " beforeQueueOutBytes: " << beforeQueueOutBytes
                       << " beforeVoQOutBytes: " << beforeVoQOutBytes
                       << " beforeFrontPanelPkts: " << beforeFrontPanelOutPkts
                       << " beforeFrontPanelBytes: " << beforeFrontPanelOutBytes
                       << " beforeRecyclePkts: " << beforeRecyclePkts
                       << " txPacketSize: " << txPacketSize
                       << " afterOutPkts: " << afterOutPkts
                       << " afterOutBytes: " << afterOutBytes
                       << " afterQueueOutPkts: " << afterQueueOutPkts
                       << " afterQueueOutBytes: " << afterQueueOutBytes
                       << " afterVoQOutBytes: " << afterVoQOutBytes
                       << " afterAclPkts: " << afterAclPkts
                       << " afterFrontPanelPkts: " << afterFrontPanelOutPkts
                       << " afterFrontPanelBytes: " << afterFrontPanelOutBytes
                       << " afterRecyclePkts: " << afterRecyclePkts
                       << " egressCoreWatermarkBytes: "
                       << egressCoreWatermarkBytes
                       << " sramMinBufferWatermarkBytes: "
                       << sramMinBufferWatermarkBytes;

            EXPECT_EVENTUALLY_EQ(afterOutPkts - 1, beforeOutPkts);
            int extraByteOffset = 0;
            auto asic =
                utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
            const auto asicMode = asic->getAsicMode();
            const auto asicType = asic->getAsicType();
            if (asic->getAsicMode() != HwAsic::AsicMode::ASIC_MODE_SIM) {
              // Account for Ethernet FCS being counted in TX out bytes.
              extraByteOffset = utility::EthFrame::FCS_SIZE;
            }
            EXPECT_EVENTUALLY_EQ(
                afterOutBytes - txPacketSize - extraByteOffset, beforeOutBytes);
            if (isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
              EXPECT_EVENTUALLY_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
              // CS00012267635: debug why queue counter is 310, when
              // txPacketSize is 322
              EXPECT_EVENTUALLY_GE(afterQueueOutBytes, beforeQueueOutBytes);
            }
            if (isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)) {
              EXPECT_EVENTUALLY_GT(afterAclPkts, beforeAclPkts);
            }
            if (isSupportedOnAllAsics(HwAsic::Feature::VOQ)) {
              EXPECT_EVENTUALLY_GT(afterVoQOutBytes, beforeVoQOutBytes);
            }
            if (frontPanelPort) {
              EXPECT_EVENTUALLY_GT(
                  afterFrontPanelOutBytes, beforeFrontPanelOutBytes);

              EXPECT_EVENTUALLY_GT(
                  afterFrontPanelOutPkts, beforeFrontPanelOutPkts);
            } else if (asicMode != HwAsic::AsicMode::ASIC_MODE_SIM) {
              EXPECT_EVENTUALLY_EQ(beforeRecyclePkts + 1, afterRecyclePkts);
            }
            auto afterSwitchDropStats = getAggregatedSwitchDropStats();
            if (asicMode != HwAsic::AsicMode::ASIC_MODE_SIM &&
                asicType == cfg::AsicType::ASIC_TYPE_JERICHO3) {
              XLOG(DBG2) << " Queue resolution drops, before: "
                         << *beforeSwitchDropStats.queueResolutionDrops()
                         << " after: "
                         << *afterSwitchDropStats.queueResolutionDrops();
              EXPECT_EVENTUALLY_EQ(
                  *afterSwitchDropStats.queueResolutionDrops(),
                  *beforeSwitchDropStats.queueResolutionDrops() + 1);
            }
            if (isSupportedOnAllAsics(
                    HwAsic::Feature::EGRESS_CORE_BUFFER_WATERMARK)) {
              EXPECT_EVENTUALLY_GT(egressCoreWatermarkBytes, 0);
            }
            if (isSupportedOnAllAsics(
                    HwAsic::Feature::INGRESS_SRAM_MIN_BUFFER_WATERMARK)) {
              EXPECT_EVENTUALLY_LE(sramMinBufferWatermarkBytes, kSramSize);
            }
          });
    };

    sendPacketCpuFrontPanelHelper(false /* cpu */);
    sendPacketCpuFrontPanelHelper(true /* front panel*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, trapPktsOnPort) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc, &ecmpHelper]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    auto l3Asics = getAgentEnsemble()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    utility::addTrapPacketAcl(asic, &cfg, kPortDesc.phyPortID());
    applyNewConfig(cfg);
    applyNewState([=](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, {kPortDesc});
    });
  };
  auto verify = [this, kPortDesc, &ecmpHelper]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snoop");
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    sendPacket(ecmpHelper.ip(kPortDesc), frontPanelPort);
    WITH_RETRIES({
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, rxPacketToCpu) {
  rxPacketToCpuHelper(
      utility::kNonSpecialPort1,
      utility::kNonSpecialPort2,
      utility::getCoppMidPriQueueId(getAgentEnsemble()->getL3Asics()));
}

TEST_F(AgentVoqSwitchTest, rxPacketToCpuBgpDstPort) {
  rxPacketToCpuHelper(
      utility::kNonSpecialPort1,
      utility::kBgpPort,
      utility::getCoppHighPriQueueId(getAgentEnsemble()->getL3Asics()));
}

TEST_F(AgentVoqSwitchTest, rxPacketToCpuBgpSrcPort) {
  rxPacketToCpuHelper(
      utility::kBgpPort,
      utility::kNonSpecialPort1,
      utility::getCoppHighPriQueueId(getAgentEnsemble()->getL3Asics()));
}

TEST_F(AgentVoqSwitchTest, localForwardingPostIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPortDesc, true /* add neighbor*/);
  };

  auto verify = [this, kPortDesc, &ecmpHelper]() {
    auto sendPktAndVerify = [&](std::optional<PortID> portToSendFrom) {
      auto beforePkts =
          getLatestPortStats(kPortDesc.phyPortID()).get_outUnicastPkts_();
      sendPacket(ecmpHelper.ip(kPortDesc), portToSendFrom);
      WITH_RETRIES({
        auto afterPkts =
            getLatestPortStats(kPortDesc.phyPortID()).get_outUnicastPkts_();
        XLOG(DBG2) << "Before pkts: " << beforePkts
                   << " After pkts: " << afterPkts;
        EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 1);
      });
    };
    // CPU send
    sendPktAndVerify(std::nullopt);
    // Front panel send
    sendPktAndVerify(ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, stressLocalForwardingPostIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPortDesc, true /* add neighbor*/);
  };

  auto verify = [this, kPortDesc, &ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPortDesc.phyPortID()).get_outUnicastPkts_();
    for (auto i = 0; i < 10000; ++i) {
      // CPU send
      sendPacket(ecmpHelper.ip(kPortDesc), std::nullopt);
      // Front panel send
      sendPacket(
          ecmpHelper.ip(kPortDesc),
          ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPortDesc.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 20000);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, localSystemPortEcmp) {
  auto setup = [this]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    auto prefix = RoutePrefixV6{folly::IPAddressV6("1::1"), 128};
    flat_set<PortDescriptor> localSysPorts;
    for (auto& systemPortMap :
         std::as_const(*getProgrammedState()->getSystemPorts())) {
      for (auto& [_, localSysPort] : std::as_const(*systemPortMap.second)) {
        localSysPorts.insert(PortDescriptor(localSysPort->getID()));
      }
    }
    applyNewState([=](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, localSysPorts);
    });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, localSysPorts, {prefix});
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchTest, packetIntegrityError) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  auto port = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [=, this]() { addRemoveNeighbor(port, true /*add*/); };
  auto verify = [=, this]() {
    const auto dstIp = ecmpHelper.ip(port);
    auto switchId = scopeResolver().scope(port.phyPortID()).switchId();
    auto switchAsic = getSw()->getHwAsicTable()->getHwAsic(switchId);
    std::string out;
    if (switchAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
      getAgentEnsemble()->runDiagCommand(
          "m SPB_FORCE_CRC_ERROR FORCE_CRC_ERROR_ON_DATA=1 FORCE_CRC_ERROR_ON_CRC=1\n",
          out);
    } else if (switchAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      getAgentEnsemble()->runDiagCommand(
          "m IRE_FORCE_CRC_ERROR FORCE_CRC_ERROR_ON_CRC=1\n", out);
    } else {
      throw FbossError(
          "Unsupported ASIC type: ",
          apache::thrift::util::enumNameSafe(switchAsic->getAsicType()));
    }
    getAgentEnsemble()->runDiagCommand("quit\n", out);
    sendPacket(dstIp, std::nullopt, std::vector<uint8_t>(1024, 0xff));
    WITH_RETRIES({
      auto switchIndex =
          getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      auto pktIntegrityDrops =
          switchStats.switchDropStats()->packetIntegrityDrops().value_or(0);
      auto corruptedCellPacketIntegrityDrops =
          switchStats.switchDropStats()
              ->corruptedCellPacketIntegrityDrops()
              .value_or(0);
      XLOG(INFO) << " Packet integrity drops: " << pktIntegrityDrops
                 << " Corrupted cell integrity drops: "
                 << corruptedCellPacketIntegrityDrops;
      EXPECT_EVENTUALLY_GT(pktIntegrityDrops, 0);
      // TODO: corruptedCellPacketIntegrityDrops should increment here.
      // But its not, uncomment the check below once we get
      // CS00012336139 resolved
      // EXPECT_EVENTUALLY_GT(corruptedCellPacketIntegrityDrops, 0);
    });
    // Assert that packet Integrity drops don't continuously increment.
    // Packet integrity drop counter is clear on read from HW. So we
    // accumulate its value in memory. If HW/SDK ever changed this to
    // not be clear on read, but cumulative, then our approach would
    // yield constantly increasing values. Assert against that.
    checkStatsStabilize(30);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, dramEnqueueDequeueBytes) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc]() {
    addRemoveNeighbor(kPortDesc, true /* add neighbor*/);
  };

  auto verify = [this, kPortDesc, &ecmpHelper]() {
    // Disable both port TX and credit watchdog
    utility::setCreditWatchdogAndPortTx(
        getAgentEnsemble(), kPortDesc.phyPortID(), false);
    auto sendPkts = [this, kPortDesc, &ecmpHelper]() {
      for (auto i = 0; i < 1000; ++i) {
        sendPacket(ecmpHelper.ip(kPortDesc), std::nullopt);
      }
    };
    int64_t dramEnqueuedBytes = 0;
    auto switchId = scopeResolver().scope(kPortDesc.phyPortID()).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    WITH_RETRIES({
      sendPkts();
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      dramEnqueuedBytes =
          *switchStats.fb303GlobalStats()->dram_enqueued_bytes();
      XLOG(DBG2) << "Dram enqueued bytes : " << dramEnqueuedBytes;
      EXPECT_EVENTUALLY_GT(dramEnqueuedBytes, 0);
    });
    // Enable port TX
    utility::setPortTx(getAgentEnsemble(), kPortDesc.phyPortID(), true);
    WITH_RETRIES({
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      auto dramDequeuedBytes =
          *switchStats.fb303GlobalStats()->dram_dequeued_bytes();
      XLOG(DBG2) << "Dram dequeued bytes : " << dramDequeuedBytes;
      EXPECT_EVENTUALLY_GT(dramDequeuedBytes, 0);
    });
    // Assert that Dram enqueue/dequeue bytes don't continuously increment
    // Eventually all pkts should be dequeued and we should stop getting
    // increments
    checkStatsStabilize(60);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, verifyQueueLatencyWatermark) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  const uint64_t kLocalVoqMaxExpectedLatencyNsec{10000};
  const uint64_t kRemoteL1VoqMaxExpectedLatencyNsec{100000};
  const uint64_t kRemoteL2VoqMaxExpectedLatencyNsec{100000};
  const uint64_t kOutOfBoundsLatencyNsec{2147483647};
  auto setup = [&]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    cfg.switchSettings()->localVoqMaxExpectedLatencyNsec() =
        kLocalVoqMaxExpectedLatencyNsec;
    cfg.switchSettings()->remoteL1VoqMaxExpectedLatencyNsec() =
        kRemoteL1VoqMaxExpectedLatencyNsec;
    cfg.switchSettings()->remoteL2VoqMaxExpectedLatencyNsec() =
        kRemoteL2VoqMaxExpectedLatencyNsec;
    cfg.switchSettings()->voqOutOfBoundsLatencyNsec() = kOutOfBoundsLatencyNsec;
    applyNewConfig(cfg);
    addRemoveNeighbor(kPortDesc, true /* add neighbor*/);
  };

  auto verify = [&]() {
    auto queueId = utility::getDefaultQueue();
    auto dscpForQueue =
        utility::kOlympicQueueToDscp().find(queueId)->second.at(0);
    auto sendPkts = [this, kPortDesc, &ecmpHelper, dscpForQueue]() {
      for (auto i = 0; i < 10000; ++i) {
        sendPacket(
            ecmpHelper.ip(kPortDesc),
            std::nullopt,
            std::vector<uint8_t>(4000),
            dscpForQueue);
      }
    };
    auto waitForQueueLatencyWatermark =
        [this, &kPortDesc, &queueId](uint64_t expectedLatencyWatermarkNsec) {
          uint64_t queueLatencyWatermarkNsec;
          WITH_RETRIES({
            queueLatencyWatermarkNsec =
                getLatestSysPortStats(
                    getSystemPortID(kPortDesc, cfg::Scope::GLOBAL))
                    .queueLatencyWatermarkNsec_()[queueId];
            XLOG(DBG2) << "Port: " << kPortDesc.phyPortID()
                       << " voq queueId: " << queueId
                       << " latency watermark: " << queueLatencyWatermarkNsec
                       << " nsec";
            EXPECT_EVENTUALLY_EQ(
                queueLatencyWatermarkNsec, expectedLatencyWatermarkNsec);
          });
        };
    // Disable both port TX and credit watchdog
    utility::setCreditWatchdogAndPortTx(
        getAgentEnsemble(), kPortDesc.phyPortID(), false);
    // Send packets and let it sit in the VoQ
    sendPkts();
    sleep(1);
    // Enable port TX
    utility::setPortTx(getAgentEnsemble(), kPortDesc.phyPortID(), true);
    // VoQ latency exceeded the configured max latency
    waitForQueueLatencyWatermark(kOutOfBoundsLatencyNsec);
    // Now, send packets without any delays
    sendPkts();
    // VoQ latency is less than max expected for local VoQ
    waitForQueueLatencyWatermark(kLocalVoqMaxExpectedLatencyNsec);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
