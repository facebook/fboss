// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include "fboss/agent/ResolvedNexthopMonitor.h"
#include "fboss/agent/ResolvedNexthopProbeScheduler.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

namespace {
using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;

const auto kPrefixV4 = RoutePrefixV4{IPAddressV4("10.0.10.0"), 24};
const std::array<IPAddressV4, 2> kNexthopsV4{IPAddressV4("10.0.10.1"),
                                             IPAddressV4("10.0.10.2")};
const auto kPrefixV6 = RoutePrefixV6{IPAddressV6("10:100::"), 64};
const std::array<IPAddressV6, 2> kNexthopsV6{IPAddressV6("10:100::1"),
                                             IPAddressV6("10:100::1")};
} // namespace

namespace facebook {
namespace fboss {

class ResolvedNexthopMonitorTest : public ::testing::Test {
 public:
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using Func = folly::Function<void()>;
  void SetUp() override {
    handle_ = createTestHandle(testStateA());
    sw_ = handle_->getSw();
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> addRoute(
      const std::shared_ptr<SwitchState>& state,
      const RoutePrefix<AddrT>& prefix,
      RouteNextHopSet nexthops,
      ClientID client = ClientID::OPENR) {
    auto routeTables = state->getRouteTables();
    RouteUpdater updater(routeTables);
    updater.addRoute(
        RouterID(0),
        prefix.network,
        prefix.mask,
        client,
        RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));
    auto newRouteTables = updater.updateDone();
    newRouteTables->publish();
    auto newState = state->isPublished() ? state->clone() : state;
    newState->resetRouteTables(newRouteTables);
    return newState;
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> delRoute(
      const std::shared_ptr<SwitchState>& state,
      const RoutePrefix<AddrT>& prefix,
      ClientID client = ClientID::OPENR) {
    auto routeTables = state->getRouteTables();
    RouteUpdater updater(routeTables);
    updater.delRoute(RouterID(0), prefix.network, prefix.mask, client);
    auto newRouteTables = updater.updateDone();
    newRouteTables->publish();
    auto newState = state->isPublished() ? state->clone() : state;
    newState->resetRouteTables(newRouteTables);
    return newState;
  }

  std::shared_ptr<SwitchState> toggleIgnoredStateUpdate(
      const std::shared_ptr<SwitchState>& state) {
    auto newState = state->isPublished() ? state->clone() : state;
    auto mirrors = newState->getMirrors()->clone();
    auto mirror = newState->getMirrors()->getMirrorIf("mirror");
    if (!mirror) {
      mirror = std::make_shared<Mirror>("mirror", PortID(5), std::nullopt);
      mirrors->addNode(mirror);
    } else {
      mirrors->removeNode(mirror);
    }
    newState->resetMirrors(mirrors);
    return newState;
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

TEST_F(ResolvedNexthopMonitorTest, AddUnresolvedRoutes) {
  updateState(
      "unresolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1),
                              UnresolvedNextHop(kNexthopsV4[1], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_FALSE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, RemoveUnresolvedRoutes) {
  updateState(
      "unresolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1),
                              UnresolvedNextHop(kNexthopsV4[1], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  updateState(
      "unresolved route removed",
      [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, kPrefixV4);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_FALSE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, AddResolvedRoutes) {
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            UnresolvedNextHop(folly::IPAddressV4("10.0.0.22"), 1),
            UnresolvedNextHop(folly::IPAddressV4("10.0.55.22"), 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_TRUE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, RemoveResolvedRoutes) {
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            UnresolvedNextHop(folly::IPAddressV4("10.0.0.22"), 1),
            UnresolvedNextHop(folly::IPAddressV4("10.0.55.22"), 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  updateState(
      "resolved route removed", [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, kPrefixV4);
      });
  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_TRUE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, ChangeUnresolvedRoutes) {
  updateState(
      "unresolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1),
                              UnresolvedNextHop(kNexthopsV4[1], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  updateState(
      "unresolved route change",
      [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_FALSE(monitor->probesScheduled());
}

} // namespace fboss
} // namespace facebook
