// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <gflags/gflags.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

DECLARE_bool(enable_macsec);

namespace facebook::fboss {

void LinkTest::SetUp() {
  AgentTest::SetUp();
  initializeCabledPorts();
  // Wait for all the cabled ports to link up before finishing the setup
  // TODO(joseph5wu) Temporarily increase the timeout to 3mins because current
  // TransceiverStateMachine can only allow handle updates sequentially and for
  // Minipack xphy programming, it will take about 68s to program all 128 ports.
  // Will lower the timeout back to 1min once we can support parallel
  // programming
  waitForAllCabledPorts(true, 60, 5s);
  XLOG(INFO) << "Link Test setup ready";
}

void LinkTest::setupFlags() const {
  FLAGS_enable_macsec = true;
  // Collect stats faster instead of defaulting to 200s. So our stat
  // tests can finish faster.
  // TODO(joseph5wu) Will deprecate this gearbox_stat_interval flag in agent
  // and rely on qsfp_service to collect xphy stats
  switch (sw()->getPlatform()->getMode()) {
    case PlatformMode::MINIPACK:
    case PlatformMode::FUJI:
      // Xphy stats takes 3s here. 8 PIMS can go in parallel but the 16
      // ports on the same PIM go in serial. So the fastest we can go is
      // 3x16=48s. 60s to be safe.
      // See: getSleepSeconds() in HwStatsCollectionTest.cpp
      FLAGS_gearbox_stat_interval = 60;
      break;

    default:
      FLAGS_gearbox_stat_interval = 5;
      break;
  }
  AgentTest::setupFlags();
}

// Waits till the link status of the ports in cabledPorts vector reaches
// the expected state
void LinkTest::waitForAllCabledPorts(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForLinkStatus(getCabledPorts(), up, retries, msBetweenRetry);
  waitForStateMachineState(
      cabledTransceivers_,
      up ? TransceiverStateMachineState::ACTIVE
         : TransceiverStateMachineState::INACTIVE,
      retries,
      msBetweenRetry);
}

// Initializes the vector that holds the ports that are expected to be cabled.
// If the expectedLLDPValues in the switch config has an entry, we expect
// that port to take part in the test
void LinkTest::initializeCabledPorts() {
  const auto& platformPorts =
      sw()->getPlatform()->getPlatformMapping()->getPlatformPorts();
  const auto& chips = sw()->getPlatform()->getPlatformMapping()->getChips();
  for (const auto port : *sw()->getConfig().ports_ref()) {
    if (!(*port.expectedLLDPValues_ref()).empty()) {
      auto portID = *port.logicalID_ref();
      cabledPorts_.push_back(PortID(portID));
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
LinkTest::getOpticalCabledPortsAndNames() const {
  std::string opticalPortNames;
  std::vector<PortID> opticalPorts;

  for (const auto& port : getCabledPorts()) {
    auto portName = getPortName(port);
    auto trcvId = platform()->getPlatformPort(port)->getTransceiverID().value();
    std::optional<TransceiverInfo> trcvInfo =
        platform()->getQsfpCache()->getIf(trcvId);

    if (trcvInfo) {
      if (TransmitterTechnology::OPTICAL ==
          *(trcvInfo->cable_ref().value_or({}).transmitterTech_ref())) {
        opticalPorts.push_back(port);
        opticalPortNames += portName + " ";
      } else {
        XLOG(INFO) << "Transceiver: " << trcvId + 1 << ", " << portName
                   << ", is not optics, skip it";
      }
    } else {
      XLOG(INFO) << "TransceiverInfo of transceiver: " << trcvId + 1 << ", "
                 << portName << ", is not present, skip it";
    }
  }

  return {opticalPorts, opticalPortNames};
}

const std::vector<PortID>& LinkTest::getCabledPorts() const {
  return cabledPorts_;
}

boost::container::flat_set<PortDescriptor> LinkTest::getVlanOwningCabledPorts()
    const {
  boost::container::flat_set<PortDescriptor> ecmpPorts;
  auto vlanOwningPorts =
      utility::getPortsWithExclusiveVlanMembership(sw()->getState());
  for (auto port : getCabledPorts()) {
    if (vlanOwningPorts.find(PortDescriptor(port)) != vlanOwningPorts.end()) {
      ecmpPorts.insert(PortDescriptor(port));
    }
  }
  return ecmpPorts;
}

void LinkTest::programDefaultRoute(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts,
    utility::EcmpSetupTargetedPorts6& ecmp6) {
  ASSERT_GT(ecmpPorts.size(), 0);
  sw()->updateStateBlocking("Resolve nhops", [ecmpPorts, &ecmp6](auto state) {
    return ecmp6.resolveNextHops(state, ecmpPorts);
  });
  ecmp6.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(sw()->getRouteUpdater()),
      ecmpPorts);
}

void LinkTest::programDefaultRoute(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts,
    std::optional<folly::MacAddress> dstMac) {
  utility::EcmpSetupTargetedPorts6 ecmp6(sw()->getState(), dstMac);
  programDefaultRoute(ecmpPorts, ecmp6);
}

void LinkTest::disableTTLDecrements(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) const {
  utility::EcmpSetupTargetedPorts6 ecmp6(sw()->getState());
  for (const auto& nextHop : ecmp6.getNextHops()) {
    if (ecmpPorts.find(nextHop.portDesc) != ecmpPorts.end()) {
      utility::disableTTLDecrements(
          sw()->getHw(), ecmp6.getRouterId(), nextHop);
    }
  }
}
void LinkTest::createL3DataplaneFlood(
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) {
  utility::EcmpSetupTargetedPorts6 ecmp6(
      sw()->getState(), sw()->getPlatform()->getLocalMac());
  programDefaultRoute(ecmpPorts, ecmp6);
  disableTTLDecrements(ecmpPorts);
  utility::pumpTraffic(
      true,
      sw()->getHw(),
      sw()->getPlatform()->getLocalMac(),
      (*sw()->getState()->getVlans()->begin())->getID());
  // TODO: Assert that traffic reached a certain rate
}

bool LinkTest::lldpNeighborsOnAllCabledPorts() const {
  auto lldpDb = sw()->getLldpMgr()->getDB();
  for (const auto& port : getCabledPorts()) {
    if (!lldpDb->getNeighbors(port).size()) {
      XLOG(INFO) << " No lldp neighbors on : " << getPortName(port);
      return false;
    }
  }
  return true;
}

PortID LinkTest::getPortID(const std::string& portName) const {
  for (auto port : *sw()->getState()->getPorts()) {
    if (port->getName() == portName) {
      return port->getID();
    }
  }
  throw FbossError("No port named: ", portName);
}

std::string LinkTest::getPortName(PortID portId) const {
  for (auto port : *sw()->getState()->getPorts()) {
    if (port->getID() == portId) {
      return port->getName();
    }
  }
  throw FbossError("No port with ID: ", portId);
}

std::set<std::pair<PortID, PortID>> LinkTest::getConnectedPairs() const {
  std::set<std::pair<PortID, PortID>> connectedPairs;
  for (auto cabledPort : cabledPorts_) {
    auto lldpNeighbors = sw()->getLldpMgr()->getDB()->getNeighbors(cabledPort);
    CHECK_EQ(lldpNeighbors.size(), 1);
    auto neighborPort = getPortID(lldpNeighbors.begin()->getPortId());
    // Insert sorted pairs, so that the same pair does not show up twice in the
    // set
    auto connectedPair = cabledPort < neighborPort
        ? std::make_pair(cabledPort, neighborPort)
        : std::make_pair(neighborPort, cabledPort);
    connectedPairs.insert(connectedPair);
  }
  return connectedPairs;
}

void LinkTest::waitForStateMachineState(
    const std::set<TransceiverID>& transceiversToCheck,
    TransceiverStateMachineState stateMachineState,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  if (!FLAGS_skip_xphy_programming) {
    XLOG(INFO) << "Skip waiting for state machine state, "
               << "FLAGS_skip_xphy_programming is false";
    return;
  }
  XLOG(INFO) << "Checking qsfp TransceiverStateMachineState on "
             << folly::join(",", transceiversToCheck);

  std::vector<int32_t> expectedTransceiver;
  for (auto transceiverID : transceiversToCheck) {
    expectedTransceiver.push_back(transceiverID);
  }

  std::vector<int32_t> badTransceivers;
  while (retries--) {
    badTransceivers.clear();
    std::map<int32_t, TransceiverInfo> info;
    try {
      auto qsfpServiceClient = utils::createQsfpServiceClient();
      qsfpServiceClient->sync_getTransceiverInfo(info, expectedTransceiver);
    } catch (const std::exception& ex) {
      // We have retry mechanism to handle failure. No crash here
      XLOG(WARN) << "Failed to call qsfp_service getTransceiverInfo(). "
                 << folly::exceptionStr(ex);
    }
    // Check whether all expected transceivers have expected state
    for (auto transceiverID : expectedTransceiver) {
      // Only continue if the transceiver state machine matches
      if (auto transceiverInfoIt = info.find(transceiverID);
          transceiverInfoIt != info.end()) {
        if (auto state = transceiverInfoIt->second.stateMachineState_ref();
            state.has_value() && *state == stateMachineState) {
          continue;
        }
      }
      // Otherwise such transceiver is considered to be in a bad state
      badTransceivers.push_back(transceiverID);
    }

    if (badTransceivers.empty()) {
      XLOG(INFO) << "All qsfp TransceiverStateMachineState on "
                 << folly::join(",", expectedTransceiver) << " match "
                 << apache::thrift::util::enumNameSafe(stateMachineState);
      return;
    } else {
      /* sleep override */
      std::this_thread::sleep_for(msBetweenRetry);
    }
  }

  throw FbossError(
      "Transceivers:[",
      folly::join(",", badTransceivers),
      "] don't have expected TransceiverStateMachineState:",
      apache::thrift::util::enumNameSafe(stateMachineState));
}

int linkTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
