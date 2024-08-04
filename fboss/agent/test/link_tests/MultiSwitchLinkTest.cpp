// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/Subprocess.h>
#include <gflags/gflags.h>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/link_tests/MultiSwitchLinkTest.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types_custom_protocol.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types_custom_protocol.h"

DECLARE_bool(enable_macsec);

DECLARE_bool(skip_drain_check_for_prbs);

DEFINE_bool(
    link_stress_test,
    false,
    "enable to run stress tests (longer duration + more iterations)");

namespace {
const std::vector<std::string> kRestartQsfpService = {
    "/bin/systemctl",
    "restart",
    "qsfp_service_for_testing"};

const std::string kForceColdbootQsfpSvcFileName =
    "/dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service";

// This file stores all non-validated transceiver configurations found while
// running getAllTransceiverConfigValidationStatuses() in EmptyLinkTest. Each
// configuration will be in JSON format. This file is then downloaded by the
// Netcastle test runner to parse it and log these configurations to Scuba.
// Matching file definition is located in
// fbcode/neteng/netcastle/teams/fboss/constants.py.
const std::string kTransceiverConfigJsonsForScuba =
    "/tmp/transceiver_config_jsons_for_scuba.log";
} // namespace

namespace facebook::fboss {

void MultiSwitchLinkTest::SetUp() {
  AgentEnsembleTest::SetUp();
  initializeCabledPorts();
  // Wait for all the cabled ports to link up before finishing the setup
  waitForAllCabledPorts(true, 60, 5s);
  waitForAllTransceiverStates(true, 60, 5s);
  XLOG(DBG2) << "Multi Switch Link Test setup ready";
}

void MultiSwitchLinkTest::restartQsfpService(bool coldboot) const {
  if (coldboot) {
    createFile(kForceColdbootQsfpSvcFileName);
    XLOG(DBG2) << "Restarting QSFP Service in coldboot mode";
  } else {
    XLOG(DBG2) << "Restarting QSFP Service in warmboot mode";
  }

  folly::Subprocess(kRestartQsfpService).waitChecked();
}

void MultiSwitchLinkTest::TearDown() {
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

void MultiSwitchLinkTest::setCmdLineFlagOverrides() const {
  FLAGS_enable_macsec = true;
  FLAGS_skip_drain_check_for_prbs = true;
  AgentEnsembleTest::setCmdLineFlagOverrides();
}

void MultiSwitchLinkTest::overrideL2LearningConfig(
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
  reloadPlatformConfig();
}

void MultiSwitchLinkTest::setupTtl0ForwardingEnable() {
  if (!getSw()->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    // don't configure if not supported
    return;
  }
  auto agentConfig = AgentConfig::fromFile(FLAGS_config);
  auto newAgentConfig =
      utility::setTTL0PacketForwardingEnableConfig(getSw(), *agentConfig);
  newAgentConfig.dumpConfig(getTestConfigPath());
  FLAGS_config = getTestConfigPath();
  reloadPlatformConfig();
}

// Waits till the link status of the ports in cabledPorts vector reaches
// the expected state
void MultiSwitchLinkTest::waitForAllCabledPorts(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForLinkStatus(getCabledPorts(), up, retries, msBetweenRetry);
}

void MultiSwitchLinkTest::waitForAllTransceiverStates(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForStateMachineState(
      cabledTransceivers_,
      up ? TransceiverStateMachineState::ACTIVE
         : TransceiverStateMachineState::INACTIVE,
      retries,
      msBetweenRetry);
}

// Retrieves the config validation status for all active transceivers. Strings
// are retrieved instead of booleans because these contain the stringified JSONs
// of each non-validated transceiver config, which will be printed to stdout and
// ingested by Netcastle.
void MultiSwitchLinkTest::getAllTransceiverConfigValidationStatuses() {
  std::vector<int32_t> expectedTransceivers(
      cabledTransceivers_.begin(), cabledTransceivers_.end());
  std::map<int32_t, std::string> responses;

  try {
    auto qsfpServiceClient = utils::createQsfpServiceClient();
    qsfpServiceClient->sync_getTransceiverConfigValidationInfo(
        responses, expectedTransceivers, true);
  } catch (const std::exception& ex) {
    XLOG(WARN)
        << "Failed to call qsfp_service getTransceiverConfigValidationInfo(). "
        << folly::exceptionStr(ex);
  }

  createFile(kTransceiverConfigJsonsForScuba);

  std::vector<int32_t> missingTransceivers, invalidTransceivers;
  std::vector<std::string> nonValidatedConfigs;
  for (auto transceiverID : expectedTransceivers) {
    if (auto transceiverStatusIt = responses.find(transceiverID);
        transceiverStatusIt != responses.end()) {
      if (transceiverStatusIt->second != "") {
        invalidTransceivers.push_back(transceiverID);
        nonValidatedConfigs.push_back(transceiverStatusIt->second);
      }
      continue;
    }
    missingTransceivers.push_back(transceiverID);
  }
  if (nonValidatedConfigs.size()) {
    writeSysfs(
        kTransceiverConfigJsonsForScuba,
        folly::join("\n", nonValidatedConfigs));
  }
  if (!missingTransceivers.empty()) {
    XLOG(DBG2) << "Transceivers [" << folly::join(",", missingTransceivers)
               << "] are missing config validation status.";
  }
  if (!invalidTransceivers.empty()) {
    XLOG(DBG2) << "Transceivers [" << folly::join(",", invalidTransceivers)
               << "] have non-validated configurations.";
  }
  if (missingTransceivers.empty() && invalidTransceivers.empty()) {
    XLOG(DBG2) << "Transceivers [" << folly::join(",", expectedTransceivers)
               << "] all have validated configurations.";
  }
}

// Wait until we have successfully fetched transceiver info (and thus know
// which transceivers are available for testing)
std::map<int32_t, TransceiverInfo> MultiSwitchLinkTest::waitForTransceiverInfo(
    std::vector<int32_t> transceiverIds,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  std::map<int32_t, TransceiverInfo> info;
  while (retries--) {
    try {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      qsfpServiceClient->sync_getTransceiverInfo(info, transceiverIds);
    } catch (const std::exception& ex) {
      XLOG(WARN) << "Failed to call qsfp_service getTransceiverInfo(). "
                 << folly::exceptionStr(ex);
    }
    // Make sure we have at least one present transceiver
    for (const auto& it : info) {
      if (*it.second.tcvrState()->present()) {
        return info;
      }
    }
    XLOG(DBG2) << "TransceiverInfo was empty";
    if (retries) {
      /* sleep override */
      std::this_thread::sleep_for(msBetweenRetry);
    }
  }

  throw FbossError("TransceiverInfo was never populated.");
}

// Initializes the vector that holds the ports that are expected to be cabled.
// If the expectedLLDPValues in the switch config has an entry, we expect
// that port to take part in the test
void MultiSwitchLinkTest::initializeCabledPorts() {
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
MultiSwitchLinkTest::getOpticalCabledPortsAndNames(bool pluggableOnly) const {
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

  auto transceiverInfos = waitForTransceiverInfo(transceiverIds);
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

const std::vector<PortID>& MultiSwitchLinkTest::getCabledPorts() const {
  return cabledPorts_;
}

boost::container::flat_set<PortDescriptor>
MultiSwitchLinkTest::getVlanOwningCabledPorts() const {
  boost::container::flat_set<PortDescriptor> ecmpPorts;
  auto vlanOwningPorts =
      utility::getPortsWithExclusiveVlanMembership(getSw()->getState());
  for (auto port : getCabledPorts()) {
    if (vlanOwningPorts.find(PortDescriptor(port)) != vlanOwningPorts.end()) {
      ecmpPorts.insert(PortDescriptor(port));
    }
  }
  return ecmpPorts;
}

void MultiSwitchLinkTest::programDefaultRoute(
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

void MultiSwitchLinkTest::programDefaultRoute(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts,
    std::optional<folly::MacAddress> dstMac) {
  utility::EcmpSetupTargetedPorts6 ecmp6(getSw()->getState(), dstMac);
  programDefaultRoute(ecmpPorts, ecmp6);
}

void MultiSwitchLinkTest::disableTTLDecrements(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) {
  if (getSw()->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    disableTTLDecrementOnPorts(ecmpPorts);
  } else {
    utility::EcmpSetupTargetedPorts6 ecmp6(getSw()->getState());
    utility::disableTTLDecrements(
        getSw(), ecmp6.getRouterId(), ecmp6.getNextHops());
  }
}

void MultiSwitchLinkTest::createL3DataplaneFlood(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) {
  auto switchId = scope(ecmpPorts);
  utility::EcmpSetupTargetedPorts6 ecmp6(
      getSw()->getState(), getSw()->getLocalMac(switchId));
  programDefaultRoute(ecmpPorts, ecmp6);
  disableTTLDecrements(ecmpPorts);
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

bool MultiSwitchLinkTest::checkReachabilityOnAllCabledPorts() const {
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

std::string MultiSwitchLinkTest::getPortName(PortID portId) const {
  for (auto portMap : std::as_const(*getSw()->getState()->getPorts())) {
    for (auto port : std::as_const(*portMap.second)) {
      if (port.second->getID() == portId) {
        return port.second->getName();
      }
    }
  }
  throw FbossError("No port with ID: ", portId);
}

std::vector<std::string> MultiSwitchLinkTest::getPortName(
    const std::vector<PortID>& portIDs) const {
  std::vector<std::string> portNames;
  for (auto port : portIDs) {
    portNames.push_back(getPortName(port));
  }
  return portNames;
}

std::optional<PortID> MultiSwitchLinkTest::getPeerPortID(PortID portId) const {
  for (auto portPair : getConnectedPairs()) {
    if (portPair.first == portId) {
      return portPair.second;
    } else if (portPair.second == portId) {
      return portPair.first;
    }
  }
  return std::nullopt;
}

std::set<std::pair<PortID, PortID>> MultiSwitchLinkTest::getConnectedPairs()
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
MultiSwitchLinkTest::getConnectedOpticalPortPairWithFeature(
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

  auto transceiverInfos = waitForTransceiverInfo(transceiverIds);
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

int multiSwitchLinkTestMain(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentEnsembleTest(argc, argv, initPlatformFn, streamType);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
