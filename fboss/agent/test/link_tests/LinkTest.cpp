// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/gen/Base.h>
#include <gflags/gflags.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/link_tests/LinkTest.h"

using namespace facebook::fboss;

namespace facebook::fboss {

void LinkTest::SetUp() {
  AgentTest::SetUp();
  initializeCabledPorts();
  // Wait for all the cabled ports to link up before finishing the setup
  waitForAllCabledPorts(true);
  XLOG(INFO) << "Link Test setup ready";
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

int linkTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
