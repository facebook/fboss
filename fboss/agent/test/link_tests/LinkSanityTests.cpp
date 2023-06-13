// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/Subprocess.h>
#include <gtest/gtest.h>
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"

using namespace ::testing;
using namespace facebook::fboss;

namespace {
bool isEqual(
    const TransceiverIdxThrift& left,
    const TransceiverIdxThrift& right) {
  auto getChannelId = [](const TransceiverIdxThrift& idx) {
    if (!idx.channelId().has_value()) {
      return std::optional<int32_t>();
    }
    return std::make_optional(static_cast<int>(*idx.channelId()));
  };

  auto getChannels = [](const TransceiverIdxThrift& idx) {
    std::set<int32_t> channels;
    if (!idx.channels().has_value()) {
      return channels;
    }
    for (auto channel : *idx.channels()) {
      channels.insert(channel);
    }
    return channels;
  };

  return *left.transceiverId() == *right.transceiverId() &&
      getChannelId(left) == getChannelId(right) &&
      getChannels(left) == getChannels(right);
}
} // namespace

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
  auto verify = [this]() {
    WITH_RETRIES({
      auto ports = getCabledPorts();
      // Set the port status on all cabled ports to false. The link should go
      // down
      for (const auto& port : ports) {
        auto transceiverId =
            platform()->getPlatformPort(port)->getTransceiverID().value();
        EXPECT_EVENTUALLY_TRUE(platform()->getQsfpCache()->getIf(transceiverId))
            << "TcvrId " << transceiverId;
      }
    })

    WITH_RETRIES({
      auto ports = getCabledPorts();
      for (const auto& port : ports) {
        auto speed = sw()->getState()->getPorts()->getNode(port)->getSpeed();
        auto transceiverIndx0 = platform()->getPortMapping(port, speed);
        auto transceiverIndx1 = sw()->getTransceiverIdxThrift(port);
        EXPECT_EVENTUALLY_TRUE(isEqual(transceiverIndx0, transceiverIndx1));
      }
    })
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(LinkTest, trafficRxTx) {
  auto verify = [this]() {
    WITH_RETRIES({
      sw()->getLldpMgr()->sendLldpOnAllPorts();
      EXPECT_EVENTUALLY_TRUE(checkReachabilityOnAllCabledPorts());
    });
  };

  verifyAcrossWarmBoots([]() {}, verify);
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
                sw()->getHw_DEPRECATED(),
                {folly::IPAddress("::"), 0},
                RouterID(0),
                ecmpSizeInSw),
            ecmpSizeInSw);
        // Assert all cabled transceivers have ACTIVE state
        EXPECT_NO_THROW(waitForAllCabledPorts(true));
        EXPECT_NO_THROW(waitForAllTransceiverStates(true));
      });
}

TEST_F(LinkTest, qsfpWarmbootIsHitLess) {
  // Create a L3 data plane flood and then warmboot qsfp_service. Then assert
  // that none of the traffic bearing ports loss traffic.
  verifyAcrossWarmBoots(
      [this]() {
        createL3DataplaneFlood();
        restartQsfpService(false /* coldboot */);
        // Wait for all transceivers to converge to Active state
        EXPECT_NO_THROW(waitForAllTransceiverStates(
            true, 60 /* retries */, 5s /* retry interval */));
      },
      [this]() {
        // Assert no traffic loss and no ecmp shrink. If ports flap
        // these conditions will not be true
        assertNoInDiscards();
        auto ecmpSizeInSw = getVlanOwningCabledPorts().size();
        EXPECT_EQ(
            utility::getEcmpSizeInHw(
                sw()->getHw_DEPRECATED(),
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
    auto switchSettings =
        util::getFirstNodeIf(newState->getSwitchSettings())->modify(&newState);
    switchSettings->setPtpTcEnable(false);
    return newState;
  });

  createL3DataplaneFlood();
  assertNoInDiscards();

  sw()->updateStateBlocking("ptp enable", [](auto state) {
    auto newState = state->clone();
    auto switchSettings =
        util::getFirstNodeIf(newState->getSwitchSettings())->modify(&newState);
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
          sw()->getHw_DEPRECATED(),
          {folly::IPAddress("::"), 0},
          RouterID(0),
          ecmpSizeInSw),
      ecmpSizeInSw);
}

TEST_F(LinkTest, opticsTxDisableEnable) {
  auto [opticalPorts, opticalPortNames] = getOpticalCabledPortsAndNames();
  EXPECT_FALSE(opticalPorts.empty())
      << "opticsTxDisableEnable: Did not detect any optical transceivers";

  if (!opticalPorts.empty()) {
    opticalPortNames = "wedge_qsfp_util " + opticalPortNames;
    const std::string txDisableCmd = opticalPortNames + "--tx-disable";

    XLOG(DBG2) << "opticsTxDisableEnable: About to execute cmd: "
               << txDisableCmd;
    // TODO(ccpowers): Doesn't seem like there's a way for us to make this
    // command a literal, since it depends on the cabling. We may want to just
    // make a qsfp-service API for this rather than using a shell cmd
    // @lint-ignore CLANGTIDY
    folly::Subprocess(txDisableCmd).waitChecked();
    XLOG(DBG2) << fmt::format(
        "opticsTxDisableEnable: cmd {:s} finished. Awaiting links to go down...",
        txDisableCmd);
    EXPECT_NO_THROW(waitForLinkStatus(opticalPorts, false));
    XLOG(DBG2) << "opticsTxDisableEnable: link Tx disabled";

    const std::string txEnableCmd = opticalPortNames + "--tx-enable";
    XLOG(DBG2) << "opticsTxDisableEnable: About to execute cmd: "
               << txEnableCmd;
    // @lint-ignore CLANGTIDY
    folly::Subprocess(txEnableCmd).waitChecked();
    XLOG(DBG2) << fmt::format(
        "opticsTxDisableEnable: cmd {:s} finished. Awaiting links to go up...",
        txEnableCmd);
    EXPECT_NO_THROW(waitForLinkStatus(opticalPorts, true));
    XLOG(DBG2) << "opticsTxDisableEnable: links are up";
  }
}

// Tests that when link goes down then remediation is triggered
TEST_F(LinkTest, testOpticsRemediation) {
  auto verify = [this]() {
    std::vector<int32_t> transceiverIds;
    auto [ports, opticalPortNames] = getOpticalCabledPortsAndNames(true);
    // Bring down the link on all the optical cabled ports. The link should go
    // down and the remediation should get triggered
    EXPECT_GT(ports.size(), 0);

    const std::string txDisableCmd =
        "wedge_qsfp_util " + opticalPortNames + " --tx-disable";
    // @lint-ignore CLANGTIDY
    folly::Subprocess(txDisableCmd).waitChecked();

    for (const auto& port : ports) {
      auto tcvrId =
          platform()->getPlatformPort(port)->getTransceiverID().value();
      transceiverIds.push_back(tcvrId);
    }
    XLOG(DBG2) << "Wait for all ports to be down " << opticalPortNames;
    EXPECT_NO_THROW(waitForLinkStatus(ports, false));

    // Check the module's remediation counter has increased.
    // Exclude the Miniphoton ports
    XLOG(DBG2) << "Check for Transceiver Info from qsfp_service now";

    WITH_RETRIES_N_TIMED(5, std::chrono::seconds(60), {
      auto transceiverInfos = waitForTransceiverInfo(transceiverIds);
      for (const auto& port : ports) {
        auto tcvrId =
            platform()->getPlatformPort(port)->getTransceiverID().value();
        auto txInfoItr = transceiverInfos.find(tcvrId);
        if ((txInfoItr != transceiverInfos.end()) &&
            (txInfoItr->second.tcvrState().has_value() &&
             txInfoItr->second.tcvrState()->identifier().has_value() &&
             txInfoItr->second.tcvrState()->identifier().value() !=
                 TransceiverModuleIdentifier::MINIPHOTON_OBO) &&
            (txInfoItr->second.tcvrStats().has_value() &&
             txInfoItr->second.tcvrStats()->remediationCounter().has_value())) {
          XLOG(DBG2)
              << "Tcvr Id " << tcvrId << " remediation counter "
              << txInfoItr->second.tcvrStats()->remediationCounter().value();
          EXPECT_EVENTUALLY_GT(
              txInfoItr->second.tcvrStats()->remediationCounter().value(), 0);
        }
      }
    });

    XLOG(DBG2) << "Wait for all ports to come up " << opticalPortNames;
    EXPECT_NO_THROW(waitForLinkStatus(ports, true, 60, 5s));
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(LinkTest, qsfpColdbootAfterAgentUp) {
  // Verifies that a qsfp cold boot after agent is up can still bringup the
  // links and there is no dependency on which service starts first
  verifyAcrossWarmBoots(
      []() {},
      [this]() {
        restartQsfpService(true /* coldboot */);
        /* sleep override */
        sleep(5);
        // Assert all cabled ports are up and transceivers have ACTIVE state
        EXPECT_NO_THROW(waitForAllCabledPorts(true));
        EXPECT_NO_THROW(waitForAllTransceiverStates(true, 60, 5s));
      });
}
