// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include <algorithm>
#include <chrono>
#include <thread>

namespace facebook::fboss {

constexpr double kTolerance = 0.10;
constexpr int32_t kHoldoffLongMs = 15000;
constexpr auto kFlapWithinWindow = std::chrono::milliseconds(1000);
// Fixed wait after egress port loopback goes to NONE, before sending traffic.
constexpr auto kPostLoopbackNoneSettleDelay = std::chrono::seconds(1);
// End the post-burst holdoff check slightly before the timer expires.
constexpr auto kHoldoffCheckSlack = std::chrono::milliseconds(1000);

class AgentHwLinkDebounceTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::PORT_DEBOUNCE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  PortID portForTest() const {
    return getAgentEnsemble()->masterLogicalInterfacePortIds(
        getCurrentSwitchIdForTesting())[0];
  }

  // Inject port: stays up in MAC loopback and sources test traffic.
  // Target (egress) port: loopback set to NONE; down-debounce delays
  // isUp=false.
  PortID portInjectForTest() const {
    return portForTest();
  }
  PortID portTargetForTest() const {
    return getAgentEnsemble()->masterLogicalInterfacePortIds(
        getCurrentSwitchIdForTesting())[1];
  }

  void applyDebounceConfig(
      std::optional<int32_t> upMs,
      std::optional<int32_t> downMs) {
    applyDebounceConfigForPort(portForTest(), upMs, downMs);
  }

  void applyDebounceConfigForPort(
      PortID port,
      std::optional<int32_t> upMs,
      std::optional<int32_t> downMs) {
    auto config = initialConfig(*getAgentEnsemble());
    auto portCfg = utility::findCfgPort(config, port);
    if (upMs.has_value()) {
      portCfg->portUpHoldoffTimeMs() = *upMs;
    } else {
      portCfg->portUpHoldoffTimeMs().reset();
    }
    if (downMs.has_value()) {
      portCfg->portDownHoldoffTimeMs() = *downMs;
    } else {
      portCfg->portDownHoldoffTimeMs().reset();
    }
    applyNewConfig(config);
  }

  // Program an IPv6 route via the target (egress) port for the packet burst.
  RoutePrefixV6 programV6RouteToPort(PortID targetPort) {
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    boost::container::flat_set<PortDescriptor> nhops{
        PortDescriptor(targetPort)};
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, nhops);
    });
    RoutePrefixV6 prefix{folly::IPAddressV6("2001:db8:dead:beef::"), 64};
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, nhops, {prefix});
    return prefix;
  }

  void unprogramV6RouteToPort(const RoutePrefixV6& prefix) {
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.unprogramRoutes(&wrapper, {prefix});
  }

  std::chrono::milliseconds measureBringUpDownLatency(bool up) {
    auto port = portForTest();
    auto start = std::chrono::steady_clock::now();
    if (up) {
      bringUpPort(port);
    } else {
      bringDownPort(port);
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  }

  // Change loopback mode without waiting for link-state debounce to complete.
  void togglePortNoWait(PortID port, bool toUp) {
    auto asicTable = getAgentEnsemble()->getHwAsicTable();
    auto switchId = scopeResolver().scope(port).switchId();
    auto asic = asicTable->getHwAsic(switchId);
    auto desiredMode = toUp
        ? asic->getDesiredLoopbackMode(
              getProgrammedState()->getPorts()->getNodeIf(port)->getPortType())
        : cfg::PortLoopbackMode::NONE;
    applyNewState([port, desiredMode](const std::shared_ptr<SwitchState>& in) {
      auto newState = in->clone();
      auto newPort = newState->getPorts()->getNodeIf(port)->modify(&newState);
      newPort->setLoopbackMode(desiredMode);
      return newState;
    });
  }

  // Confirm loopback NONE is programmed, then wait for the ASIC to stabilize
  // before injecting traffic toward the egress port.
  void waitForLoopbackNoneSettle(PortID port) {
    auto settleStart = std::chrono::steady_clock::now();
    WITH_RETRIES({
      EXPECT_EQ(
          getProgrammedState()->getPorts()->getNodeIf(port)->getLoopbackMode(),
          cfg::PortLoopbackMode::NONE)
          << "port " << port << " loopback did not reach NONE";
    });
    auto pollElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - settleStart)
                             .count();
    XLOG(INFO) << "port " << port << " loopback NONE observed after "
               << pollElapsedMs << " ms; sleeping "
               << kPostLoopbackNoneSettleDelay.count() << "s before burst";
    std::this_thread::sleep_for(kPostLoopbackNoneSettleDelay);
    auto totalElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - settleStart)
                              .count();
    XLOG(INFO) << "port " << port
               << " post-NONE settle complete, total wait=" << totalElapsedMs
               << " ms";
  }

  int64_t getLinkStateFlapCount(PortID port) const {
    auto name = getProgrammedState()->getPorts()->getNodeIf(port)->getName();
    return fb303::fbData->getCounterIfExists(name + ".link_state.flap.sum")
        .value_or(0);
  }
};

TEST_F(AgentHwLinkDebounceTest, DebounceTimerWithinTolerance) {
  auto port = portForTest();
  auto setup = [&]() { bringUpPort(port); };
  auto verify = [&]() {
    // Measure no-debounce baseline, then verify configured holdoff timers.
    applyDebounceConfig(std::nullopt, std::nullopt);
    auto baselineDownMs = measureBringUpDownLatency(false);
    auto baselineUpMs = measureBringUpDownLatency(true);
    XLOG(INFO) << "Baseline (no debounce) down=" << baselineDownMs.count()
               << "ms up=" << baselineUpMs.count() << "ms";

    auto verifyHoldoff = [&](int32_t holdoffMs) {
      applyDebounceConfig(holdoffMs, holdoffMs);
      auto downMs = measureBringUpDownLatency(false);
      auto upMs = measureBringUpDownLatency(true);
      XLOG(INFO) << "Debounce(" << holdoffMs << "ms) down=" << downMs.count()
                 << "ms up=" << upMs.count() << "ms";

      auto downHoldoff = (downMs - baselineDownMs).count();
      auto upHoldoff = (upMs - baselineUpMs).count();
      auto loBound = static_cast<int64_t>(holdoffMs * (1 - kTolerance));
      auto hiBound = static_cast<int64_t>(holdoffMs * (1 + kTolerance));

      EXPECT_GE(downHoldoff, loBound)
          << "down holdoff " << downHoldoff << "ms below " << loBound
          << "ms (configured " << holdoffMs << "ms)";
      EXPECT_LE(downHoldoff, hiBound)
          << "down holdoff " << downHoldoff << "ms above " << hiBound
          << "ms (configured " << holdoffMs << "ms)";
      EXPECT_GE(upHoldoff, loBound)
          << "up holdoff " << upHoldoff << "ms below " << loBound
          << "ms (configured " << holdoffMs << "ms)";
      EXPECT_LE(upHoldoff, hiBound)
          << "up holdoff " << upHoldoff << "ms above " << hiBound
          << "ms (configured " << holdoffMs << "ms)";
    };

    verifyHoldoff(kHoldoffLongMs);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwLinkDebounceTest, FlapWithinDebounceWindow) {
  auto port = portForTest();

  auto verifyNoEventInWindow = [&](bool portStartsAsUp, int32_t holdoffMs) {
    auto preFlap = getLinkStateFlapCount(port);
    bool needsTransition =
        getProgrammedState()->getPorts()->getNodeIf(port)->isUp() !=
        portStartsAsUp;

    if (portStartsAsUp) {
      bringUpPort(port);
    } else {
      bringDownPort(port);
    }
    EXPECT_EQ(
        getProgrammedState()->getPorts()->getNodeIf(port)->isUp(),
        portStartsAsUp);

    // Wait for flap counter to reflect the initial port transition.
    auto baselineFlaps = preFlap + (needsTransition ? 1 : 0);
    WITH_RETRIES(
        { EXPECT_EVENTUALLY_EQ(getLinkStateFlapCount(port), baselineFlaps); });

    auto noFlapsSinceBaseline = [&]() -> bool {
      return getLinkStateFlapCount(port) == baselineFlaps;
    };

    togglePortNoWait(port, !portStartsAsUp);
    CHECK_HOLDS_FOR_DURATION(kFlapWithinWindow, noFlapsSinceBaseline);
    togglePortNoWait(port, portStartsAsUp);

    auto verifyWindow =
        std::chrono::milliseconds(holdoffMs) - kFlapWithinWindow;
    CHECK_HOLDS_FOR_DURATION(verifyWindow, noFlapsSinceBaseline);
    EXPECT_EQ(
        getProgrammedState()->getPorts()->getNodeIf(port)->isUp(),
        portStartsAsUp)
        << "Port should still be in initial state after suppressed flap";
  };

  applyDebounceConfig(0, kHoldoffLongMs);
  verifyNoEventInWindow(true /* portStartsAsUp */, kHoldoffLongMs);

  applyDebounceConfig(kHoldoffLongMs, 0);
  verifyNoEventInWindow(false /* portStartsAsUp */, kHoldoffLongMs);
}

// Two-port test: inject on one port, route to a second port with down-debounce.
// Egress loopback is set to NONE; packets should drop while isUp is still true.
TEST_F(AgentHwLinkDebounceTest, PacketDropDuringDownHoldoff) {
  constexpr int kNumPackets = 100;

  if (getAgentEnsemble()
          ->masterLogicalInterfacePortIds(getCurrentSwitchIdForTesting())
          .size() < 2) {
    GTEST_SKIP() << "Test requires at least 2 interface ports";
  }
  auto portInject = portInjectForTest();
  auto portTarget = portTargetForTest();
  RoutePrefixV6 routedPrefix{folly::IPAddressV6("2001:db8:dead:beef::"), 64};
  auto setup = [&]() {
    bringUpPort(portInject);
    bringUpPort(portTarget);
    routedPrefix = programV6RouteToPort(portTarget);
  };
  auto verify = [&]() {
    // Long down-debounce on the egress port only; inject port keeps defaults.
    applyDebounceConfigForPort(portTarget, std::nullopt, kHoldoffLongMs);

    ASSERT_TRUE(
        getProgrammedState()->getPorts()->getNodeIf(portInject)->isUp());
    ASSERT_TRUE(
        getProgrammedState()->getPorts()->getNodeIf(portTarget)->isUp());

    auto baselineTargetFlaps = getLinkStateFlapCount(portTarget);
    auto injectStatsBefore = getLatestPortStats(portInject);
    auto targetStatsBefore = getLatestPortStats(portTarget);
    auto injectInBefore = *injectStatsBefore.inDiscards_();
    auto injectOutBefore = *injectStatsBefore.outDiscards_();
    auto targetInBefore = *targetStatsBefore.inDiscards_();
    auto targetOutBefore = *targetStatsBefore.outDiscards_();
    XLOG(INFO) << "PacketDropDuringDownHoldoff baseline"
               << " portInject=" << portInject << " in=" << injectInBefore
               << " out=" << injectOutBefore << " portTarget=" << portTarget
               << " in=" << targetInBefore << " out=" << targetOutBefore
               << " kNumPackets=" << kNumPackets
               << " holdoff_ms=" << kHoldoffLongMs;

    auto holdoffStart = std::chrono::steady_clock::now();
    // Program egress loopback to NONE (link-down stimulus); inject port
    // unchanged.
    togglePortNoWait(portTarget, false /* toUp */);
    waitForLoopbackNoneSettle(portTarget);

    // Routed IPv6 burst: inject on portInject, egress via portTarget.
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

    auto burstStart = std::chrono::steady_clock::now();
    for (int i = 1; i <= kNumPackets; ++i) {
      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          std::nullopt /* vlan */,
          srcMac,
          intfMac,
          folly::IPAddress("1001::1"),
          folly::IPAddress("2001:db8:dead:beef::1"),
          4242,
          4242);
      getSw()->sendPacketOutOfPortAsync(std::move(pkt), portInject);
    }
    auto burstElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - burstStart)
                              .count();
    XLOG(INFO) << "Burst of " << kNumPackets << " routed packets sent in "
               << burstElapsedMs << " ms (target holdoff=" << kHoldoffLongMs
               << " ms)";

    auto elapsed = std::chrono::steady_clock::now() - holdoffStart;
    auto remainingHoldoff = std::chrono::milliseconds(kHoldoffLongMs) - elapsed;
    auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    auto remainingMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(remainingHoldoff)
            .count();
    XLOG(INFO)
        << "Post-burst holdoff check: elapsed=" << elapsedMs
        << "ms remaining=" << remainingMs
        << "ms baselineFlaps=" << baselineTargetFlaps
        << " currentFlaps=" << getLinkStateFlapCount(portTarget) << " isUp="
        << getProgrammedState()->getPorts()->getNodeIf(portTarget)->isUp();

    // Loopback is already NONE; down-debounce keeps reported link state (isUp)
    // true until the holdoff timer expires.
    EXPECT_TRUE(getProgrammedState()->getPorts()->getNodeIf(portTarget)->isUp())
        << "portTarget should still be UP immediately after burst";
    EXPECT_EQ(getLinkStateFlapCount(portTarget), baselineTargetFlaps)
        << "Unexpected flap on portTarget immediately after burst";

    auto verifyWindow = remainingHoldoff - kHoldoffCheckSlack;
    if (verifyWindow > std::chrono::milliseconds(0)) {
      // isUp should stay true for the rest of the debounce window (not
      // loopback).
      CHECK_HOLDS_FOR_DURATION(verifyWindow, [&]() -> bool {
        return getProgrammedState()
                   ->getPorts()
                   ->getNodeIf(portTarget)
                   ->isUp() &&
            getLinkStateFlapCount(portTarget) == baselineTargetFlaps;
      });
    } else {
      XLOG(WARNING) << "Skipping CHECK_HOLDS_FOR_DURATION: remaining holdoff "
                    << remainingMs << "ms <= slack "
                    << kHoldoffCheckSlack.count() << "ms";
    }

    WITH_RETRIES({
      EXPECT_EVENTUALLY_FALSE(
          getProgrammedState()->getPorts()->getNodeIf(portTarget)->isUp());
    });

    // After debounce expires, verify discard counters increased on either port.
    WITH_RETRIES({
      auto injectStatsAfter = getLatestPortStats(portInject);
      auto targetStatsAfter = getLatestPortStats(portTarget);
      auto injectInDelta = *injectStatsAfter.inDiscards_() - injectInBefore;
      auto injectOutDelta = *injectStatsAfter.outDiscards_() - injectOutBefore;
      auto targetInDelta = *targetStatsAfter.inDiscards_() - targetInBefore;
      auto targetOutDelta = *targetStatsAfter.outDiscards_() - targetOutBefore;
      XLOG(INFO) << "Final deltas (portTarget DOWN)"
                 << " inject: in=" << injectInDelta << " out=" << injectOutDelta
                 << " target: in=" << targetInDelta << " out=" << targetOutDelta
                 << " expected>=" << kNumPackets;
      EXPECT_EVENTUALLY_GE(
          std::max(
              {injectInDelta, injectOutDelta, targetInDelta, targetOutDelta}),
          static_cast<int64_t>(kNumPackets))
          << "Expected in_discards or out_discards on inject or target port"
          << " to accumulate >= " << kNumPackets << " packets";
    });

    // Restore egress port loopback after the test.
    togglePortNoWait(portTarget, true /* toUp */);

    unprogramV6RouteToPort(routedPrefix);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
