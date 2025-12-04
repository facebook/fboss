// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/prod_agent_tests/ProdInvariantTests.h"
#include <folly/gen/Base.h>
#include <chrono>
#include <thread>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/validated_shell_commands_constants.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/ProdConfigFactory.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/DscpMarkingUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace {
using facebook::fboss::PortDescriptor;
using facebook::fboss::PortID;
auto constexpr kEcmpWidth = 4;
auto constexpr kLbPacketCount = 200000;
auto constexpr kLbDeviation = 25;
} // namespace

namespace facebook::fboss {

void ProdInvariantTest::setupAgentTestEcmp(
    const std::vector<PortDescriptor>& ecmpPorts) {
  ASSERT_GT(ecmpPorts.size(), 0);

  boost::container::flat_set<PortDescriptor> ports;
  std::for_each(ecmpPorts.begin(), ecmpPorts.end(), [&ports](auto ecmpPort) {
    ports.insert(ecmpPort);
  });

  // When prod config is used, use uplink's subnet IP with last bit flipped as
  // the next hop IP.
  auto forProdConfig =
      useProdConfig_.has_value() ? useProdConfig_.value() : false;
  utility::EcmpSetupTargetedPorts6 ecmp6(
      getSw()->getState(),
      getSw()->needL2EntryForNeighbor(),
      forProdConfig,
      {cfg::PortType::INTERFACE_PORT});

  getSw()->updateStateBlocking("Resolve v6 nhops", [&](auto state) {
    return ecmp6.resolveNextHops(state, ports);
  });

  ecmp6.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(getSw()->getRouteUpdater()),
      ports);
}

void ProdInvariantTest::SetUp() {
  AgentEnsembleTest::SetUp();
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto ecmpUplinlinkPorts = utility::getAllUplinkDownlinkPorts(
                                getSw()->getPlatformType(),
                                initialConfig(*ensemble),
                                kEcmpWidth,
                                is_mmu_lossless_mode())
                                .first;
  for (auto& uplinkPort : ecmpUplinlinkPorts) {
    ecmpPorts_.emplace_back(uplinkPort);
  }

  if (ecmpPorts_.size() >= kEcmpWidth) {
    setupAgentTestEcmp(ecmpPorts_);
  }
  XLOG(DBG2) << "ProdInvariantTest setup done";
}

std::vector<PortID> ProdInvariantTest::getAllPlatformPorts(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts) {
  std::vector<PortID> ports;
  ports.reserve(0);
  auto subsidiaryPortMap = utility::getSubsidiaryPortIDs(platformPorts);
  for (auto& port : subsidiaryPortMap) {
    ports.emplace_back(port.first);
  }
  return ports;
}
bool ProdInvariantTest::checkBaseConfigPortsEmpty() {
  const auto baseSwitchConfig = getConfigFromFlag();
  return baseSwitchConfig.ports()->empty();
}

cfg::SwitchConfig ProdInvariantTest::getConfigFromFlag() {
  auto agentConfig = AgentConfig::fromFile(FLAGS_config);
  auto config = *agentConfig->thrift.sw();

  // If we're passed a config, there's a high probability that it's a prod
  // config and the ports are not in loopback mode.
  for (auto& port : *config.ports()) {
    port.loopbackMode() = cfg::PortLoopbackMode::MAC;
  }
  for (auto& intf : *config.interfaces()) {
    if (intf.ndp()) {
      intf.ndp()->routerAdvertisementSeconds() = 0;
    }
  }
  return config;
}

cfg::SwitchConfig ProdInvariantTest::initialConfig(
    const AgentEnsemble& ensemble) {
  if (useProdConfig_.has_value()) {
    return *(ensemble.getSw()->getAgentConfig().sw());
  }
  cfg::SwitchConfig cfg;
  std::vector<PortID> ports;
  ports.reserve(0);
  if (checkBaseConfigPortsEmpty()) {
    useProdConfig_ = false;
    ports = getAllPlatformPorts(ensemble.getPlatformPorts());
    cfg = utility::createProdRswConfig(
        ensemble.getL3Asics(),
        ensemble.getSw()->getPlatformType(),
        ensemble.getSw()->getPlatformMapping(),
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        ports,
        ensemble.isSai());

    return cfg;
  } else {
    useProdConfig_ = true;
    return getConfigFromFlag();
  }
}

void ProdInvariantTest::setupConfigFlag() {
  AgentEnsemble* ensemble = getAgentEnsemble();
  cfg::AgentConfig testConfig;
  auto hwAsics = ensemble->getSw()->getHwAsicTable()->getL3Asics();
  auto asic = checkSameAndGetAsic(hwAsics);
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<MultiSwitchPortMap>(),
      ensemble->getPlatformMapping(),
      asic,
      ensemble->supportsAddRemovePort());
  testConfig.sw() = initialConfig(*ensemble);
  const auto& baseConfig = ensemble->getSw()->getAgentConfig();
  testConfig.defaultCommandLineArgs() = *baseConfig.defaultCommandLineArgs();
  testConfig.platform() = *baseConfig.platform();
  auto newCfg = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  auto newCfgFile = getTestConfigPath();
  newCfg.dumpConfig(newCfgFile);
  FLAGS_config = newCfgFile;
  // reload config so that test config is loaded
  ensemble->reloadPlatformConfig();
}

void ProdInvariantTest::sendTraffic(int numPackets) {
  auto state = getSw()->getState();
  auto intfID = utility::firstInterfaceIDWithPorts(state);
  auto mac = utility::getInterfaceMac(state, intfID);
  std::optional<PortID> portId = std::nullopt;
  int hopLimit = 255;
  utility::pumpTraffic(
      true,
      utility::getAllocatePktFn(getSw()),
      utility::getSendPktFunc(getSw()),
      mac,
      getAgentEnsemble()->getVlanIDForTx(),
      portId,
      hopLimit,
      numPackets);
}

PortID ProdInvariantTest::getDownlinkPort() {
  // pick the first downlink in the list
  auto downlinkPort = utility::getAllUplinkDownlinkPorts(
                          getSw()->getPlatformType(),
                          getSw()->getConfig(),
                          kEcmpWidth,
                          is_mmu_lossless_mode())
                          .second[0];
  return downlinkPort;
}

std::vector<PortID> ProdInvariantTest::getEcmpPortIds() {
  std::vector<PortID> ecmpPortIds{};
  for (auto portDesc : ecmpPorts_) {
    EXPECT_TRUE(portDesc.isPhysicalPort());
    auto portId = portDesc.phyPortID();
    ecmpPortIds.emplace_back(portId);
  }
  return ecmpPortIds;
}

void ProdInvariantTest::verifyAcl() {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto switchConfig = getSw()->getConfig();
  auto aclTableGroup = utility::getAclTableGroup(switchConfig);
  auto switchId = getSw()->getScopeResolver()->scope(*aclTableGroup).switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);

  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(client->sync_isDefaultAclTableEnabled()); });

  XLOG(DBG2) << "Verify ACL Done";
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ProdInvariantTest::verifyCopp() {
  AgentEnsemble* ensemble = getAgentEnsemble();
  std::vector<PortID> ports = getAllPlatformPorts(ensemble->getPlatformPorts());
  auto switchId = getSw()->getScopeResolver()->scope(ports).switchId();
  auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);

  utility::verifyCoppInvariantHelper(
      getSw(), switchId, asic, getSw()->getState(), getDownlinkPort());
  XLOG(DBG2) << "Verify COPP Done";
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ProdInvariantTest::verifyLoadBalancing(int numPackets) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
      getPortStatsFn = [&](const std::vector<PortID>& portIds) {
        return ensemble->getLatestPortStats(portIds);
      };
  utility::pumpTrafficAndVerifyLoadBalanced(
      [=, this]() { sendTraffic(numPackets); },
      [=, this]() {
        auto ports = std::make_unique<std::vector<int32_t>>();
        auto ecmpPortIds = getEcmpPortIds();
        for (auto ecmpPortId : ecmpPortIds) {
          ports->push_back(static_cast<int32_t>(ecmpPortId));
        }
        ensemble->clearPortStats(ports);
      },
      [=, this]() {
        return utility::isLoadBalanced(
            ecmpPorts_,
            std::vector<NextHopWeight>(ecmpPorts_.size(), 1),
            getPortStatsFn,
            25);
      });
  XLOG(DBG2) << "Verify Load Balancing Done";
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ProdInvariantTest::verifyDscpToQueueMapping() {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto hwAsics = ensemble->getSw()->getHwAsicTable()->getL3Asics();
  auto asic = checkSameAndGetAsic(hwAsics);

  if (!asic->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto uplinkDownlinkPorts = utility::getAllUplinkDownlinkPorts(
      getSw()->getPlatformType(),
      getSw()->getConfig(),
      kEcmpWidth,
      is_mmu_lossless_mode());

  // gather all uplink + downlink ports
  std::vector<PortID> portIds = uplinkDownlinkPorts.first;
  for (auto it = uplinkDownlinkPorts.second.begin();
       it != uplinkDownlinkPorts.second.end();
       ++it) {
    portIds.push_back(*it);
  }

  auto getPortStatsFn = [&]() -> std::map<PortID, HwPortStats> {
    return ensemble->getLatestPortStats(portIds);
  };

  auto config = ensemble->getSw()->getConfig();
  auto q2dscpMap = utility::getOlympicQosMaps(config);
  // To account for switches that take longer to update port stats, bump sleep
  // time to 100ms.
  if (ecmpPorts_.size() == 0) {
    EXPECT_TRUE(
        utility::verifyQueueMappingsInvariantSinglePortHelper(
            q2dscpMap,
            getSw(),
            getSw()->getState(),
            getPortStatsFn,
            getDownlinkPort(),
            100 /* sleep in ms */));
    XLOG(DBG2) << "Verify DSCP to Queue Mapping Done for single port";
    std::this_thread::sleep_for(std::chrono::seconds(1));
  } else {
    EXPECT_TRUE(
        utility::verifyQueueMappingsInvariantEcmpHelper(
            q2dscpMap,
            getSw(),
            getSw()->getState(),
            getPortStatsFn,
            getEcmpPortIds(),
            100 /* sleep in ms */));
    XLOG(DBG2) << "Verify DSCP to Queue Mapping Done";
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void ProdInvariantTest::verifyQueuePerHostMapping(bool dscpMarkingTest) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto vlanId = ensemble->getVlanIDForTx();
  auto intfMac = utility::getMacForFirstInterfaceWithPorts(getSw()->getState());
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO());

  // if DscpMarkingTest is set, send unmarked packet matching DSCP marking ACL,
  // but expect queue-per-host to be honored, as the DSCP Marking ACL is listed
  // AFTER queue-per-host ACL by design.
  std::optional<uint16_t> l4SrcPort = std::nullopt;
  std::optional<uint8_t> dscp = std::nullopt;
  if (dscpMarkingTest) {
    l4SrcPort = utility::kUdpPorts().front();
    dscp = 0;
  }

  utility::verifyQueuePerHostMapping(
      ensemble,
      vlanId,
      srcMac,
      intfMac,
      folly::IPAddressV4("1.0.0.1"),
      folly::IPAddressV4("10.10.1.2"),
      true /* useFrontPanel */,
      false /* blockNeighbor */,
      l4SrcPort,
      std::nullopt, /* l4DstPort */
      dscp);
  XLOG(DBG2) << "Verify Queue per Host Mapping Done";
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ProdInvariantTest::printDiagCmd(const std::string& cmd) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto hwSwitchRunStateMap =
      getSw()->getHwSwitchHandler()->getHwSwitchRunStates();
  // validate safe diag commands on all switches
  for (auto& [switchId, runState] : hwSwitchRunStateMap) {
    auto client = ensemble->getHwAgentTestClient(SwitchID(switchId));
    client = ensemble->getHwAgentTestClient(SwitchID(switchId));
    client->sync_printDiagCmd(cmd);
  }
}

void ProdInvariantTest::verifySafeDiagCommands() {
  AgentEnsemble* ensemble = getAgentEnsemble();
  std::set<std::string> diagCmds;
  auto hwAsics = ensemble->getSw()->getHwAsicTable()->getL3Asics();
  auto asic = checkSameAndGetAsic(hwAsics);

  switch (asic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_AGERA3:
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_CHENAB:
      break;

    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      diagCmds = validated_shell_commands_constants::TD2_TESTED_CMDS();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      diagCmds = validated_shell_commands_constants::TH_TESTED_CMDS();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      diagCmds = validated_shell_commands_constants::TH3_TESTED_CMDS();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      diagCmds = validated_shell_commands_constants::TH4_TESTED_CMDS();
      break;
  }
  if (diagCmds.size()) {
    for (auto i = 0; i < 10; ++i) {
      for (auto cmd : diagCmds) {
        std::string out;
        printDiagCmd(cmd + "\n");
      }
    }

    std::string out;
    printDiagCmd("quit\n");
  }
  XLOG(DBG2) << "Verify Safe Diagnostic Commands Done";
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ProdInvariantTest::verifySwSwitchHandler() {
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread;
  auto client = setupClient<apache::thrift::Client<FbossCtrl>>("sw", evbThread);
  auto runState = client->sync_getSwitchRunState();
  EXPECT_GE(runState, SwitchRunState::CONFIGURED);
}

void ProdInvariantTest::verifyHwSwitchHandler() {
  auto hwSwitchRunStateMap =
      getSw()->getHwSwitchHandler()->getHwSwitchRunStates();
  for (auto& [switchId, runState] : hwSwitchRunStateMap) {
    EXPECT_GE(runState, SwitchRunState::CONFIGURED);
  }
}

void ProdInvariantTest::verifyThriftHandler() {
  verifySwSwitchHandler();
  verifyHwSwitchHandler();

  XLOG(DBG2) << "Verify Thrift Handler Done";
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

int ProdInvariantTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentEnsembleTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

// Wait 1 second between tests to address flakiness on some platforms caused by
// invariants running in quick succession.
TEST_F(ProdInvariantTest, verifyInvariants) {
  auto setup = [&]() {};
  auto verify = [&]() {
    verifyAcl();
    verifyCopp();
    if (ecmpPorts_.size() >= kEcmpWidth) {
      verifyLoadBalancing();
    }
    verifyDscpToQueueMapping();
    verifySafeDiagCommands();
    verifyThriftHandler();
  };
  verifyAcrossWarmBoots(setup, verify);
}

class ProdInvariantRswMhnicTest : public ProdInvariantTest {
 protected:
  void SetUp() override {
    // Todo: Refractor could be done here to avoid code duplication.
    AgentEnsembleTest::SetUp();
    auto ecmpUplinlinkPorts =
        utility::getAllUplinkDownlinkPorts(
            getSw()->getPlatformType(), getSw()->getConfig(), kEcmpWidth, false)
            .first;
    for (auto& uplinkPort : ecmpUplinlinkPorts) {
      ecmpPorts_.emplace_back(uplinkPort);
    }
    setupRSWMhnicEcmpV4(ecmpPorts_);
    XLOG(DBG2) << "ProdInvariantTest setup done";
  }

  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) override {
    if (useProdConfig_.has_value()) {
      return *(ensemble.getSw()->getAgentConfig().sw());
    }

    // TODO: Currently ProdInvariantTests only has support for BCM
    // switches. That's why we're passing false in the call below.
    if (checkBaseConfigPortsEmpty()) {
      std::vector<PortID> ports =
          getAllPlatformPorts(ensemble.getPlatformPorts());
      auto switchId = getSw()->getScopeResolver()->scope(ports).switchId();
      auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);
      auto config = utility::createProdRswMhnicConfig(
          asic,
          ensemble.getSw()->getPlatformType(),
          ensemble.getSw()->getPlatformMapping(),
          ensemble.getSw()->getPlatformSupportsAddRemovePort(),
          ports,
          false /* isSai() */);
      useProdConfig_ = false;
      return config;
    } else {
      useProdConfig_ = true;
      return getConfigFromFlag();
    }
  }

  RoutePrefix<folly::IPAddressV4> kGetRoutePrefix() {
    // Currently hardcoded to IPv4. Enabling IPv6 testing for all classes is on
    // the to-do list, after which we can choose v4 or v6 with the same model as
    // kGetRoutePrefix in HwQueuePerHostRouteTests.
    return RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.10.1.0"}, 24};
  }

 private:
  void setupRSWMhnicEcmpV4(const std::vector<PortDescriptor>& ecmpPorts) {
    ASSERT_GT(ecmpPorts.size(), 0);

    boost::container::flat_set<PortDescriptor> ports;
    std::for_each(ecmpPorts.begin(), ecmpPorts.end(), [&ports](auto ecmpPort) {
      ports.insert(ecmpPort);
    });

    getSw()->updateStateBlocking("Resolve v4 nhops", [&](auto state) {
      utility::EcmpSetupTargetedPorts4 ecmp4(
          state, getSw()->needL2EntryForNeighbor());
      return ecmp4.resolveNextHops(state, ports);
    });

    utility::EcmpSetupTargetedPorts4 ecmp4(
        getSw()->getState(), getSw()->needL2EntryForNeighbor());
    ecmp4.programRoutes(
        std::make_unique<SwSwitchRouteUpdateWrapper>(
            getSw()->getRouteUpdater()),
        ports,
        {kGetRoutePrefix()});
  }
};

TEST_F(ProdInvariantRswMhnicTest, verifyInvariants) {
  // TODO: Currently this test does not work. Packets are not recognized in the
  // queues. Notes: Packets are either dropped during transfer or not sent to
  // the correct PortID. Debugging shows that PortID(5) (manually changed)
  // occasionally hold packets in queue 0. But we are still not getting two
  // packets as expected. Printouts shows other queues of the expected port also
  // does not have any pkts. Show c log shows that there are 2 pkts going
  // through ce1.
  auto setup = [&]() {
    auto updaterPointer = std::make_unique<SwSwitchRouteUpdateWrapper>(
        getSw()->getRouteUpdater());
    utility::updateRoutesClassID(
        {{kGetRoutePrefix(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2}},
        updaterPointer.get());
  };
  auto verify = [&]() {
    verifyAcl();
    verifyCopp();
    verifyLoadBalancing();
    verifyDscpToQueueMapping();
    verifyQueuePerHostMapping(true /*dscpMarkingTest*/);
    verifyQueuePerHostMapping(false /*dscpMarkingTest*/);
    verifySafeDiagCommands();
  };
  verifyAcrossWarmBoots(setup, verify);
}

class ProdInvariantRtswTest : public ProdInvariantTest {
 public:
  ProdInvariantRtswTest() {
    set_mmu_lossless(true);
  }

 protected:
  void SetUp() override {
    ProdInvariantTest::SetUp();
  }

  void setCmdLineFlagOverrides() const override {
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_flowletStatsEnable = true;
    ProdInvariantTest::setCmdLineFlagOverrides();
  }

  cfg::SwitchConfig getConfigFromFlag() override {
    auto config = ProdInvariantTest::getConfigFromFlag();

    // alter the flowlet-* dstIP (present only when dlb_crosszone enabled) to
    // match IP of the test RoCE packet.
    for (auto& acl : *config.acls()) {
      if (acl.dstIp().has_value() &&
          (acl.name().value().find("flowlet") != std::string::npos)) {
        acl.dstIp() = "2001::/50";
      }
    }

    return config;
  }

  void verifyFlowletAcls() {
    auto config = getSw()->getConfig();
    // verify udf roce opcode ACL
    sendAndVerifyRoCETraffic(
        utility::kUdfRoceOpcodeAck,
        utility::kUdfAclRoceOpcodeName,
        utility::isSaiConfig(config) ? utility::kUdfAclRoceOpcodeName
                                     : utility::kUdfAclRoceOpcodeStats);

    // verify flowlet ACL to enable DLB
    sendAndVerifyRoCETraffic(
        utility::kUdfRoceOpcodeWriteImmediate,
        "flowlet-selective-enable",
        utility::isSaiConfig(config) ? "flowlet-selective-enable"
                                     : "flowlet-selective-stats");

    // 12 is a random opcode not ack or write-imm. This ACL will go away once
    // all RTSWs transition to spray
    sendAndVerifyRoCETraffic(
        12,
        "flowlet-enable",
        utility::isSaiConfig(config) ? "flowlet-enable" : "flowlet-stats");

    ASSERT_TRUE(flowletAclsFound_ > 0);
  }

  void verifyEEcmp() {
    AgentEnsemble* ensemble = getAgentEnsemble();
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getPortStatsFn = [&](const std::vector<PortID>& portIds) {
          return ensemble->getLatestPortStats(portIds);
        };
    utility::pumpTrafficAndVerifyLoadBalanced(
        [=, this]() {
          sendRoCETraffic(utility::kUdfRoceOpcodeAck, kLbPacketCount);
        },
        [=, this]() {
          auto ports = std::make_unique<std::vector<int32_t>>();
          auto ecmpPortIds = getEcmpPortIds();
          for (auto ecmpPortId : ecmpPortIds) {
            ports->push_back(static_cast<int32_t>(ecmpPortId));
          }
          ensemble->clearPortStats(ports);
        },
        [=, this]() {
          return utility::isLoadBalanced(
              ecmpPorts_,
              std::vector<NextHopWeight>(ecmpPorts_.size(), 1),
              getPortStatsFn,
              kLbDeviation);
        });
    XLOG(DBG2) << "Verify E-Ecmp Done";
  }

  void verifyDlbGroups() {
    // DLB groups can be either spray or flowlet
    // Verify that the default route uses dynamic load balancing (DLB)
    RoutePrefixV6 defaultPrefix{folly::IPAddressV6("::"), 0};
    auto switchingMode = getAgentEnsemble()->getFwdSwitchingMode(defaultPrefix);
    XLOG(DBG2) << "Default route switching mode: "
               << apache::thrift::util::enumNameSafe(switchingMode);
    WITH_RETRIES({
      // DLB modes are FLOWLET_QUALITY and PER_PACKET_QUALITY
      EXPECT_TRUE(
          switchingMode == cfg::SwitchingMode::FLOWLET_QUALITY ||
          switchingMode == cfg::SwitchingMode::PER_PACKET_QUALITY);
    });
    XLOG(DBG2) << "Verify DLB groups Done";
  }

 private:
  void sendRoCETraffic(int opcode, int packetCount = 1) {
    auto mac = utility::getInterfaceMac(
        getSw()->getState(), getSw()->getState()->getVlans()->getFirstVlanID());

    utility::pumpRoCETraffic(
        true,
        utility::getAllocatePktFn(getSw()),
        utility::getSendPktFunc(getSw()),
        mac,
        getSw()->getState()->getVlans()->getFirstVlanID(),
        getDownlinkPort(),
        utility::kUdfL4DstPort,
        255,
        std::nullopt,
        packetCount,
        opcode,
        utility::kRoceReserved);
  }

  void sendAndVerifyRoCETraffic(
      int opcode,
      const std::string& aclName,
      const std::string& counterName) {
    // DLB configuration is still evolving so we will opportunistically test
    // ACLs if they are present on a given device
    auto config = getSw()->getConfig();
    if (!utility::checkConfigHasAclEntry(config, aclName)) {
      XLOG(DBG2) << aclName << " ACL entry not found, skipping verification";
      return;
    }

    auto aclPktCountBefore = utility::getAclInOutPackets(getSw(), counterName);
    sendRoCETraffic(opcode);

    WITH_RETRIES({
      auto aclPktCountAfter = utility::getAclInOutPackets(getSw(), counterName);

      XLOG(DBG2) << "\n"
                 << "aclPacketCounter(" << counterName
                 << "): " << aclPktCountBefore << " -> " << (aclPktCountAfter);

      // On some ASICs looped back pkt hits the ACL before being
      // dropped in the ingress pipeline, hence GE
      EXPECT_EVENTUALLY_GE(aclPktCountAfter, aclPktCountBefore + 1);
      // At most we should get a pkt bump of 2
      EXPECT_EVENTUALLY_LE(aclPktCountAfter, aclPktCountBefore + 2);
    });
    flowletAclsFound_++;
  }
  int flowletAclsFound_ = 0;
};

TEST_F(ProdInvariantRtswTest, verifyInvariants) {
  auto setup = [&]() {};
  auto verify = [&]() {
    verifyAcl();
    verifyCopp();
    verifyLoadBalancing();
    verifyDscpToQueueMapping();
    verifySafeDiagCommands();
    verifyThriftHandler();
    verifyFlowletAcls();
    // verifyEEcmp();
    verifyDlbGroups();
  };
  verifyAcrossWarmBoots(setup, verify);
}

class ProdInvariantFtswTest : public ProdInvariantRtswTest {
 public:
  ProdInvariantFtswTest() {
    set_mmu_lossless(true);
  }
};

TEST_F(ProdInvariantFtswTest, verifyInvariants) {
  auto setup = [&]() {};
  auto verify = [&]() {
    verifyAcl();
    verifyCopp();
    verifyLoadBalancing();
    verifyDscpToQueueMapping();
    verifySafeDiagCommands();
    verifyThriftHandler();
    verifyFlowletAcls();
    verifyDlbGroups();
  };
  verifyAcrossWarmBoots(setup, verify);
}

class ProdInvariantStswTest : public ProdInvariantRtswTest {
 protected:
  void SetUp() override {
    AgentEnsembleTest::SetUp();
    AgentEnsemble* ensemble = getAgentEnsemble();
    // Spine switches do not have uplinks. They are highest in our spine leaf
    // hierarchy.
    auto ecmpDownlinkPorts = utility::getAllUplinkDownlinkPorts(
                                 getSw()->getPlatformType(),
                                 initialConfig(*ensemble),
                                 kEcmpWidth,
                                 is_mmu_lossless_mode())
                                 .second;
    for (auto& downlinkPort : ecmpDownlinkPorts) {
      ecmpPorts_.emplace_back(downlinkPort);
    }
    setupAgentTestEcmp(ecmpPorts_);
    XLOG(DBG2) << "ProdInvariantTest setup done";
  }

 public:
  ProdInvariantStswTest() {
    set_mmu_lossless(true);
  }
};

TEST_F(ProdInvariantStswTest, verifyInvariants) {
  auto setup = [&]() {};
  auto verify = [&]() {
    verifyAcl();
    verifyCopp();
    verifyLoadBalancing(30000);
    verifyDscpToQueueMapping();
    verifySafeDiagCommands();
    verifyThriftHandler();
    verifyFlowletAcls();
    verifyDlbGroups();
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
