// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include <optional>
#include <thread>
#include <utility>

#include <folly/logging/xlog.h>

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

namespace {
// The BRCM SDK only accepts a fixed set of intervals -- 0/50/75/100/150/250/
// 500ms, plus 10ms on TH5/TH6 since the FLR 10ms timeout support. 500ms is the
// longest, which keeps the hold well clear of the measurement noise floor: the
// baseline down-detect latency alone is ~250-300ms and swings by tens of ms
// run to run, so shorter holds are not resolvable by subtracting a baseline.
constexpr int32_t kHoldoffMs = 500;
constexpr double kTolerance = 0.10;
} // namespace

class AgentEnsembleLinkDebounceTest : public AgentEnsembleLinkTest {
  std::vector<link_test_production_features::LinkTestProductionFeature>
  getProductionFeatures() const override {
    return {
        link_test_production_features::LinkTestProductionFeature::L1_LINK_TEST};
  }

 protected:
  // Pick a cabled interface-port pair. The first port is the one under test;
  // the second is its neighbor, whose transmitter is squelched to drive the
  // link of the first. Returns nullopt if the setup has no such pair.
  std::optional<std::pair<PortID, PortID>> getCabledInterfacePortPair() const {
    const auto state = getProgrammedState();
    for (const auto& pair : getConnectedPairs()) {
      if (state->getPorts()->getNodeIf(pair.first)->getPortType() ==
              cfg::PortType::INTERFACE_PORT &&
          state->getPorts()->getNodeIf(pair.second)->getPortType() ==
              cfg::PortType::INTERFACE_PORT) {
        return std::make_pair(pair.first, pair.second);
      }
    }
    return std::nullopt;
  }

  // Apply a config that overrides the link-down debounce hold timer for
  // `port`; std::nullopt leaves the SDK default (no debounce) in place.
  //
  // Hardware linkscan is set on the same port in the same config on purpose.
  // The BRCM SDK rejects SAI_PORT_ATTR_LINK_DOWN_DEBOUNCE_TIMEOUT outright
  // unless the port is already in SAI_PORT_LINKSCAN_MODE_HW, and LinkScanMode
  // sits ahead of LinkDownDebouncePeriodMs in SaiPortTraits::CreateAttributes,
  // which is the order both the create list and checkAndSetAttribute follow.
  void applyDownHoldoff(PortID port, std::optional<int32_t> downMs) {
    auto config = getSw()->getConfig();
    auto portCfg = utility::findCfgPort(config, port);
    portCfg->linkScanMode() = cfg::LinkScanMode::HARDWARE;
    if (downMs.has_value()) {
      portCfg->portDownHoldoffTimeMs() = *downMs;
    } else {
      portCfg->portDownHoldoffTimeMs().reset();
    }
    applyNewConfig(config);
  }

  // Bring `port`'s link down/up by squelching its transceiver optical TX via
  // wedge_qsfp_util, rather than admin-disabling the port. A genuine optical
  // loss-of-signal makes the link partner observe a real link down, which
  // exercises the SDK link-down debounce hold timer; an admin disable instead
  // reprograms the observed port and puts tens of ms of SAI writes inside the
  // measurement window. squelch=true disables TX (link down), squelch=false
  // re-enables TX (link up).
  void setPortTxSquelched(PortID port, bool squelch) const {
    const auto cmd = std::string("wedge_qsfp_util ") + getPortName(port) +
        (squelch ? " --tx_disable" : " --tx_enable");
    XLOG(INFO) << "setPortTxSquelched: " << (squelch ? "disable" : "enable")
               << " TX on port=" << static_cast<int>(port) << " via: " << cmd;
    runShellCommand(cmd);
  }
};

// Verify the SDK link-down debounce (hold) timer using a real physical link
// flap. Configure a down-holdoff on portA -- which FBOSS programs as the
// per-port SAI_PORT_ATTR_LINK_DOWN_DEBOUNCE_TIMEOUT extension, wrapped as
// SaiPortTraits::Attributes::LinkDownDebouncePeriodMs -- then squelch the
// transmitter of its cabled neighbor portB so portA observes a genuine loss of
// signal. portA should report oper-down only after roughly the configured
// holdoff.
//
// Only the down direction is covered: Broadcom has no per-port link-up
// debounce, and the switch-global one it does have is not wired here.
TEST_F(AgentEnsembleLinkDebounceTest, DebounceTimerWithinTolerance) {
  addVerifiedProductionFeatures(
      {link_test_production_features::LinkTestProductionFeature::
           TRANSCEIVER_TX_DISABLE});

  // Pick a cabled interface-port pair. portA is observed; its neighbor portB is
  // toggled to drive the link down/up.
  const auto portPair = getCabledInterfacePortPair();
  ASSERT_TRUE(portPair.has_value())
      << "No cabled interface-port pair available for debounce test";
  const auto portA = portPair->first;
  const auto portB = portPair->second;
  addTestedPorts({portA, portB});

  // Disable neighbor portB and time how long portA takes to report oper-down.
  // Poll portA's oper state tightly; waitForLinkStatus's 1s granularity is too
  // coarse for a 500ms +/- 10% holdoff. Leaves portB (and thus portA) down.
  auto measureDownLatency = [&]() {
    XLOG(INFO) << "measureDownLatency: portA=" << static_cast<int>(portA)
               << " portB=" << static_cast<int>(portB);
    EXPECT_NO_THROW(waitForLinkStatus(
        {portA, portB}, true, 60, std::chrono::milliseconds(1000)));
    const auto start = std::chrono::steady_clock::now();
    setPortTxSquelched(portB, true);
    constexpr auto kMaxWait = std::chrono::seconds(5);
    while (getProgrammedState()->getPorts()->getNodeIf(portA)->isUp()) {
      if (std::chrono::steady_clock::now() - start > kMaxWait) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
  };

  // Unsquelch neighbor portB and wait for portA to come back up, so the next
  // measurement starts from a known oper-up state. Not itself a measurement:
  // optical bring-up takes 1.5-3s and swings by hundreds of ms.
  auto restoreLink = [&]() {
    setPortTxSquelched(portB, false);
    EXPECT_NO_THROW(waitForLinkStatus(
        {portA, portB}, true, 60, std::chrono::milliseconds(1000)));
  };

  auto verify = [&]() {
    // Baseline (no debounce) isolates the fixed propagation/notification
    // latency so it can be subtracted from the debounced measurement.
    XLOG(INFO) << "Measuring baseline down latency";
    applyDownHoldoff(portA, std::nullopt);
    const auto baselineDownMs = measureDownLatency();
    restoreLink();

    XLOG(INFO) << "Measuring debounced down latency";
    applyDownHoldoff(portA, kHoldoffMs);
    // Programming the debounce re-asserts HW linkscan on portA, which can
    // bounce a live link. Such a flap is invisible for up to the holdoff --
    // hiding it is precisely what the debounce does -- so the measurement
    // would otherwise start against a port that is already physically down.
    // Wait the hold out before timing anything.
    std::this_thread::sleep_for(
        std::chrono::milliseconds(2 * kHoldoffMs + 1000));
    const auto downMs = measureDownLatency();
    restoreLink();

    XLOG(INFO) << "Link down latency baseline=" << baselineDownMs.count()
               << "ms debounce(" << kHoldoffMs << "ms)=" << downMs.count()
               << "ms";

    const auto loBound = static_cast<int64_t>(kHoldoffMs * (1 - kTolerance));
    const auto hiBound = static_cast<int64_t>(kHoldoffMs * (1 + kTolerance));
    const auto downHoldoff = (downMs - baselineDownMs).count();
    EXPECT_GE(downHoldoff, loBound)
        << "down holdoff " << downHoldoff << "ms below " << loBound
        << "ms (configured " << kHoldoffMs << "ms)";
    EXPECT_LE(downHoldoff, hiBound)
        << "down holdoff " << downHoldoff << "ms above " << hiBound
        << "ms (configured " << kHoldoffMs << "ms)";

    // Leave the port undebounced for whatever runs next.
    applyDownHoldoff(portA, std::nullopt);
  };

  verifyAcrossWarmBoots([]() {}, verify);
}
