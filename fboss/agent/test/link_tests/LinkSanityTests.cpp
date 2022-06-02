// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"
#include "fboss/util/QsfpUtilTx.h"

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
    EXPECT_NO_THROW(waitForAllTransceiverStates(false, 60, 5s));

    // Set the port status on all cabled ports to true. The link should come
    // back up
    for (const auto& port : ports) {
      setPortStatus(port, true);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    EXPECT_NO_THROW(waitForAllTransceiverStates(true, 60, 5s));
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
    return lldpNeighborsOnAllCabledPorts();
  };

  verifyAcrossWarmBoots(
      []() {},
      [packetsFlowingOnAllPorts, this]() {
        checkWithRetry(packetsFlowingOnAllPorts);
      });
}

TEST_F(LinkTest, warmbootIsHitLess) {
  // Create a L3 data plane flood and then assert that none of the
  // traffic bearing ports loss traffic.
  // TODO: Assert that all (non downlink) cabled ports get traffic.
  verifyAcrossWarmBoots(
      [this]() { createL3DataplaneFlood(); },
      [this]() {
        // Assert no traffic loss and no ecmp shrink. If ports flap
        // these conditions will not be true
        assertNoInDiscards();
        auto ecmpSizeInSw = getVlanOwningCabledPorts().size();
        EXPECT_EQ(
            utility::getEcmpSizeInHw(
                sw()->getHw(),
                {folly::IPAddress("::"), 0},
                RouterID(0),
                ecmpSizeInSw),
            ecmpSizeInSw);
        // Assert all cabled transceivers have ACTIVE state
        EXPECT_NO_THROW(waitForAllCabledPorts(true));
        EXPECT_NO_THROW(waitForAllTransceiverStates(true));
      });
}

TEST_F(LinkTest, ptpEnableIsHitless) {
  // disable PTP as by default we'll  have it enabled now
  sw()->updateStateBlocking("ptp disable", [](auto state) {
    auto newState = state->clone();
    auto switchSettings = newState->getSwitchSettings()->modify(&newState);
    switchSettings->setPtpTcEnable(false);
    return newState;
  });

  createL3DataplaneFlood();
  assertNoInDiscards();

  sw()->updateStateBlocking("ptp enable", [](auto state) {
    auto newState = state->clone();
    auto switchSettings = newState->getSwitchSettings()->modify(&newState);
    EXPECT_FALSE(switchSettings->isPtpTcEnable());
    switchSettings->setPtpTcEnable(true);
    return newState;
  });

  // Assert no traffic loss and no ecmp shrink. If ports flap
  // these conditions will not be true
  assertNoInDiscards();
  auto ecmpSizeInSw = getVlanOwningCabledPorts().size();
  EXPECT_EQ(
      utility::getEcmpSizeInHw(
          sw()->getHw(),
          {folly::IPAddress("::"), 0},
          RouterID(0),
          ecmpSizeInSw),
      ecmpSizeInSw);
}

TEST_F(LinkTest, opticsTxDisableEnable) {
  auto setTxDisable = [](const auto& ports, auto disable) {
    XLOG(INFO) << "opticsTxDisableEnable: About to "
               << (disable ? "disable" : "enable") << " ports";
    FLAGS_direct_i2c = false;
    FLAGS_tx_disable = disable;
    FLAGS_tx_enable = !disable;
    auto evb = folly::EventBase();
    QsfpUtilTx(nullptr, ports, evb).setTxDisable();
  };
  auto [opticalPorts, opticalPortNames] = getOpticalCabledPortsAndNames();
  EXPECT_FALSE(opticalPorts.empty())
      << "opticsTxDisableEnable: Did not detect any optical transceivers";

  if (!opticalPorts.empty()) {
    std::vector<unsigned int> ports;
    for (const auto& port : opticalPorts) {
      ports.push_back(static_cast<unsigned int>(port));
    }
    setTxDisable(ports, true);
    XLOG(INFO) << fmt::format(
        "opticsTxDisableEnable: tx disable finished. Awaiting links to go down...");
    EXPECT_NO_THROW(waitForLinkStatus(opticalPorts, false));
    XLOG(INFO) << "opticsTxDisableEnable: link Tx disabled";

    setTxDisable(ports, false);
    XLOG(INFO) << fmt::format(
        "opticsTxDisableEnable: tx enable finished. Awaiting links to go up...");
    EXPECT_NO_THROW(waitForLinkStatus(opticalPorts, true));
    XLOG(INFO) << "opticsTxDisableEnable: links are up";
  }
}
