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

DECLARE_bool(enable_macsec);

namespace facebook::fboss {

void LinkTest::SetUp() {
  AgentTest::SetUp();
  initializeCabledPorts();
  // Wait for all the cabled ports to link up before finishing the setup
  waitForAllCabledPorts(true);
  XLOG(INFO) << "Link Test setup ready";
}

void LinkTest::setupFlags() const {
  FLAGS_enable_macsec = true;
  AgentTest::setupFlags();
}

// Waits till the link status of the ports in cabledPorts vector reaches
// the expected state
void LinkTest::waitForAllCabledPorts(
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  waitForLinkStatus(getCabledPorts(), up, retries, msBetweenRetry);
}

// Initializes the vector that holds the ports that are expected to be cabled.
// If the expectedLLDPValues in the switch config has an entry, we expect
// that port to take part in the test
void LinkTest::initializeCabledPorts() {
  for (const auto port : *sw()->getConfig().ports_ref()) {
    if (!(*port.expectedLLDPValues_ref()).empty()) {
      cabledPorts_.push_back(PortID(*port.logicalID_ref()));
    }
  }
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

void LinkTest::assertNoInDiscards() {
  // When port stat is not updated yet post warmboot (collect timestamp will be
  // -1), retry another round on all ports.
  int numRounds = 0;
  int maxRetry = 5;
  while (numRounds < 2 && maxRetry-- > 0) {
    bool retry = false;
    auto portStats = sw()->getHw()->getPortStats();
    for (auto [port, stats] : portStats) {
      auto inDiscards = *stats.inDiscards__ref();
      XLOG(INFO) << "Port: " << port << " in discards: " << inDiscards
                 << " in bytes: " << *stats.inBytes__ref()
                 << " out bytes: " << *stats.outBytes__ref() << " at timestamp "
                 << *stats.timestamp__ref();
      if (*stats.timestamp__ref() > 0) {
        EXPECT_EQ(inDiscards, 0);
      } else {
        retry = true;
        break;
      }
    }
    numRounds = retry ? numRounds : numRounds + 1;
    // Allow for a few rounds of stat collection
    sleep(1);
  }
  // Need at least two rounds of stats collection.
  EXPECT_EQ(numRounds, 2);
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

int linkTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
