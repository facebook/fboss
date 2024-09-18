// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <gflags/gflags.h>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types_custom_protocol.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types_custom_protocol.h"

namespace facebook::fboss {

void AgentEnsembleLinkTest::SetUp() {
  AgentEnsembleTest::SetUp();
  initializeCabledPorts();
  // Wait for all the cabled ports to link up before finishing the setup
  waitForAllCabledPorts(true, 60, 5s);
  utility::waitForAllTransceiverStates(true, getCabledTranceivers(), 60, 5s);
  XLOG(DBG2) << "Multi Switch Link Test setup ready";
}

void AgentEnsembleLinkTest::TearDown() {
  // Expect the qsfp service to be running at the end of the tests
  auto qsfpServiceClient = utils::createQsfpServiceClient();
  EXPECT_EQ(
      facebook::fb303::cpp2::fb_status::ALIVE,
      qsfpServiceClient.get()->sync_getStatus())
      << "QSFP Service no longer alive after the test";
  EXPECT_EQ(
      QsfpServiceRunState::ACTIVE,
      qsfpServiceClient.get()->sync_getQsfpServiceRunState())
      << "QSFP Service run state no longer active after the test";
  AgentEnsembleTest::TearDown();
}

void AgentEnsembleLinkTest::setCmdLineFlagOverrides() const {
  FLAGS_enable_macsec = true;
  AgentEnsembleTest::setCmdLineFlagOverrides();
}

void AgentEnsembleLinkTest::overrideL2LearningConfig(
    bool swLearning,
    int ageTimer) {
  auto agentConfig = AgentConfig::fromFile(FLAGS_config);
  cfg::AgentConfig testConfig = agentConfig->thrift;
  if (swLearning) {
    testConfig.sw()->switchSettings()->l2LearningMode() =
        cfg::L2LearningMode::SOFTWARE;
  } else {
    testConfig.sw()->switchSettings()->l2LearningMode() =
        cfg::L2LearningMode::HARDWARE;
  }
  testConfig.sw()->switchSettings()->l2AgeTimerSeconds() = ageTimer;
  auto newAgentConfig = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  newAgentConfig.dumpConfig(getTestConfigPath());
  FLAGS_config = getTestConfigPath();
}

void AgentEnsembleLinkTest::setupTtl0ForwardingEnable() {
  if (!isSupportedOnAllAsics(HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    // don't configure if not supported
    return;
  }
  auto agentConfig = AgentConfig::fromFile(FLAGS_config);
  auto newAgentConfig =
      utility::setTTL0PacketForwardingEnableConfig(getSw(), *agentConfig);
  newAgentConfig.dumpConfig(getTestConfigPath());
  FLAGS_config = getTestConfigPath();
}

// Waits till the link status of the ports in cabledPorts vector reaches
// the expected state
void AgentEnsembleLinkTest::waitForAllCabledPorts(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForLinkStatus(getCabledPorts(), up, retries, msBetweenRetry);
}

// Initializes the vector that holds the ports that are expected to be cabled.
// If the expectedLLDPValues in the switch config has an entry, we expect
// that port to take part in the test
void AgentEnsembleLinkTest::initializeCabledPorts() {
  const auto& platformPorts = getSw()->getPlatformMapping()->getPlatformPorts();

  auto swConfig = getSw()->getConfig();
  const auto& chips = getSw()->getPlatformMapping()->getChips();
  for (const auto& port : *swConfig.ports()) {
    if (!(*port.expectedLLDPValues()).empty()) {
      auto portID = *port.logicalID();
      cabledPorts_.push_back(PortID(portID));
      if (*port.portType() == cfg::PortType::FABRIC_PORT) {
        cabledFabricPorts_.push_back(PortID(portID));
      }
      const auto platformPortEntry = platformPorts.find(portID);
      EXPECT_TRUE(platformPortEntry != platformPorts.end())
          << "Can't find port:" << portID << " in PlatformMapping";
      auto transceiverID =
          utility::getTransceiverId(platformPortEntry->second, chips);
      if (transceiverID.has_value()) {
        cabledTransceivers_.insert(*transceiverID);
      }
    }
  }
}

std::tuple<std::vector<PortID>, std::string>
AgentEnsembleLinkTest::getOpticalCabledPortsAndNames(bool pluggableOnly) const {
  std::string opticalPortNames;
  std::vector<PortID> opticalPorts;
  std::vector<int32_t> transceiverIds;
  for (const auto& port : getCabledPorts()) {
    auto portName = getPortName(port);
    // TODO to find equivalent to getPlatformPort
    auto tcvrId =
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(port);
    transceiverIds.push_back(tcvrId);
  }

  auto transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);
  for (const auto& port : getCabledPorts()) {
    auto portName = getPortName(port);
    auto tcvrId =
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(port);
    auto tcvrInfo = transceiverInfos.find(tcvrId);

    if (tcvrInfo != transceiverInfos.end()) {
      auto tcvrState = *tcvrInfo->second.tcvrState();
      if (TransmitterTechnology::OPTICAL ==
          tcvrState.cable().value_or({}).transmitterTech()) {
        if (!pluggableOnly ||
            (pluggableOnly &&
             (tcvrState.identifier().value_or({}) !=
              TransceiverModuleIdentifier::MINIPHOTON_OBO))) {
          opticalPorts.push_back(port);
          opticalPortNames += portName + " ";
        } else {
          XLOG(DBG2) << "Transceiver: " << tcvrId + 1 << ", " << portName
                     << ", is on-board optics, skip it";
        }
      } else {
        XLOG(DBG2) << "Transceiver: " << tcvrId + 1 << ", " << portName
                   << ", is not optics, skip it";
      }
    } else {
      XLOG(DBG2) << "TransceiverInfo of transceiver: " << tcvrId + 1 << ", "
                 << portName << ", is not present, skip it";
    }
  }

  return {opticalPorts, opticalPortNames};
}

const std::vector<PortID>& AgentEnsembleLinkTest::getCabledPorts() const {
  return cabledPorts_;
}

boost::container::flat_set<PortDescriptor>
AgentEnsembleLinkTest::getSingleVlanOrRoutedCabledPorts() const {
  boost::container::flat_set<PortDescriptor> ecmpPorts;
  auto singleVlanOrRoutedPorts =
      utility::getSingleVlanOrRoutedCabledPorts(getSw());
  for (auto port : getCabledPorts()) {
    if (singleVlanOrRoutedPorts.find(PortDescriptor(port)) !=
        singleVlanOrRoutedPorts.end()) {
      ecmpPorts.insert(PortDescriptor(port));
    }
  }
  return ecmpPorts;
}

void AgentEnsembleLinkTest::programDefaultRoute(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts,
    utility::EcmpSetupTargetedPorts6& ecmp6) {
  ASSERT_GT(ecmpPorts.size(), 0);
  getSw()->updateStateBlocking(
      "Resolve nhops", [ecmpPorts, &ecmp6](auto state) {
        return ecmp6.resolveNextHops(state, ecmpPorts);
      });
  ecmp6.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(getSw()->getRouteUpdater()),
      ecmpPorts);
}

void AgentEnsembleLinkTest::programDefaultRoute(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts,
    std::optional<folly::MacAddress> dstMac) {
  utility::EcmpSetupTargetedPorts6 ecmp6(getSw()->getState(), dstMac);
  programDefaultRoute(ecmpPorts, ecmp6);
}

void AgentEnsembleLinkTest::createL3DataplaneFlood(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) {
  auto switchId = scope(ecmpPorts);
  utility::EcmpSetupTargetedPorts6 ecmp6(
      getSw()->getState(), getSw()->getLocalMac(switchId));
  programDefaultRoute(ecmpPorts, ecmp6);
  utility::disableTTLDecrements(getSw(), ecmpPorts);
  auto vlanID = utility::getFirstMap(getSw()->getState()->getVlans())
                    ->cbegin()
                    ->second->getID();
  utility::pumpTraffic(
      true,
      utility::getAllocatePktFn(getSw()),
      utility::getSendPktFunc(getSw()),
      getSw()->getLocalMac(switchId),
      vlanID);
  // TODO: Assert that traffic reached a certain rate
  XLOG(DBG2) << "Created L3 Data Plane Flood";
}

bool AgentEnsembleLinkTest::checkReachabilityOnAllCabledPorts() const {
  auto lldpDb = getSw()->getLldpMgr()->getDB();
  for (const auto& port : getCabledPorts()) {
    auto portType =
        getProgrammedState()->getPorts()->getNodeIf(port)->getPortType();
    if (portType == cfg::PortType::INTERFACE_PORT &&
        !lldpDb->getNeighbors(port).size()) {
      XLOG(DBG2) << " No lldp neighbors on : " << getPortName(port);
      return false;
    }
    if (portType == cfg::PortType::FABRIC_PORT) {
      auto switchId = getSw()->getScopeResolver()->scope(port).switchId();
      // TODO(Elangovan) does the reachability have to be retried?
      auto fabricReachabilityEntries = getFabricConnectivity(switchId);
      auto fabricPortEndPoint = fabricReachabilityEntries.find(port);
      if (fabricPortEndPoint == fabricReachabilityEntries.end() ||
          !*fabricPortEndPoint->second.isAttached()) {
        XLOG(DBG2) << " No fabric end points on : " << getPortName(port);
        return false;
      }
    }
  }
  return true;
}

std::optional<PortID> AgentEnsembleLinkTest::getPeerPortID(
    PortID portId) const {
  for (auto portPair : getConnectedPairs()) {
    if (portPair.first == portId) {
      return portPair.second;
    } else if (portPair.second == portId) {
      return portPair.first;
    }
  }
  return std::nullopt;
}

std::set<std::pair<PortID, PortID>> AgentEnsembleLinkTest::getConnectedPairs()
    const {
  waitForLldpOnCabledPorts();
  std::set<std::pair<PortID, PortID>> connectedPairs;
  for (auto cabledPort : cabledPorts_) {
    PortID neighborPort;
    auto portType =
        getProgrammedState()->getPorts()->getNodeIf(cabledPort)->getPortType();
    if (portType == cfg::PortType::FABRIC_PORT) {
      auto switchId = getSw()->getScopeResolver()->scope(cabledPort).switchId();
      auto fabricReachabilityEntries = getFabricConnectivity(switchId);
      auto fabricPortEndpoint = fabricReachabilityEntries.find(cabledPort);
      if (fabricPortEndpoint == fabricReachabilityEntries.end() ||
          !*fabricPortEndpoint->second.isAttached()) {
        XLOG(DBG2) << " No fabric end points on : " << getPortName(cabledPort);
        continue;
      }
      neighborPort = PortID(fabricPortEndpoint->second.get_portId()) +
          getRemotePortOffset(getSw()->getPlatformType());
    } else {
      auto lldpNeighbors =
          getSw()->getLldpMgr()->getDB()->getNeighbors(cabledPort);
      if (lldpNeighbors.size() != 1) {
        XLOG(WARN) << "Wrong lldp neighbor size for port "
                   << getPortName(cabledPort) << ", should be 1 but got "
                   << lldpNeighbors.size();
        continue;
      }
      neighborPort = getPortID((*lldpNeighbors.begin())->getPortId());
    }
    // Insert sorted pairs, so that the same pair does not show up twice in
    // the set
    auto connectedPair = cabledPort < neighborPort
        ? std::make_pair(cabledPort, neighborPort)
        : std::make_pair(neighborPort, cabledPort);
    connectedPairs.insert(connectedPair);
  }
  return connectedPairs;
}

/*
 * getConnectedOpticalPortPairWithFeature
 *
 * Returns the set of connected port pairs with optical link and the optics
 * supporting the given feature. For feature==None, this will return set of
 * connected port pairs using optical links
 */
std::set<std::pair<PortID, PortID>>
AgentEnsembleLinkTest::getConnectedOpticalPortPairWithFeature(
    TransceiverFeature feature,
    phy::Side side) const {
  auto connectedPairs = getConnectedPairs();
  auto opticalPorts = std::get<0>(getOpticalCabledPortsAndNames(false));

  std::set<std::pair<PortID, PortID>> connectedOpticalPortPairs;
  for (auto connectedPair : connectedPairs) {
    if (std::find(
            opticalPorts.begin(), opticalPorts.end(), connectedPair.first) !=
        opticalPorts.end()) {
      connectedOpticalPortPairs.insert(connectedPair);
    }
  }

  if (feature == TransceiverFeature::NONE) {
    return connectedOpticalPortPairs;
  }

  std::vector<int32_t> transceiverIds;
  for (auto portPair : connectedOpticalPortPairs) {
    auto tcvrId = getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(
        portPair.first);

    transceiverIds.push_back(tcvrId);
    tcvrId = getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(
        portPair.second);
    transceiverIds.push_back(tcvrId);
  }

  auto transceiverInfos = utility::waitForTransceiverInfo(transceiverIds);
  std::set<std::pair<PortID, PortID>> connectedOpticalFeaturedPorts;
  for (auto portPair : connectedOpticalPortPairs) {
    auto tcvrId = getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(
        portPair.first);

    auto tcvrInfo = transceiverInfos.find(tcvrId);
    if (tcvrInfo != transceiverInfos.end()) {
      auto tcvrState = *tcvrInfo->second.tcvrState();
      if (tcvrState.diagCapability().value().diagnostics().value()) {
        if (feature == TransceiverFeature::TX_DISABLE &&
            side == phy::Side::LINE) {
          bool txDisCapable =
              tcvrState.diagCapability().value().txOutputControl().value();
          if (txDisCapable) {
            connectedOpticalFeaturedPorts.insert(portPair);
          }
        } else if (
            feature == TransceiverFeature::TX_DISABLE &&
            side == phy::Side::SYSTEM) {
          bool rxDisCapable =
              tcvrState.diagCapability().value().rxOutputControl().value();
          if (rxDisCapable) {
            connectedOpticalFeaturedPorts.insert(portPair);
          }
        } else if (
            feature == TransceiverFeature::LOOPBACK &&
            side == phy::Side::LINE) {
          bool lineLoopCapable =
              tcvrState.diagCapability().value().loopbackLine().value();
          if (lineLoopCapable) {
            connectedOpticalFeaturedPorts.insert(portPair);
          }
        } else if (
            feature == TransceiverFeature::LOOPBACK &&
            side == phy::Side::SYSTEM) {
          bool sysLoopCapable =
              tcvrState.diagCapability().value().loopbackSystem().value();
          if (sysLoopCapable) {
            connectedOpticalFeaturedPorts.insert(portPair);
          }
        } else if (feature == TransceiverFeature::VDM) {
          bool vdmCapable = tcvrState.diagCapability().value().vdm().value();
          if (vdmCapable) {
            connectedOpticalFeaturedPorts.insert(portPair);
          }
        }
      }
    }
  }

  return connectedOpticalFeaturedPorts;
}

void AgentEnsembleLinkTest::waitForLldpOnCabledPorts(
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  WITH_RETRIES_N_TIMED(retries, msBetweenRetry, {
    ASSERT_EVENTUALLY_TRUE(checkReachabilityOnAllCabledPorts());
  });
}

// Log debug information from IPHY, XPHY and optics
void AgentEnsembleLinkTest::logLinkDbgMessage(
    std::vector<PortID>& portIDs) const {
  auto iPhyInfos = getSw()->getIPhyInfo(portIDs);
  auto qsfpServiceClient = utils::createQsfpServiceClient();
  auto portNames = getPortNames(portIDs);
  std::map<std::string, phy::PhyInfo> xPhyInfos;
  std::map<int32_t, TransceiverInfo> tcvrInfos;

  try {
    qsfpServiceClient->sync_getInterfacePhyInfo(xPhyInfos, portNames);
  } catch (const std::exception& ex) {
    XLOG(WARN) << "Failed to call qsfp_service getInterfacePhyInfo(). "
               << folly::exceptionStr(ex);
  }

  std::vector<int32_t> tcvrIds;
  for (auto portID : portIDs) {
    tcvrIds.push_back(
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(portID));
  }

  try {
    qsfpServiceClient->sync_getTransceiverInfo(tcvrInfos, tcvrIds);
  } catch (const std::exception& ex) {
    XLOG(WARN) << "Failed to call qsfp_service getTransceiverInfo(). "
               << folly::exceptionStr(ex);
  }

  for (auto portID : portIDs) {
    auto portName = getPortName(portID);
    XLOG(ERR) << "Debug information for " << portName;
    if (iPhyInfos.find(portID) != iPhyInfos.end()) {
      XLOG(ERR) << "IPHY INFO: "
                << apache::thrift::debugString(iPhyInfos[portID]);
    } else {
      XLOG(ERR) << "IPHY info missing for " << portName;
    }

    if (xPhyInfos.find(portName) != xPhyInfos.end()) {
      XLOG(ERR) << "XPHY INFO: "
                << apache::thrift::debugString(xPhyInfos[portName]);
    } else {
      XLOG(ERR) << "XPHY info missing for " << portName;
    }

    auto tcvrId =
        getSw()->getPlatformMapping()->getTransceiverIdFromSwPort(portID);
    if (tcvrInfos.find(tcvrId) != tcvrInfos.end()) {
      XLOG(ERR) << "Transceiver INFO: "
                << apache::thrift::debugString(tcvrInfos[tcvrId]);
    } else {
      XLOG(ERR) << "Transceiver info missing for " << portName;
    }
  }
}

void AgentEnsembleLinkTest::setLinkState(
    bool enable,
    std::vector<PortID>& portIds) {
  for (const auto& port : portIds) {
    setPortStatus(port, enable);
  }
  EXPECT_NO_THROW(
      waitForLinkStatus(portIds, enable, 60, std::chrono::milliseconds(1000)););
}

phy::FecMode AgentEnsembleLinkTest::getPortFECMode(PortID portId) const {
  auto port = getSw()->getState()->getPorts()->getNodeIf(portId);
  auto matcher =
      PlatformPortProfileConfigMatcher(port->getProfileID(), port->getID());
  auto profile = getSw()->getPlatformMapping()->getPortProfileConfig(matcher);

  if (profile.has_value()) {
    return profile->iphy()->fec().value();
  } else {
    throw FbossError("No profile found for port: ", portId);
  }
}

std::vector<std::pair<PortID, PortID>>
AgentEnsembleLinkTest::getPortPairsForFecErrInj() const {
  auto connectedPairs = getConnectedPairs();
  std::unordered_set<phy::FecMode> supportedFecs = {
      phy::FecMode::RS528, phy::FecMode::RS544, phy::FecMode::RS544_2N};
  std::vector<std::pair<PortID, PortID>> supportedPorts;
  for (const auto& [port1, port2] : connectedPairs) {
    auto fecPort1 = getPortFECMode(port1);
    auto fecPort2 = getPortFECMode(port2);
    if (fecPort1 != fecPort2) {
      throw FbossError(
          "FEC different on both ends of the link: ", fecPort1, fecPort2);
    }
    if (supportedFecs.find(fecPort1) != supportedFecs.end()) {
      supportedPorts.push_back({port1, port2});
    }
  }
  return supportedPorts;
}

void AgentEnsembleLinkTest::setForceTrafficOverFabric(bool force) {
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

int agentEnsembleLinkTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentEnsembleTest(argc, argv, initPlatformFn, streamType);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
