// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include <chrono>
#include <thread>

namespace facebook::fboss {

constexpr double kTolerance = 0.10;
constexpr int32_t kHoldoffLongMs = 15000;
constexpr auto kFlapWithinWindow = std::chrono::milliseconds(1000);
// After driving loopback to NONE the SDK needs time to apply TX squelch before
// we inject the test packet.
constexpr auto kPostLoopbackNoneSettleDelay = std::chrono::seconds(1);
// End CHECK_HOLDS slightly before the configured down-holdoff expires.
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

  // Apply a config that overrides the debounce values for the test port.
  // Either argument may be std::nullopt to leave the SDK default in place.
  void applyDebounceConfig(
      std::optional<int32_t> upMs,
      std::optional<int32_t> downMs) {
    auto config = initialConfig(*getAgentEnsemble());
    auto portCfg = utility::findCfgPort(config, portForTest());
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

  // Time how long it takes for the test port to reach `up` after triggering
  // a flap via bringUpPort/bringDownPort. Both calls block until the
  // corresponding state change is observed, so the elapsed wall-clock time
  // reflects the SDK debounce hold timer + baseline FBOSS overhead.
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

  // bringUp/DownPort go through linkStateToggler which waits for the port
  // status to be updated; this helper flips loopback mode without waiting so
  // we can flap inside the SDK debounce window.
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

  // Poll until loopback NONE is programmed, then wait for SDK squelch settle.
  void waitForLoopbackNoneSettle(PortID port) {
    WITH_RETRIES({
      EXPECT_EQ(
          getProgrammedState()->getPorts()->getNodeIf(port)->getLoopbackMode(),
          cfg::PortLoopbackMode::NONE)
          << "port " << port << " loopback did not reach NONE";
    });
    XLOG(INFO) << "port " << port << " loopback NONE; sleeping "
               << kPostLoopbackNoneSettleDelay.count()
               << "s for SDK squelch settle";
    std::this_thread::sleep_for(kPostLoopbackNoneSettleDelay);
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
    // First measure the no-debounce baseline so we can subtract it from each
    // holdoff measurement and isolate the SDK's hold-timer contribution.
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

    // Wait for fb303 publish to catch up to the expected flap count from the
    // initial transition. bringUp/DownPort produces one flap event per call.
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

TEST_F(AgentHwLinkDebounceTest, PacketDropDuringDownHoldoff) {
  auto port = portForTest();
  auto setup = [&]() { bringUpPort(port); };
  auto verify = [&]() {
    applyDebounceConfig(std::nullopt, kHoldoffLongMs);

    ASSERT_TRUE(getProgrammedState()->getPorts()->getNodeIf(port)->isUp());
    auto baselineFlaps = getLinkStateFlapCount(port);
    auto statsBefore = getLatestPortStats(port);
    auto outBefore = *statsBefore.outDiscards_();

    auto holdoffStart = std::chrono::steady_clock::now();
    togglePortNoWait(port, false /* toUp */);
    waitForLoopbackNoneSettle(port);

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto pkt = utility::makeUDPTxPacket(
        getSw(),
        std::nullopt /* vlan */,
        srcMac,
        intfMac,
        folly::IPAddress("1001::1"),
        folly::IPAddress("2001::1"),
        4242,
        4242);
    getSw()->sendPacketOutOfPortAsync(std::move(pkt), port);

    auto elapsed = std::chrono::steady_clock::now() - holdoffStart;
    auto remainingHoldoff = std::chrono::milliseconds(kHoldoffLongMs) - elapsed;
    EXPECT_TRUE(getProgrammedState()->getPorts()->getNodeIf(port)->isUp())
        << "port should still be UP immediately after packet inject";
    EXPECT_EQ(getLinkStateFlapCount(port), baselineFlaps)
        << "Unexpected flap immediately after packet inject";

    auto verifyWindow = remainingHoldoff - kHoldoffCheckSlack;
    if (verifyWindow > std::chrono::milliseconds(0)) {
      CHECK_HOLDS_FOR_DURATION(verifyWindow, [&]() -> bool {
        return getProgrammedState()->getPorts()->getNodeIf(port)->isUp() &&
            getLinkStateFlapCount(port) == baselineFlaps;
      });
    }

    WITH_RETRIES({
      EXPECT_EVENTUALLY_FALSE(
          getProgrammedState()->getPorts()->getNodeIf(port)->isUp());
    });

    WITH_RETRIES({
      auto statsAfter = getLatestPortStats(port);
      auto outAfter = *statsAfter.outDiscards_();
      XLOG(INFO) << "Port outDiscards before/after: " << outBefore << "/"
                 << outAfter;
      EXPECT_EVENTUALLY_EQ(outAfter, outBefore + 1);
    });

    togglePortNoWait(port, true /* toUp */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
