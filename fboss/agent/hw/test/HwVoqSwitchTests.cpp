// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestFabricUtils.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/hw/test/HwVoqUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

namespace {
constexpr uint8_t kDefaultQueue = 0;
constexpr auto kSystemPortCountPerNode = 15;
} // namespace

namespace facebook::fboss {
class HwVoqSwitchTest : public HwLinkStateDependentTest {
  using pktReceivedCb = folly::Function<void(RxPacket* pkt) const>;

 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);
    const auto& cpuStreamTypes =
        getAsic()->getQueueStreamTypes(cfg::PortType::CPU_PORT);
    for (const auto& cpuStreamType : cpuStreamTypes) {
      if (getAsic()->getDefaultNumPortQueues(cpuStreamType, true)) {
        // cpu queues supported
        addCpuTrafficPolicy(cfg);
        utility::addCpuQueueConfig(cfg, getAsic());
        break;
      }
    }
    return cfg;
  }
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::VOQ);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }

 private:
  void addCpuTrafficPolicy(cfg::SwitchConfig& cfg) const {
    cfg::CPUTrafficPolicyConfig cpuConfig;
    std::vector<cfg::PacketRxReasonToQueue> rxReasonToQueues;
    std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
        rxReasonToQueueMappings = {
            std::pair(
                cfg::PacketRxReason::BGP,
                utility::getCoppHighPriQueueId(this->getAsic())),
            std::pair(
                cfg::PacketRxReason::BGPV6,
                utility::getCoppHighPriQueueId(this->getAsic())),
            std::pair(
                cfg::PacketRxReason::CPU_IS_NHOP, utility::kCoppMidPriQueueId),

        };
    for (auto rxEntry : rxReasonToQueueMappings) {
      auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
      rxReasonToQueue.rxReason() = rxEntry.first;
      rxReasonToQueue.queueId() = rxEntry.second;
      rxReasonToQueues.push_back(rxReasonToQueue);
    }
    cpuConfig.rxReasonToQueueOrderedList() = rxReasonToQueues;
    cfg.cpuTrafficPolicy() = cpuConfig;
  }

 protected:
  std::string kDscpAclName() const {
    return "dscp_acl";
  }
  std::string kDscpAclCounterName() const {
    return "dscp_acl_counter";
  }
  std::vector<cfg::CounterType> kCounterTypes() const {
    // At times, it is non-trivial for SAI implementations to support enabling
    // bytes counters only or packet counters only. In such cases, SAI
    // implementations enable bytes as well as packet counters even if only one
    // of the two is enabled. FBOSS use case does not require enabling only
    // one, but always enables both packets and bytes counters. Thus, enable
    // both in the test.
    // Reference: CS00012271364
    return {cfg::CounterType::BYTES, cfg::CounterType::PACKETS};
  }
  void addDscpAclWithCounter() {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, kDscpAclName());
    acl->dscp() = 0x24;
    utility::addAclStat(
        &newCfg, kDscpAclName(), kDscpAclCounterName(), kCounterTypes());
    applyNewConfig(newCfg);
  }
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

  SystemPortID getSystemPortID(const PortDescriptor& port) {
    auto switchId = getHwSwitch()->getSwitchId();
    CHECK(switchId.has_value());
    auto sysPortRange = getProgrammedState()
                            ->getDsfNodes()
                            ->getNodeIf(SwitchID(*switchId))
                            ->getSystemPortRange();
    CHECK(sysPortRange.has_value());
    return SystemPortID(port.intID() + *sysPortRange->minimum());
  }

  int sendPacket(
      const folly::IPAddressV6& dstIp,
      std::optional<PortID> frontPanelPort,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>()) {
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
        0x24 << 2, // dscp
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

  void
  rxPacketToCpuHelper(uint16_t l4SrcPort, uint16_t l4DstPort, uint8_t queueId) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);

    auto verify = [this, ecmpHelper, kPort, l4SrcPort, l4DstPort, queueId]() {
      // TODO(skhare)
      // Send to only one IPv6 address for ease of debugging.
      // Once SAI implementation bugs are fixed, send to ALL interface
      // addresses.
      auto ipAddrs = *(this->initialConfig().interfaces()[0].ipAddresses());
      auto ipv6Addr =
          std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
            auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
            return ip.isV6();
          });

      auto dstIp = folly::IPAddress::createNetwork(*ipv6Addr, -1, false).first;
      folly::IPAddressV6 kSrcIp("1::1");
      const auto srcMac = folly::MacAddress("00:00:01:02:03:04");
      const auto dstMac = utility::kLocalCpuMac();

      auto createTxPacket =
          [this, srcMac, dstMac, kSrcIp, dstIp, l4SrcPort, l4DstPort]() {
            return utility::makeTCPTxPacket(
                getHwSwitch(),
                std::nullopt, // vlanID
                srcMac,
                dstMac,
                kSrcIp,
                dstIp,
                l4SrcPort,
                l4DstPort);
          };

      auto pktReceiveHandler = [createTxPacket](RxPacket* rxPacket) {
        XLOG(DBG3) << "RX Packet Dump::"
                   << folly::hexDump(
                          rxPacket->buf()->data(), rxPacket->buf()->length());

        auto txPacket = createTxPacket();
        XLOG(DBG2) << "TX Packet Length: " << txPacket->buf()->length()
                   << " RX Packet Length: " << rxPacket->buf()->length();
        EXPECT_EQ(txPacket->buf()->length(), rxPacket->buf()->length());
        EXPECT_EQ(
            0,
            memcmp(
                txPacket->buf()->data(),
                rxPacket->buf()->data(),
                rxPacket->buf()->length()));
      };

      registerPktReceivedCallback(pktReceiveHandler);

      auto [beforeQueueOutPkts, beforeQueueOutBytes] =
          utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), queueId);

      auto txPacket = createTxPacket();
      size_t txPacketSize = txPacket->buf()->length();
      XLOG(DBG3) << "TX Packet Dump::"
                 << folly::hexDump(
                        txPacket->buf()->data(), txPacket->buf()->length());

      const PortID port = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), port);

      WITH_RETRIES({
        auto [afterQueueOutPkts, afterQueueOutBytes] =
            utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), queueId);

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

        for (auto i = 0; i < utility::getCoppHighPriQueueId(this->getAsic());
             i++) {
          auto [outPkts, outBytes] =
              utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), i);
          XLOG(DBG2) << "QueueID: " << i << " outPkts: " << outPkts
                     << " outBytes: " << outBytes;
        }
      });

      unRegisterPktReceivedCallback();
    };

    verifyAcrossWarmBoots([] {}, verify);
  }

  void packetReceived(RxPacket* pkt) noexcept override {
    auto receivedCallback = pktReceivedCallback_.lock();
    if (*receivedCallback) {
      (*receivedCallback)(pkt);
    }
  }

  void registerPktReceivedCallback(pktReceivedCb callback) {
    *pktReceivedCallback_.lock() = std::move(callback);
  }

  void unRegisterPktReceivedCallback() {
    *pktReceivedCallback_.lock() = nullptr;
  }

 private:
  folly::Synchronized<pktReceivedCb, std::mutex> pktReceivedCallback_;
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

class HwVoqSwitchWithFabricPortsTest : public HwVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true, /*interfaceHasSubnet*/
        true, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/
    );
    populatePortExpectedNeighbors(masterLogicalPortIds(), cfg);
    return cfg;
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(HwVoqSwitchWithFabricPortsTest, init) {
  auto setup = [this]() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        if (port.second->isEnabled()) {
          EXPECT_EQ(
              port.second->getLoopbackMode(),
              getAsic()->getDesiredLoopbackMode(port.second->getPortType()));
        }
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, checkFabricReachability) {
  auto verify = [this]() { checkFabricReachability(getHwSwitch()); };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, fabricIsolate) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };

  auto verify = [=]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    checkPortFabricReachability(getHwSwitch(), fabricPortId);
    auto newState = getProgrammedState();
    auto port = newState->getPorts()->getNodeIf(fabricPortId);
    auto newPort = port->modify(&newState);
    newPort->setPortDrainState(cfg::PortDrainState::DRAINED);
    applyNewState(newState);
    getHwSwitch()->updateStats(&dummy);
    checkPortFabricReachability(getHwSwitch(), fabricPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, switchIsolate) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
  };

  auto verify = [=]() { checkFabricReachability(getHwSwitch()); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, checkFabricPortSprayWithIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort, ecmpHelper]() {
    setForceTrafficOverFabric(getHwSwitch(), true);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();

    // Drain a fabric port
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    auto newState = getProgrammedState();
    auto port = newState->getPorts()->getNodeIf(fabricPortId);
    auto newPort = port->modify(&newState);
    newPort->setPortDrainState(cfg::PortDrainState::DRAINED);
    applyNewState(newState);

    // Send 10K packets and spray on fabric ports
    for (auto i = 0; i < 10000; ++i) {
      sendPacket(
          ecmpHelper.ip(kPort), ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 10000);
      auto nifBytes = getLatestPortStats(kPort.phyPortID()).get_outBytes_();
      auto fabricPortStats = getLatestPortStats(masterLogicalFabricPortIds());
      auto fabricBytes = 0;
      for (const auto& idAndStats : fabricPortStats) {
        fabricBytes += idAndStats.second.get_outBytes_();
      }
      XLOG(DBG2) << "NIF bytes: " << nifBytes
                 << " Fabric bytes: " << fabricBytes;
      EXPECT_EVENTUALLY_GE(fabricBytes, nifBytes);
      // Confirm load balance fails as the drained fabric port
      // should see close to 0 packets. We may see some control packtes.
      EXPECT_FALSE(utility::isLoadBalanced(fabricPortStats, 15));

      // Confirm traffic is load balanced across all UNDRAINED fabric ports
      fabricPortStats.erase(fabricPortId);
      EXPECT_TRUE(utility::isLoadBalanced(fabricPortStats, 15));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, checkFabricPortSpray) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort, ecmpHelper]() {
    setForceTrafficOverFabric(getHwSwitch(), true);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
    for (auto i = 0; i < 10000; ++i) {
      sendPacket(
          ecmpHelper.ip(kPort), ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 10000);
      auto nifBytes = getLatestPortStats(kPort.phyPortID()).get_outBytes_();
      auto fabricPortStats = getLatestPortStats(masterLogicalFabricPortIds());
      auto fabricBytes = 0;
      for (const auto& idAndStats : fabricPortStats) {
        fabricBytes += idAndStats.second.get_outBytes_();
      }
      XLOG(DBG2) << "NIF bytes: " << nifBytes
                 << " Fabric bytes: " << fabricBytes;
      EXPECT_EVENTUALLY_GE(fabricBytes, nifBytes);
      EXPECT_TRUE(utility::isLoadBalanced(fabricPortStats, 15));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, addRemoveNeighbor) {
  auto setup = [this]() {
    const PortDescriptor kPort(
        masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    // Add neighbor
    addRemoveNeighbor(kPort, true);
    // Remove neighbor
    addRemoveNeighbor(kPort, false);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, sendPacketCpuAndFrontPanel) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);

  auto setup = [this, kPort, ecmpHelper]() {
    addDscpAclWithCounter();
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, ecmpHelper]() {
    auto sendPacketCpuFrontPanelHelper = [this, kPort, ecmpHelper](
                                             bool isFrontPanel) {
      auto getPortOutPktsBytes = [kPort, this]() {
        return std::make_pair(
            getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_(),
            getLatestPortStats(kPort.phyPortID()).get_outBytes_());
      };

      auto getQueueOutPktsBytes = [kPort, this]() {
        return std::make_pair(
            getLatestPortStats(kPort.phyPortID())
                .get_queueOutPackets_()
                .at(kDefaultQueue),
            getLatestPortStats(kPort.phyPortID())
                .get_queueOutBytes_()
                .at(kDefaultQueue));
      };
      auto getVoQOutBytes = [kPort, this]() {
        if (!getAsic()->isSupported(HwAsic::Feature::VOQ)) {
          return 0L;
        }
        return getLatestSysPortStats(getSystemPortID(kPort))
            .get_queueOutBytes_()
            .at(kDefaultQueue);
      };

      auto getAclPackets = [this]() {
        return utility::getAclInOutPackets(
            getHwSwitch(),
            getProgrammedState(),
            kDscpAclName(),
            kDscpAclCounterName());
      };

      int64_t beforeQueueOutPkts = 0, beforeQueueOutBytes = 0;
      int64_t afterQueueOutPkts = 0, afterQueueOutBytes = 0;

      auto beforeVoQOutBytes = getVoQOutBytes();
      if (getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
        auto beforeQueueOut = getQueueOutPktsBytes();
        beforeQueueOutPkts = beforeQueueOut.first;
        beforeQueueOutBytes = beforeQueueOut.second;
      }

      auto [beforeOutPkts, beforeOutBytes] = getPortOutPktsBytes();
      auto beforeAclPkts = getAclPackets();
      std::optional<PortID> frontPanelPort;
      if (isFrontPanel) {
        frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
      }
      auto txPacketSize = sendPacket(ecmpHelper.ip(kPort), frontPanelPort);

      auto [maxRetryCount, sleepTimeMsecs] =
          utility::getRetryCountAndDelay(getAsic());
      WITH_RETRIES_N_TIMED(
          maxRetryCount, std::chrono::milliseconds(sleepTimeMsecs), {
            auto afterVoQOutBytes = getVoQOutBytes();
            if (getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
              std::tie(afterQueueOutPkts, afterQueueOutBytes) =
                  getQueueOutPktsBytes();
            }
            auto afterAclPkts = getAclPackets();
            auto portOutPktsAndBytes = getPortOutPktsBytes();
            auto afterOutPkts = portOutPktsAndBytes.first;
            auto afterOutBytes = portOutPktsAndBytes.second;

            XLOG(DBG2) << "Verifying: "
                       << (isFrontPanel ? "Send Packet from Front Panel Port"
                                        : "Send Packet from CPU Port")
                       << " Stats:: beforeOutPkts: " << beforeOutPkts
                       << " beforeOutBytes: " << beforeOutBytes
                       << " beforeQueueOutPkts: " << beforeQueueOutPkts
                       << " beforeQueueOutBytes: " << beforeQueueOutBytes
                       << " beforeVoQOutBytes: " << beforeVoQOutBytes
                       << " beforeAclPkts: " << beforeAclPkts
                       << " txPacketSize: " << txPacketSize
                       << " afterOutPkts: " << afterOutPkts
                       << " afterOutBytes: " << afterOutBytes
                       << " afterQueueOutPkts: " << afterQueueOutPkts
                       << " afterQueueOutBytes: " << afterQueueOutBytes
                       << " afterVoQOutBytes: " << afterVoQOutBytes
                       << " afterAclPkts: " << afterAclPkts;

            EXPECT_EVENTUALLY_EQ(afterOutPkts - 1, beforeOutPkts);
            int extraByteOffset = 0;
            if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
              // CS00012267635: debug why we get 4 extra bytes
              // CS00012299306 why we don't get extra 4 bytes for J3
              extraByteOffset = 4;
            }
            EXPECT_EVENTUALLY_EQ(
                afterOutBytes - txPacketSize - extraByteOffset, beforeOutBytes);
            if (getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
              EXPECT_EVENTUALLY_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
              // CS00012267635: debug why queue counter is 310, when
              // txPacketSize is 322
              EXPECT_EVENTUALLY_GE(afterQueueOutBytes, beforeQueueOutBytes);
            }
            EXPECT_EVENTUALLY_GT(afterAclPkts, beforeAclPkts);
            if (getAsic()->isSupported(HwAsic::Feature::VOQ)) {
              EXPECT_EVENTUALLY_GT(afterVoQOutBytes, beforeVoQOutBytes);
            }
          });
    };

    sendPacketCpuFrontPanelHelper(false /* cpu */);
    sendPacketCpuFrontPanelHelper(true /* front panel*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, trapPktsOnPort) {
  auto verify = [this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    auto ensemble = getHwSwitchEnsemble();
    auto snooper = std::make_unique<HwTestPacketSnooper>(ensemble);
    auto entry = std::make_unique<HwTestPacketTrapEntry>(
        ensemble->getHwSwitch(), kPort.phyPortID());
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    sendPacket(ecmpHelper.ip(kPort), frontPanelPort);
    WITH_RETRIES({
      auto frameRx = snooper->waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwVoqSwitchTest, rxPacketToCpu) {
  rxPacketToCpuHelper(
      utility::kNonSpecialPort1,
      utility::kNonSpecialPort2,
      utility::kCoppMidPriQueueId);
}

TEST_F(HwVoqSwitchTest, rxPacketToCpuBgpDstPort) {
  rxPacketToCpuHelper(
      utility::kNonSpecialPort1,
      utility::kBgpPort,
      utility::getCoppHighPriQueueId(this->getAsic()));
}

TEST_F(HwVoqSwitchTest, rxPacketToCpuBgpSrcPort) {
  rxPacketToCpuHelper(
      utility::kBgpPort,
      utility::kNonSpecialPort1,
      utility::getCoppHighPriQueueId(this->getAsic()));
}

TEST_F(HwVoqSwitchTest, AclQualifiersWithCounter) {
  auto kAclName = "acl1";
  auto kAclCounterName = "aclCounter1";

  auto setup = [kAclName, kAclCounterName, this]() {
    auto newCfg = initialConfig();

    auto* acl = utility::addAcl(&newCfg, kAclName);
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    acl->srcIp() = "::ffff:c0a8:1"; // fails: CS00012270649
    acl->dstIp() = "2401:db00:3020:70e2:face:0:63:0/64"; // fails: CS00012270650
    acl->srcPort() = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
    acl->dscp() = 0x24;

    utility::addAclStat(&newCfg, kAclName, kAclCounterName, kCounterTypes());

    applyNewConfig(newCfg);
  };

  auto verify = [kAclName, kAclCounterName, this]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), kAclName);

    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 2);
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {kAclName},
        kAclCounterName,
        kCounterTypes());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, voqDelete) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  auto port = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [=]() {
    addRemoveNeighbor(port, true /*add*/);
    // Disable port TX
    utility::setPortTx(getHwSwitch(), port.phyPortID(), false);
  };
  auto verify = [=]() {
    auto getVoQDeletedPkts = [port, this]() {
      if (!getAsic()->isSupported(HwAsic::Feature::VOQ_DELETE_COUNTER)) {
        return 0L;
      }
      return getLatestSysPortStats(getSystemPortID(port))
          .get_queueCreditWatchdogDeletedPackets_()
          .at(kDefaultQueue);
    };

    auto voqDeletedPktsBefore = getVoQDeletedPkts();
    const auto dstIp = ecmpHelper.ip(port);
    for (auto i = 0; i < 100; ++i) {
      // Send pkts via front panel
      sendPacket(dstIp, ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
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

TEST_F(HwVoqSwitchTest, packetIntegrityError) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  auto port = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [=]() { addRemoveNeighbor(port, true /*add*/); };
  auto verify = [=]() {
    const auto dstIp = ecmpHelper.ip(port);
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        "m SPB_FORCE_CRC_ERROR FORCE_CRC_ERROR_ON_DATA=1 FORCE_CRC_ERROR_ON_CRC=1\n",
        out);
    sendPacket(dstIp, std::nullopt);
    WITH_RETRIES({
      SwitchStats dummy;
      getHwSwitch()->updateStats(&dummy);
      auto pktIntegrityDrops =
          getHwSwitch()->getSwitchStats()->getPacketIntegrityDropsCount();
      XLOG(INFO) << " Packet integrity drops: " << pktIntegrityDrops;
      EXPECT_EVENTUALLY_GT(pktIntegrityDrops, 0);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, localForwardingPostIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort]() {
    auto newCfg = initialConfig();
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, &ecmpHelper]() {
    auto sendPktAndVerify = [&](std::optional<PortID> portToSendFrom) {
      auto beforePkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      sendPacket(ecmpHelper.ip(kPort), portToSendFrom);
      WITH_RETRIES({
        auto afterPkts =
            getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
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

TEST_F(HwVoqSwitchTest, stressLocalForwardingPostIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort]() {
    auto newCfg = initialConfig();
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, &ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
    for (auto i = 0; i < 10000; ++i) {
      // CPU send
      sendPacket(ecmpHelper.ip(kPort), std::nullopt);
      // Front panel send
      sendPacket(
          ecmpHelper.ip(kPort), ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 20000);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, localSystemPortEcmp) {
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
    applyNewState(
        ecmpHelper.resolveNextHops(getProgrammedState(), localSysPorts));
    ecmpHelper.programRoutes(getRouteUpdater(), localSysPorts, {prefix});
  };
  verifyAcrossWarmBoots(setup, [] {});
}

class HwVoqSwitchWithMultipleDsfNodesTest : public HwVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = HwVoqSwitchTest::initialConfig();
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    return cfg;
  }

  static SwitchID getRemoteSwitchId() {
    return SwitchID(4);
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const override {
    CHECK(!curDsfNodes.empty());
    auto dsfNodes = curDsfNodes;
    const auto& firstDsfNode = dsfNodes.begin()->second;
    CHECK(firstDsfNode.systemPortRange().has_value());
    CHECK(firstDsfNode.nodeMac().has_value());
    folly::MacAddress mac(*firstDsfNode.nodeMac());
    auto asic = HwAsic::makeAsic(
        *firstDsfNode.asicType(),
        cfg::SwitchType::VOQ,
        *firstDsfNode.switchId(),
        *firstDsfNode.systemPortRange(),
        mac);
    auto otherDsfNodeCfg = utility::dsfNodeConfig(*asic, getRemoteSwitchId());
    dsfNodes.insert({*otherDsfNodeCfg.switchId(), otherDsfNodeCfg});
    return dsfNodes;
  }

 protected:
  void assertVoqTailDrops(
      const folly::IPAddressV6& nbrIp,
      const SystemPortID& sysPortId) {
    auto sendPkts = [=]() {
      for (auto i = 0; i < 1000; ++i) {
        sendPacket(nbrIp, std::nullopt);
      }
    };
    auto voqDiscardBytes = 0;
    WITH_RETRIES_N(100, {
      sendPkts();
      SwitchStats dummy;
      getHwSwitch()->updateStats(&dummy);
      voqDiscardBytes =
          getLatestSysPortStats(sysPortId).get_queueOutDiscardBytes_().at(
              kDefaultQueue);
      XLOG(INFO) << " VOQ discard bytes: " << voqDiscardBytes;
      EXPECT_EVENTUALLY_GT(voqDiscardBytes, 0);
    });
  }
};

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, twoDsfNodes) {
  verifyAcrossWarmBoots([] {}, [] {});
}

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, remoteSystemPort) {
  auto setup = [this]() {
    applyNewState(utility::addRemoteSysPort(
        getProgrammedState(), scopeResolver(), SystemPortID(401)));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, remoteRouterInterface) {
  auto setup = [this]() {
    auto constexpr remotePortId = 401;
    applyNewState(utility::addRemoteSysPort(
        getProgrammedState(), scopeResolver(), SystemPortID(remotePortId)));
    applyNewState(utility::addRemoteInterface(
        getProgrammedState(),
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
        }));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, addRemoveRemoteNeighbor) {
  auto setup = [this]() {
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    applyNewState(utility::addRemoteSysPort(
        getProgrammedState(), scopeResolver(), kRemoteSysPortId));
    const InterfaceID kIntfId(remotePortId);
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
    folly::IPAddressV6 kNeighborIp("100::2");
    uint64_t dummyEncapIndex = 401;
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState(utility::addRemoveRemoteNeighbor(
        getProgrammedState(),
        scopeResolver(),
        kNeighborIp,
        kIntfId,
        kPort,
        true,
        dummyEncapIndex));
    // Remove neighbor
    applyNewState(utility::addRemoveRemoteNeighbor(
        getProgrammedState(),
        scopeResolver(),
        kNeighborIp,
        kIntfId,
        kPort,
        false,
        dummyEncapIndex));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, stressAddRemoveObjects) {
  auto setup = [=]() {
    // Disable credit watchdog
    utility::enableCreditWatchdog(getHwSwitch(), false);
  };
  auto verify = [this]() {
    auto numIterations = 500;
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    folly::IPAddressV6 kNeighborIp("100::2");
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    for (auto i = 0; i < numIterations; ++i) {
      // add local neighbor
      addRemoveNeighbor(kPort, true /* add neighbor*/);
      // Remote objs
      applyNewState(utility::addRemoteSysPort(
          getProgrammedState(), scopeResolver(), kRemoteSysPortId));
      const InterfaceID kIntfId(remotePortId);
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
      uint64_t dummyEncapIndex = 401;
      PortDescriptor kRemotePort(kRemoteSysPortId);
      // Add neighbor
      applyNewState(utility::addRemoveRemoteNeighbor(
          getProgrammedState(),
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kRemotePort,
          true,
          dummyEncapIndex));
      // Delete on all but the last iteration. In the last iteration
      // we will leave the entries intact and then forward pkts
      // to this VOQ
      if (i < numIterations - 1) {
        addRemoveNeighbor(kPort, false /* remove neighbor*/);
        // Remove neighbor
        applyNewState(utility::addRemoveRemoteNeighbor(
            getProgrammedState(),
            scopeResolver(),
            kNeighborIp,
            kIntfId,
            kRemotePort,
            false,
            dummyEncapIndex));
        // Remove rif
        applyNewState(
            utility::removeRemoteInterface(getProgrammedState(), kIntfId));
        // Remove sys port
        applyNewState(utility::removeRemoteSysPort(
            getProgrammedState(), kRemoteSysPortId));
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
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, voqTailDropCounter) {
  folly::IPAddressV6 kNeighborIp("100::2");
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  auto setup = [=]() {
    // Disable credit watchdog
    utility::enableCreditWatchdog(getHwSwitch(), false);
    applyNewState(utility::addRemoteSysPort(
        getProgrammedState(), scopeResolver(), kRemoteSysPortId));
    const InterfaceID kIntfId(remotePortId);
    applyNewState(utility::addRemoteInterface(
        getProgrammedState(),
        scopeResolver(),
        kIntfId,
        {
            {folly::IPAddress("100::1"), 64},
            {folly::IPAddress("100.0.0.1"), 24},
        }));
    uint64_t dummyEncapIndex = 401;
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState(utility::addRemoveRemoteNeighbor(
        getProgrammedState(),
        scopeResolver(),
        kNeighborIp,
        kIntfId,
        kPort,
        true,
        dummyEncapIndex));
  };

  auto verify = [=]() { assertVoqTailDrops(kNeighborIp, kRemoteSysPortId); };
  verifyAcrossWarmBoots(setup, verify);
};

// FullScaleDsfNode Test sets up 128 remote DSF nodes for J2 and 256 for J3.
class HwVoqSwitchFullScaleDsfNodesTest
    : public HwVoqSwitchWithMultipleDsfNodesTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = HwVoqSwitchTest::initialConfig();
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    return cfg;
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const override {
    CHECK(!curDsfNodes.empty());
    auto dsfNodes = curDsfNodes;
    const auto& firstDsfNode = dsfNodes.begin()->second;
    CHECK(firstDsfNode.systemPortRange().has_value());
    CHECK(firstDsfNode.nodeMac().has_value());
    folly::MacAddress mac(*firstDsfNode.nodeMac());
    auto asic = HwAsic::makeAsic(
        *firstDsfNode.asicType(),
        cfg::SwitchType::VOQ,
        *firstDsfNode.switchId(),
        *firstDsfNode.systemPortRange(),
        mac);
    int numCores = asic->getNumCores();
    for (int remoteSwitchId = numCores;
         remoteSwitchId < getDsfNodeCount(asic.get()) * numCores;
         remoteSwitchId += numCores) {
      // Ideally there's no need to add extra offset if each dsfNode has 15
      // ports. However, local switch already used 1 CPU, 1 recyle and 16 system
      // ports. Adding an extra offset of 5 for remote system ports.
      const auto systemPortMin =
          (remoteSwitchId / numCores) * kSystemPortCountPerNode + 5;
      cfg::Range64 systemPortRange;
      systemPortRange.minimum() = systemPortMin;
      systemPortRange.maximum() = systemPortMin + kSystemPortCountPerNode - 1;
      auto remoteDsfNodeCfg = utility::dsfNodeConfig(
          *asic, SwitchID(remoteSwitchId), systemPortMin);
      dsfNodes.insert({remoteSwitchId, remoteDsfNodeCfg});
    }
    return dsfNodes;
  }

 protected:
  int getMaxEcmpWidth(const HwAsic* asic) const {
    return asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2 ? 128 : 512;
  }

  int getMaxEcmpGroup() const {
    return 64;
  }

  // Resolve and return list of remote nhops
  std::vector<PortDescriptor> resolveRemoteNhops(
      utility::EcmpSetupTargetedPorts6& ecmpHelper) {
    auto remoteSysPorts =
        getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
    std::vector<PortDescriptor> sysPortDescs;
    std::for_each(
        remoteSysPorts->begin(),
        remoteSysPorts->end(),
        [&sysPortDescs](const auto& idAndPort) {
          sysPortDescs.push_back(
              PortDescriptor(static_cast<SystemPortID>(idAndPort.first)));
        });
    auto currState = getProgrammedState();
    for (const auto& sysPortDesc : sysPortDescs) {
      currState = ecmpHelper.resolveNextHops(
          currState,
          {sysPortDesc},
          false,
          /* encapIndex */ sysPortDesc.sysPortID());
    }
    applyNewState(currState);
    return sysPortDescs;
  }

 private:
  int getDsfNodeCount(const HwAsic* asic) const {
    return asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2 ? 128 : 256;
  }
};

TEST_F(HwVoqSwitchFullScaleDsfNodesTest, systemPortScaleTest) {
  auto setup = [this]() {
    applyNewState(utility::setupRemoteIntfAndSysPorts(
        getProgrammedState(), scopeResolver(), initialConfig()));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchFullScaleDsfNodesTest, remoteNeighborWithEcmpGroup) {
  auto kEcmpWidth = getMaxEcmpWidth(getAsic());
  FLAGS_ecmp_width = kEcmpWidth;
  auto setup = [&]() {
    applyNewState(utility::setupRemoteIntfAndSysPorts(
        getProgrammedState(), scopeResolver(), initialConfig()));
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    // Trigger config apply to add remote interface routes as directly connected
    // in RIB. This is to resolve ECMP members pointing to remote nexthops.
    applyNewConfig(initialConfig());

    // Resolve remote nhops and get a list of remote sysPort descriptors
    auto sysPortDescs = resolveRemoteNhops(ecmpHelper);

    for (int i = 0; i < getMaxEcmpGroup(); i++) {
      auto prefix = RoutePrefixV6{
          folly::IPAddressV6(folly::to<std::string>(i + 1, "::", i + 1)), 128};
      ecmpHelper.programRoutes(
          getRouteUpdater(),
          flat_set<PortDescriptor>(
              std::make_move_iterator(sysPortDescs.begin() + i),
              std::make_move_iterator(sysPortDescs.begin() + i + kEcmpWidth)),
          {prefix});
    }
  };
  // TODO: Send and verify packets across voq drops.
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchFullScaleDsfNodesTest, stressProgramEcmpRoutes) {
  auto kEcmpWidth = getMaxEcmpWidth(getAsic());
  FLAGS_ecmp_width = kEcmpWidth;
  // Stress add/delete 100 iterations of 5 routes with ECMP width.
  const auto routeScale = 5;
  const auto numIterations = 100;
  auto setup = [&]() {
    applyNewState(utility::setupRemoteIntfAndSysPorts(
        getProgrammedState(), scopeResolver(), initialConfig()));
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    // Trigger config apply to add remote interface routes as directly connected
    // in RIB. This is to resolve ECMP members pointing to remote nexthops.
    applyNewConfig(initialConfig());

    // Resolve remote nhops and get a list of remote sysPort descriptors
    auto sysPortDescs = resolveRemoteNhops(ecmpHelper);

    for (int iter = 0; iter < numIterations; iter++) {
      std::vector<RoutePrefixV6> routes;
      for (int i = 0; i < routeScale; i++) {
        auto prefix = RoutePrefixV6{
            folly::IPAddressV6(folly::to<std::string>(i + 1, "::", i + 1)),
            128};
        ecmpHelper.programRoutes(
            getRouteUpdater(),
            flat_set<PortDescriptor>(
                std::make_move_iterator(sysPortDescs.begin() + i),
                std::make_move_iterator(sysPortDescs.begin() + i + kEcmpWidth)),
            {prefix});
        routes.push_back(prefix);
      }
      ecmpHelper.unprogramRoutes(getRouteUpdater(), routes);
    }
  };
  verifyAcrossWarmBoots(setup, [] {});
}
} // namespace facebook::fboss
