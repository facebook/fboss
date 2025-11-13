// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/format.h>
#include <re2/re2.h>
#include <filesystem>
#include <string>

#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
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

std::string AgentVoqSwitchTest::getSdkMajorVersion(const SwitchID& switchId) {
  std::string majorVersion;

  static const re2::RE2 bcmSaiPattern("BRCM SAI ver: \\[(\\d+)\\.");
  std::string output;
  // Start with a blank command first and then get the BCM SAI version
  // This is because, sometimes first diag command doesn't work in some SDK
  // versions
  getAgentEnsemble()->runDiagCommand("\n", output, switchId);
  getAgentEnsemble()->runDiagCommand("bcmsai ver\n", output, switchId);
  if (RE2::PartialMatch(output, bcmSaiPattern, &majorVersion)) {
    return majorVersion;
  }

  // Return 0 by default
  return "0";
}

void AgentVoqSwitchTest::rxPacketToCpuHelper(
    uint16_t l4SrcPort,
    uint16_t l4DstPort,
    uint8_t queueId) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
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
  auto vlanId = getVlanIDForTx();
  auto intfMac =
      utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
  auto srcIp = folly::IPAddressV6("fe80::ff:fe00:f0b");
  auto dstIp = folly::IPAddressV6("ff15::efc0:988f");
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
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

void AgentVoqSwitchTest::addDscpAclWithCounter() {
  auto newCfg = initialConfig(*getAgentEnsemble());
  auto* acl = utility::addAcl_DEPRECATED(&newCfg, kDscpAclName());
  auto asic = checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
  acl->dscp() = 0x24;
  utility::addEtherTypeToAcl(asic, acl, cfg::EtherType::IPv6);
  utility::addAclStat(
      &newCfg,
      kDscpAclName(),
      kDscpAclCounterName(),
      utility::getAclCounterTypes(getAgentEnsemble()->getL3Asics()));
  applyNewConfig(newCfg);
}

void AgentVoqSwitchTest::addRemoveNeighbor(
    PortDescriptor port,
    NeighborOp operation) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  switch (operation) {
    case NeighborOp::ADD:
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(in, {port});
      });
      break;
    case NeighborOp::DEL:
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.unresolveNextHops(in, {port});
      });
      break;
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

void AgentVoqSwitchTest::setupForDramErrorTestFromDiagShell(
    const SwitchID& switchId) {
  std::string out;
  getAgentEnsemble()->runDiagCommand(
      "s CGM_DRAM_BOUND_STATE_TH 0\n", out, switchId);
  getAgentEnsemble()->runDiagCommand(
      "m CGM_DRAM_BOUND_STATE_TH  DRAM_BOUND_TOTAL_FREE_SRAM_BUFFERS_TH=0xFFF DRAM_BOUND_TOTAL_FREE_SRAM_PDBS_TH=0xFFF\n",
      out,
      switchId);
  getAgentEnsemble()->runDiagCommand(
      "mod CGM_VOQ_DRAM_BOUND_PRMS 0 127 SRAM_BUFFERS_BOUND_FREE_MAX_TH=0x0 SRAM_BUFFERS_BOUND_FREE_MIN_TH=0x0 SRAM_BUFFERS_BOUND_MAX_TH=0x0 SRAM_BUFFERS_BOUND_MIN_TH=0x0\n",
      out,
      switchId);
  getAgentEnsemble()->runDiagCommand(
      "m IPS_DRAM_ONLY_PROFILE  DRAM_ONLY_PROFILE=-1\n", out, switchId);
  getAgentEnsemble()->runDiagCommand(
      "mod CGM_VOQ_SRAM_DRAM_MODE 0 127 VOQ_SRAM_DRAM_MODE_DATA=0x2\n",
      out,
      switchId);
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
    const PortDescriptor kPortDesc(
        getAgentEnsemble()->masterLogicalPortIds(
            {cfg::PortType::INTERFACE_PORT})[0]);
    // Add neighbor
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
    // Remove neighbor
    addRemoveNeighbor(kPortDesc, NeighborOp::DEL);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchTest, sendPacketCpuAndFrontPanel) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);

  auto setup = [this, kPortDesc, ecmpHelper]() {
    if (isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)) {
      addDscpAclWithCounter();
    }
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
  };

  auto verify = [this, kPortDesc, ecmpHelper]() {
    auto sendPacketCpuFrontPanelHelper = [this, kPortDesc, ecmpHelper](
                                             bool isFrontPanel) {
      auto getPortOutPktsBytes = [this](PortID port) {
        return std::make_pair(
            folly::copy(getLatestPortStats(port).outUnicastPkts_().value()),
            folly::copy(getLatestPortStats(port).outBytes_().value()));
      };

      auto getAllQueueOutPktsBytes = [kPortDesc, this]() {
        return std::make_pair(
            folly::copy(getLatestPortStats(kPortDesc.phyPortID())
                            .queueOutPackets_()
                            .value()),
            folly::copy(getLatestPortStats(kPortDesc.phyPortID())
                            .queueOutBytes_()
                            .value()));
      };
      auto getAllVoQOutBytes = [kPortDesc, this]() {
        return folly::copy(getLatestSysPortStats(
                               getSystemPortID(kPortDesc, cfg::Scope::GLOBAL))
                               .queueOutBytes_()
                               .value());
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
                       << " afterRecyclePkts: " << afterRecyclePkts;

            EXPECT_EVENTUALLY_EQ(afterOutPkts - 1, beforeOutPkts);
            int extraByteOffset = 0;
            auto asic = checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
            const auto asicMode = asic->getAsicMode();
            const auto asicType = asic->getAsicType();
            if (asic->getAsicMode() != HwAsic::AsicMode::ASIC_MODE_SIM) {
              // Account for Ethernet FCS being counted in TX out bytes.
              extraByteOffset = utility::EthFrame::FCS_SIZE;
              if (FLAGS_hyper_port) {
                extraByteOffset += utility::EthFrame::HYPER_PORT_HEADER_SIZE;
              }
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
          });
    };

    sendPacketCpuFrontPanelHelper(false /* cpu */);
    sendPacketCpuFrontPanelHelper(true /* front panel*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, trapPktsOnPort) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc, &ecmpHelper]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    auto l3Asics = getAgentEnsemble()->getL3Asics();
    auto asic = checkSameAndGetAsic(l3Asics);
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
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
  };

  auto verify = [this, kPortDesc, &ecmpHelper]() {
    auto sendPktAndVerify = [&](std::optional<PortID> portToSendFrom) {
      auto beforePkts = folly::copy(
          getLatestPortStats(kPortDesc.phyPortID()).outUnicastPkts_().value());
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
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
  };

  auto verify = [this, kPortDesc, &ecmpHelper]() {
    auto beforePkts = folly::copy(
        getLatestPortStats(kPortDesc.phyPortID()).outUnicastPkts_().value());
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
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto prefix = RoutePrefixV6{folly::IPAddressV6("1::1"), 128};
    flat_set<PortDescriptor> localSysPorts;
    for (auto& systemPortMap :
         std::as_const(*getProgrammedState()->getSystemPorts())) {
      for (auto& [_, localSysPort] : std::as_const(*systemPortMap.second)) {
        PortDescriptor portDesc = PortDescriptor(localSysPort->getID());
        // no need to configure RIF interface for hyper port members
        if (ecmpHelper.getInterface(portDesc, getProgrammedState())) {
          localSysPorts.insert(portDesc);
        }
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
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  auto port = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [=, this]() { addRemoveNeighbor(port, NeighborOp::ADD); };
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
      for (const auto& switchIdx : getSw()->getHwAsicTable()->getSwitchIDs()) {
        getAgentEnsemble()->runDiagCommand(
            "m IRE_FORCE_CRC_ERROR FORCE_CRC_ERROR_ON_CRC=1\n", out, switchIdx);
      }
    } else {
      throw FbossError(
          "Unsupported ASIC type: ",
          apache::thrift::util::enumNameSafe(switchAsic->getAsicType()));
    }
    for (const auto& switchIdx : getSw()->getHwAsicTable()->getSwitchIDs()) {
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchIdx);
    }
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
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPortDesc]() {
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
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
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
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
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
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
    XLOG(DBG2) << "Disabling port TX and credit watchdog";
    utility::setCreditWatchdogAndPortTx(
        getAgentEnsemble(), kPortDesc.phyPortID(), false);

    // Send packets and let it sit in the VoQ
    XLOG(DBG2) << "Send packets";
    sendPkts();
    XLOG(DBG2) << "Check VoQ latency watermark, expect out of bound";
    // VoQ latency exceeded the configured max latency
    waitForQueueLatencyWatermark(kOutOfBoundsLatencyNsec);
    // Enable port TX
    XLOG(DBG2) << "Enabling port TX";
    utility::setPortTx(getAgentEnsemble(), kPortDesc.phyPortID(), true);

    // Now, send packets without any delays
    sendPkts();
    // VoQ latency is less than max expected for local VoQ
    waitForQueueLatencyWatermark(kLocalVoqMaxExpectedLatencyNsec);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, verifyDramErrorDetection) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [&]() { addRemoveNeighbor(kPortDesc, NeighborOp::ADD); };
  auto verify = [&]() {
    // The DRAM error will only be for the switchID on which packet
    // is egressing, hence the test is limited to a single switchId.
    SwitchID switchId = scopeResolver().scope(kPortDesc.phyPortID()).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    WITH_RETRIES({
      std::string out;
      setupForDramErrorTestFromDiagShell(switchId);
      getAgentEnsemble()->runDiagCommand(
          "m DDP_ERR_INITIATE BUFF_CRC_INITIATE_ERR=1\n", out, switchId);
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
      sendPacket(
          ecmpHelper.ip(kPortDesc), std::nullopt, std::vector<uint8_t>(4000));
      getSw()->updateStats();
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      ASSERT_EVENTUALLY_TRUE(
          switchStats.hwAsicErrors()->dramErrors().has_value());
      EXPECT_EVENTUALLY_GT(switchStats.hwAsicErrors()->dramErrors().value(), 0);
      XLOG(DBG0) << "Dram Error count: "
                 << switchStats.hwAsicErrors()->dramErrors().value();
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, verifyDramBufferQuarantine) {
  auto generateDramErrorCint = [](int port1, int port2) {
    // By default, need 10 errors in a memory to trigger quarantine.
    // With packet inject test, each packet can trigger error in a
    // different memory. Sending a lot of packets with the CINT to
    // trigger errors such that some memory will see > 10 errors
    // and will be quarantined.
    return fmt::format(
        R"(
        cint_reset();
        print bcm_port_force_forward_set(0, {0}, {1}, 1);
        int loop=0;
        for (loop=0; loop<1000; loop++) {{
          bshell(0, "m DDP_ERR_INITIATE BUFF_CRC_INITIATE_ERR=1");
          bshell(0, "tx 1 visibility=0 psrc={0}");
        }})",
        port1,
        port2);
  };
  auto verify = [&]() {
    SwitchID switchId =
        scopeResolver().scope(masterLogicalInterfacePortIds()[0]).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    auto dramErrorCint = generateDramErrorCint(
        masterLogicalInterfacePortIds()[0], masterLogicalInterfacePortIds()[1]);
    WITH_RETRIES({
      std::string out;
      setupForDramErrorTestFromDiagShell(switchId);
      getAgentEnsemble()->runCint(dramErrorCint, out, switchId);
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      auto dramQurantinedBufferCount =
          switchStats.fb303GlobalStats()->dram_quarantined_buffer_count();
      ASSERT_EVENTUALLY_TRUE(dramQurantinedBufferCount.has_value());
      EXPECT_EVENTUALLY_GT(dramQurantinedBufferCount.value(), 0);
      XLOG(DBG0) << "Dram Quarantined Buffer count: "
                 << dramQurantinedBufferCount.value();
    });
    WITH_RETRIES({
      // Make sure that we remove the deleted buffer file created
      // so that it wont interfere with the next test run.
      EXPECT_EVENTUALLY_TRUE(
          std::filesystem::remove("/tmp/dram_quarantine_deleted_buffers_file"));
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchTest, verifyDramDataPathPacketErrorCounter) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [&]() { addRemoveNeighbor(kPortDesc, NeighborOp::ADD); };
  auto verify = [&]() {
    // The DRAM error will only be for the switchID on which packet
    // is egressing, hence the test is limited to a single switchId.
    SwitchID switchId = scopeResolver().scope(kPortDesc.phyPortID()).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    std::string out;
    WITH_RETRIES({
      setupForDramErrorTestFromDiagShell(switchId);
      getAgentEnsemble()->runDiagCommand(
          "m DDP_ERR_INITIATE INITIATE_PKT_ERR=1\n", out, switchId);
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
      sendPacket(
          ecmpHelper.ip(kPortDesc), std::nullopt, std::vector<uint8_t>(4000));
      getSw()->updateStats();
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      ASSERT_EVENTUALLY_TRUE(
          switchStats.switchDropStats()->dramDataPathPacketError().has_value());
      EXPECT_EVENTUALLY_GT(
          switchStats.switchDropStats()->dramDataPathPacketError().value(), 0);
      XLOG(DBG0)
          << "Dram Data Path Packet Error count: "
          << switchStats.switchDropStats()->dramDataPathPacketError().value();
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, verifyEgressCoreAndSramWatermark) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      getProgrammedState(), getSw()->needL2EntryForNeighbor());
  const auto kPortDesc = ecmpHelper.ecmpPortDescriptorAt(0);

  auto setup = [this, kPortDesc, ecmpHelper]() {
    addRemoveNeighbor(kPortDesc, NeighborOp::ADD);
  };

  auto verify = [this, kPortDesc, ecmpHelper]() {
    std::string kEgressCoreWm{"buffer_watermark_egress_core.p100.60"};
    std::string kSramMinWm{"buffer_watermark_min_sram.p0.60"};
    // Get SRAM size per core as thats the highest possible free SRAM
    const uint64_t kSramSize =
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getSramSizeBytes() /
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores();
    auto regex = kEgressCoreWm + "|" + kSramMinWm;
    sendPacket(
        ecmpHelper.ip(kPortDesc),
        ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    WITH_RETRIES({
      auto counters = getAgentEnsemble()->getFb303CountersByRegex(
          ecmpHelper.ecmpPortDescriptorAt(1).phyPortID(), regex);
      ASSERT_EVENTUALLY_EQ(counters.size(), 2);
      for (const auto& ctr : counters) {
        XLOG(DBG0) << ctr.first << " : " << ctr.second;
        if (ctr.first.find(kEgressCoreWm) != std::string::npos) {
          EXPECT_EVENTUALLY_GT(ctr.second, 0);
        } else {
          EXPECT_EVENTUALLY_LE(ctr.second, kSramSize);
        }
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, verifySendPacketOutOfEventorBlocked) {
  for (const auto& portId :
       masterLogicalPortIds({cfg::PortType::EVENTOR_PORT})) {
    auto beforeOutPkts = *getLatestPortStats(portId).outUnicastPkts_();
    sendLocalServiceDiscoveryMulticastPacket(portId, 1);
    // NOLINTNEXTLINE(facebook-hte-BadCall-sleep)
    sleep(5);
    EXPECT_EQ(beforeOutPkts, *getLatestPortStats(portId).outUnicastPkts_());
  }
}

TEST_F(AgentVoqSwitchTest, verifyAI23ModeConfig) {
  auto setup = []() {};
  auto verify = [this]() {
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      // Get the SDK major version
      auto sdkMajorVersionStr = getSdkMajorVersion(switchId);
      auto sdkMajorVersionNum = std::stoi(sdkMajorVersionStr);
      // Check if SDK major version is valid
      EXPECT_GT(sdkMajorVersionNum, 0);

      // If major version is >= 12, check for AI23_mode
      if (sdkMajorVersionNum >= 12) {
        std::string configOutput;
        getAgentEnsemble()->runDiagCommand(
            "config show\n", configOutput, switchId);

        // Check if AI23_mode is enabled in the config
        bool isAI23ModeEnabled =
            configOutput.find("AI23_mode=1") != std::string::npos;
        XLOG(DBG2) << "Switch ID: " << switchId << ", AI23_mode enabled? "
                   << (isAI23ModeEnabled ? "yes" : "no");

        EXPECT_TRUE(isAI23ModeEnabled)
            << "AI23_mode=1 is not enabled for SAI major version "
            << sdkMajorVersionNum << " on switch ID " << switchId;
      } else {
        XLOG(DBG2)
            << "Skipping AI23_mode config verification for SAI major version "
            << sdkMajorVersionNum << " on switch ID " << switchId;
      }
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
