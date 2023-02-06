// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/IndusAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwFabricUtils.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

namespace {
constexpr uint8_t kDefaultQueue = 0;
}

namespace facebook::fboss {
namespace {
const SwitchID kRemoteSwitchId(2);
}
class HwVoqSwitchTest : public HwLinkStateDependentTest {
  using pktReceivedCb = folly::Function<void(RxPacket* pkt) const>;

 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode(),
        true /*interfaceHasSubnet*/);
    addCpuTrafficPolicy(cfg);
    addCpuDefaultQueueConfig(cfg);
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
            std::pair(cfg::PacketRxReason::CPU_IS_NHOP, kDefaultQueue)};
    for (auto rxEntry : rxReasonToQueueMappings) {
      auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
      rxReasonToQueue.rxReason() = rxEntry.first;
      rxReasonToQueue.queueId() = rxEntry.second;
      rxReasonToQueues.push_back(rxReasonToQueue);
    }
    cpuConfig.rxReasonToQueueOrderedList() = rxReasonToQueues;
    cfg.cpuTrafficPolicy() = cpuConfig;
  }
  void addCpuDefaultQueueConfig(cfg::SwitchConfig& cfg) const {
    std::vector<cfg::PortQueue> cpuQueues;

    cfg::PortQueue queue0;
    queue0.id() = kDefaultQueue;
    queue0.name() = "cpuQueue-default";
    queue0.streamType() = utility::getCpuDefaultStreamType(this->getAsic());

    cpuQueues.push_back(queue0);
    *cfg.cpuQueues() = cpuQueues;
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
  void addRemoteSysPort(SystemPortID portId) {
    auto newState = getProgrammedState()->clone();
    auto localPort = newState->getSystemPorts()->cbegin()->second;
    auto remoteSystemPorts =
        newState->getRemoteSystemPorts()->modify(&newState);
    auto numPrevPorts = remoteSystemPorts->size();
    auto remoteSysPort = std::make_shared<SystemPort>(portId);
    remoteSysPort->setSwitchId(kRemoteSwitchId);
    remoteSysPort->setNumVoqs(localPort->getNumVoqs());
    remoteSysPort->setCoreIndex(localPort->getCoreIndex());
    remoteSysPort->setCorePortIndex(localPort->getCorePortIndex());
    remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
    remoteSysPort->setEnabled(true);
    remoteSystemPorts->addSystemPort(remoteSysPort);
    applyNewState(newState);
    EXPECT_EQ(
        getProgrammedState()->getRemoteSystemPorts()->size(), numPrevPorts + 1);
  }
  void addRemoteInterface(
      InterfaceID intfId,
      const Interface::Addresses& subnets) {
    auto newState = getProgrammedState();
    auto newRemoteInterfaces =
        newState->getRemoteInterfaces()->modify(&newState);
    auto numPrevIntfs = newRemoteInterfaces->size();
    auto newRemoteInterface = std::make_shared<Interface>(
        intfId,
        RouterID(0),
        std::optional<VlanID>(std::nullopt),
        folly::StringPiece("RemoteIntf"),
        folly::MacAddress("c6:ca:2b:2a:b1:b6"),
        9000,
        false,
        false,
        cfg::InterfaceType::SYSTEM_PORT);
    newRemoteInterface->setAddresses(subnets);
    newRemoteInterfaces->addInterface(newRemoteInterface);
    applyNewState(newState);
    EXPECT_EQ(
        getProgrammedState()->getRemoteInterfaces()->size(), numPrevIntfs + 1);
  }
  void addRemoveRemoteNeighbor(
      const folly::IPAddressV6& neighborIp,
      InterfaceID intfID,
      PortDescriptor port,
      bool add,
      std::optional<int64_t> encapIndex = std::nullopt) {
    auto outState = getProgrammedState();
    auto interfaceMap = outState->getRemoteInterfaces()->modify(&outState);
    auto interface = interfaceMap->getInterface(intfID)->clone();
    auto ndpTable = interfaceMap->getInterface(intfID)->getNdpTable()->clone();
    if (add) {
      const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
      state::NeighborEntryFields ndp;
      ndp.mac() = kNeighborMac.toString();
      ndp.ipaddress() = neighborIp.str();
      ndp.portId() = port.toThrift();
      ndp.interfaceId() = static_cast<int>(intfID);
      ndp.state() = state::NeighborState::Reachable;
      if (encapIndex) {
        ndp.encapIndex() = *encapIndex;
      }
      ndp.isLocal() = encapIndex == std::nullopt;
      ndpTable->emplace(neighborIp.str(), std::move(ndp));
    } else {
      ndpTable->remove(neighborIp.str());
    }
    interface->setNdpTable(ndpTable->toThrift());
    interfaceMap->updateNode(interface);
    applyNewState(outState);
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

  void sendPacketHelper(bool isFrontPanel, bool checkAclCounter = true) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);

    auto setup = [this, kPort, ecmpHelper, checkAclCounter]() {
      if (checkAclCounter) {
        addDscpAclWithCounter();
      }
      addRemoveNeighbor(kPort, true /* add neighbor*/);
    };

    auto verify = [this, kPort, ecmpHelper, isFrontPanel, checkAclCounter]() {
      folly::IPAddressV6 kSrcIp("1::1");
      folly::IPAddressV6 kNeighborIp = ecmpHelper.ip(kPort);
      const auto srcMac = utility::kLocalCpuMac();
      const auto dstMac = utility::kLocalCpuMac();

      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
          std::nullopt, // vlanID
          srcMac,
          dstMac,
          kSrcIp,
          kNeighborIp,
          8000, // l4 src port
          8001, // l4 dst port
          0x24 << 2); // dscp
      size_t txPacketSize = txPacket->buf()->length();

      XLOG(DBG3) << "\n"
                 << folly::hexDump(
                        txPacket->buf()->data(), txPacket->buf()->length());

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
        auto sysPortRange =
            getProgrammedState()->getSwitchSettings()->getSystemPortRange();
        CHECK(sysPortRange.has_value());
        const SystemPortID sysPortId(kPort.intID() + *sysPortRange->minimum());
        return getLatestSysPortStats(sysPortId).get_queueOutBytes_().at(
            kDefaultQueue);
      };

      auto getAclPackets = [this, checkAclCounter]() {
        return checkAclCounter ? utility::getAclInOutPackets(
                                     getHwSwitch(),
                                     getProgrammedState(),
                                     kDscpAclName(),
                                     kDscpAclCounterName())
                               : 0;
      };

      auto [beforeOutPkts, beforeOutBytes] = getPortOutPktsBytes();
      auto [beforeQueueOutPkts, beforeQueueOutBytes] = getQueueOutPktsBytes();
      auto beforeVoQOutBytes = getVoQOutBytes();
      auto beforeAclPkts = getAclPackets();

      if (isFrontPanel) {
        const PortID port = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
        getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
            std::move(txPacket), port);
      } else {
        getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
      }

      WITH_RETRIES({
        auto afterVoQOutBytes = getVoQOutBytes();
        auto afterAclPkts = getAclPackets();
        auto portOutPktsAndBytes = getPortOutPktsBytes();
        auto queueOutPktsAndBytes = getQueueOutPktsBytes();
        auto afterOutPkts = portOutPktsAndBytes.first;
        auto afterOutBytes = portOutPktsAndBytes.second;
        auto afterQueueOutPkts = queueOutPktsAndBytes.first;
        auto afterQueueOutBytes = queueOutPktsAndBytes.second;

        XLOG(DBG2) << "Stats:: beforeOutPkts: " << beforeOutPkts
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
        // CS00012267635: debug why we get 4 extra bytes
        EXPECT_EVENTUALLY_EQ(afterOutBytes - txPacketSize - 4, beforeOutBytes);
        EXPECT_EVENTUALLY_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
        // CS00012267635: debug why queue counter is 310, when txPacketSize is
        // 322
        EXPECT_EVENTUALLY_GE(afterQueueOutBytes, beforeQueueOutBytes);
        if (checkAclCounter) {
          EXPECT_EVENTUALLY_GT(afterAclPkts, beforeAclPkts);
        }
        if (getAsic()->isSupported(HwAsic::Feature::VOQ)) {
          EXPECT_EVENTUALLY_GT(afterVoQOutBytes, beforeVoQOutBytes);
        }
      });
    };

    verifyAcrossWarmBoots(setup, verify);
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
        getAsic()->desiredLoopbackMode(),
        true, /*interfaceHasSubnet*/
        false, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/
    );
    return cfg;
  }
};

TEST_F(HwVoqSwitchWithFabricPortsTest, init) {
  auto setup = [this]() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : std::as_const(*state->getPorts())) {
      if (port.second->isEnabled()) {
        EXPECT_EQ(
            port.second->getLoopbackMode(), getAsic()->desiredLoopbackMode());
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchWithFabricPortsTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->size(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwVoqSwitchTest, remoteSystemPort) {
  auto setup = [this]() { addRemoteSysPort(SystemPortID(401)); };
  verifyAcrossWarmBoots(setup, [] {});
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

TEST_F(HwVoqSwitchTest, remoteRouterInterface) {
  auto setup = [this]() {
    auto constexpr remotePortId = 401;
    addRemoteSysPort(SystemPortID(remotePortId));
    addRemoteInterface(
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
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, addRemoveRemoteNeighbor) {
  auto setup = [this]() {
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    addRemoteSysPort(kRemoteSysPortId);
    const InterfaceID kIntfId(remotePortId);
    addRemoteInterface(
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
    folly::IPAddressV6 kNeighborIp("100::2");
    uint64_t dummyEncapIndex = 401;
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    addRemoveRemoteNeighbor(kNeighborIp, kIntfId, kPort, true, dummyEncapIndex);
    // Remove neighbor
    addRemoveRemoteNeighbor(
        kNeighborIp, kIntfId, kPort, false, dummyEncapIndex);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, sendPacketCpu) {
  sendPacketHelper(false /* cpu */);
}

TEST_F(HwVoqSwitchTest, sendPacketFrontPanel) {
  sendPacketHelper(true /* front panel */);
}

TEST_F(HwVoqSwitchTest, trapPktsOnPort) {
  auto verify = [this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    auto ensemble = getHwSwitchEnsemble();
    auto snooper = std::make_unique<HwTestPacketSnooper>(ensemble);
    auto entry = std::make_unique<HwTestPacketTrapEntry>(
        ensemble->getHwSwitch(), kPort.phyPortID());
    sendPacketHelper(true /* front panel */, false /*checkAclCounter*/);
    WITH_RETRIES({
      auto frameRx = snooper->waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
  };
  verifyAcrossWarmBoots([] {}, verify);
}
TEST_F(HwVoqSwitchTest, rxPacketToCpu) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);

  auto verify = [this, ecmpHelper, kPort]() {
    // TODO(skhare)
    // Send to only one IPv6 address for ease of debugging.
    // Once SAI implementation bugs are fixed, send to ALL interface addresses.
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

    auto createTxPacket = [this, srcMac, dstMac, kSrcIp, dstIp]() {
      return utility::makeUDPTxPacket(
          getHwSwitch(),
          std::nullopt, // vlanID
          srcMac,
          dstMac,
          kSrcIp,
          dstIp,
          8000, // l4 src port
          8001); // l4 dst port
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
        utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), kDefaultQueue);

    auto txPacket = createTxPacket();
    size_t txPacketSize = txPacket->buf()->length();
    XLOG(DBG3) << "TX Packet Dump::"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());

    const PortID port = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(txPacket), port);

    auto [afterQueueOutPkts, afterQueueOutBytes] =
        utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), kDefaultQueue);

    XLOG(DBG2) << "Stats:: beforeQueueOutPkts: " << beforeQueueOutPkts
               << " beforeQueueOutBytes: " << beforeQueueOutBytes
               << " txPacketSize: " << txPacketSize
               << " afterQueueOutPkts: " << afterQueueOutPkts
               << " afterQueueOutBytes: " << afterQueueOutBytes;

    EXPECT_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
    // CS00012267635: debug why queue counter is 362, when txPacketSize is 322
    EXPECT_GE(afterQueueOutBytes, beforeQueueOutBytes);

    unRegisterPktReceivedCallback();
  };

  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwVoqSwitchTest, AclQualifiers) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto* acl = utility::addAcl(&newCfg, "acl1", cfg::AclActionType::DENY);
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    acl->srcIp() = "::ffff:c0a8:1"; // fails: CS00012270649
    acl->dstIp() = "2401:db00:3020:70e2:face:0:63:0/64"; // fails: CS00012270650
    acl->srcPort() = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
    acl->dscp() = 0x24;
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch()));
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, AclCounter) {
  auto setup = [=]() { addDscpAclWithCounter(); };

  auto verify = [=]() {
    // Needs CS00012270647 (support SAI_ACL_COUNTER_ATTR_LABEL) to PASS
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 2);
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {kDscpAclName()},
        kDscpAclCounterName(),
        kCounterTypes());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, checkFabricReachability) {
  verifyAcrossWarmBoots(
      [] {}, [this]() { checkFabricReachability(getHwSwitch()); });
}

class HwVoqSwitchWithMultipleDsfNodesTest : public HwVoqSwitchTest {
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
    auto asic = HwAsic::makeAsic(
        *firstDsfNode.asicType(),
        cfg::SwitchType::VOQ,
        *firstDsfNode.switchId(),
        *firstDsfNode.systemPortRange());
    auto otherDsfNodeCfg = utility::dsfNodeConfig(*asic);
    dsfNodes.insert({*otherDsfNodeCfg.switchId(), otherDsfNodeCfg});
    return dsfNodes;
  }
};

TEST_F(HwVoqSwitchWithMultipleDsfNodesTest, twoDsfNodes) {
  verifyAcrossWarmBoots([] {}, [] {});
}

} // namespace facebook::fboss
