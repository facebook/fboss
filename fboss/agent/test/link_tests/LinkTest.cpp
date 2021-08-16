// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/gen/Base.h>
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

// Waits till the link status of the passed in ports reaches
// the expected state
void LinkTest::waitForLinkStatus(
    const std::vector<PortID>& portsToCheck,
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  XLOG(INFO) << "Checking link status on "
             << folly::join(",", getPortNames(getCabledPorts()));
  auto portStatus = sw()->getPortStatus();
  std::vector<PortID> badPorts;
  while (retries--) {
    badPorts.clear();
    for (const auto& port : portsToCheck) {
      if (*portStatus[port].up_ref() != up) {
        std::this_thread::sleep_for(msBetweenRetry);
        portStatus = sw()->getPortStatus();
        badPorts.push_back(port);
      }
    }
    if (badPorts.empty()) {
      return;
    }
  }

  auto msg = folly::format(
      "Unexpected Link status {:d} for {:s}",
      !up,
      folly::join(",", getPortNames(badPorts)));
  throw FbossError(msg);
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

// Returns the port names for a given list of portIDs
std::vector<std::string> LinkTest::getPortNames(
    const std::vector<PortID>& ports) const {
  return folly::gen::from(ports) | folly::gen::map([&](PortID port) {
           return sw()->getState()->getPort(port)->getName();
         }) |
      folly::gen::as<std::vector<std::string>>();
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
  for (auto i = 0; i < 2; ++i) {
    auto portStats = sw()->getHw()->getPortStats();
    for (auto [port, stats] : portStats) {
      auto inDiscards = *stats.inDiscards__ref();
      XLOG(INFO) << "Port: " << port << " in discards: " << inDiscards
                 << " in bytes: " << *stats.inBytes__ref()
                 << " out bytes: " << *stats.outBytes__ref();
      EXPECT_EQ(inDiscards, 0);
    }
    // Allow for a few rounds of stat collection
    sleep(1);
  }
}

void LinkTest::createL3DataplaneFlood() {
  auto ecmpPorts = getVlanOwningCabledPorts();
  ASSERT_GT(ecmpPorts.size(), 0);
  sw()->updateStateBlocking("Resolve nhops", [this, ecmpPorts](auto state) {
    utility::EcmpSetupTargetedPorts6 ecmp6(
        state, sw()->getPlatform()->getLocalMac());
    return ecmp6.resolveNextHops(state, ecmpPorts);
  });
  utility::EcmpSetupTargetedPorts6 ecmp6(
      sw()->getState(), sw()->getPlatform()->getLocalMac());
  ecmp6.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(sw()->getRouteUpdater()),
      ecmpPorts);
  for (const auto& nextHop : ecmp6.getNextHops()) {
    if (ecmpPorts.find(nextHop.portDesc) != ecmpPorts.end()) {
      utility::disableTTLDecrements(
          sw()->getHw(), ecmp6.getRouterId(), nextHop);
    }
  }
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
      XLOG(INFO) << " No lldp neighbors on : " << port;
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

int linkTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
