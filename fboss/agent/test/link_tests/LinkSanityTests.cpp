// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

using namespace ::testing;
using namespace facebook::fboss;

// Tests that the link comes up after a flap on the ASIC
TEST_F(LinkTest, asicLinkFlap) {
  auto verify = [this]() {
    auto ports = getCabledPorts();
    // Set the port status on all cabled ports to false. The link should go down
    for (const auto& port : ports) {
      setPortStatus(port, false);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(false));

    // Set the port status on all cabled ports to true. The link should come
    // back up
    for (const auto& port : ports) {
      setPortStatus(port, true);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(LinkTest, getTransceivers) {
  auto allTransceiversPresent = [this]() {
    auto ports = getCabledPorts();
    // Set the port status on all cabled ports to false. The link should go
    // down
    for (const auto& port : ports) {
      auto transceiverId =
          platform()->getPlatformPort(port)->getTransceiverID().value();
      if (!platform()->getQsfpCache()->getIf(transceiverId)) {
        return false;
      }
    }
    return true;
  };

  verifyAcrossWarmBoots(
      []() {},
      [allTransceiversPresent, this]() {
        checkWithRetry(allTransceiversPresent);
      });
}

TEST_F(LinkTest, trafficRxTx) {
  auto packetsFlowingOnAllPorts = [this]() {
    sw()->getLldpMgr()->sendLldpOnAllPorts();
    auto lldpDb = sw()->getLldpMgr()->getDB();
    for (const auto& port : getCabledPorts()) {
      if (!lldpDb->getNeighbors(port).size()) {
        XLOG(INFO) << " No lldp neighbors on : " << port;
        return false;
      }
    }
    return true;
  };

  verifyAcrossWarmBoots(
      []() {},
      [packetsFlowingOnAllPorts, this]() {
        checkWithRetry(packetsFlowingOnAllPorts);
      });
}
