/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/RouteUpdateLogger.h"
#include <folly/IPAddress.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace facebook::fboss;

namespace {

template <typename AddrT>
class MockRouteLogger : public RouteLogger<AddrT> {
 public:
  void logAddedRoute(
      const std::shared_ptr<Route<AddrT>>& newRoute,
      const std::vector<std::string>& /*identifiers*/) override {
    added.push_back(newRoute->str());
  }

  void logChangedRoute(
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& newRoute,
      const std::vector<std::string>& /*identifiers*/) override {
    changed.push_back({oldRoute->str(), newRoute->str()});
  }

  void logRemovedRoute(
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::vector<std::string>& /*identifiers*/) override {
    removed.push_back(oldRoute->str());
  }

  std::vector<std::string> added;
  std::vector<std::string> removed;
  std::vector<std::pair<std::string, std::string>> changed;
};

class MockMplsRouteLogger : public MplsRouteLogger {
 public:
  void logAddedRoute(
      const std::shared_ptr<LabelForwardingEntry>& newEntry,
      const std::vector<std::string>& identifiers) override {
    added.push_back(newEntry->str());
    addedFor.insert(addedFor.end(), identifiers.begin(), identifiers.end());
  }

  void logChangedRoute(
      const std::shared_ptr<LabelForwardingEntry>& oldEntry,
      const std::shared_ptr<LabelForwardingEntry>& newEntry,
      const std::vector<std::string>& identifiers) override {
    changed.push_back({oldEntry->str(), newEntry->str()});
    changedFor.insert(changedFor.end(), identifiers.begin(), identifiers.end());
  }

  void logRemovedRoute(
      const std::shared_ptr<LabelForwardingEntry>& oldEntry,
      const std::vector<std::string>& identifiers) override {
    removed.push_back(oldEntry->str());
    removedFor.insert(removedFor.end(), identifiers.begin(), identifiers.end());
  }

  std::vector<std::string> added;
  std::vector<std::string> addedFor;
  std::vector<std::string> removed;
  std::vector<std::string> removedFor;
  std::vector<std::pair<std::string, std::string>> changed;
  std::vector<std::string> changedFor;
};

class RouteUpdateLoggerTest : public ::testing::Test {
 public:
  void SetUp() override {
    handle = createTestHandle();
    sw = handle->getSw();
    initState = sw->getState();
    auto initialStateA = testStateA();
    RouteUpdater updater(initialStateA->getRouteTables());

    // Add default routes for consistency
    updater.addRoute(
        RouterID(0),
        folly::IPAddressV4("0.0.0.0"),
        0,
        ClientID::STATIC_ROUTE,
        RouteNextHopEntry(
            RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
    updater.addRoute(
        RouterID(0),
        folly::IPAddressV6("::"),
        0,
        ClientID::STATIC_ROUTE,
        RouteNextHopEntry(
            RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
    auto newRt = updater.updateDone();
    stateA = initialStateA->clone();
    stateA->resetRouteTables(newRt);

    deltaAdd = std::make_shared<StateDelta>(initState, stateA);
    deltaRemove = std::make_shared<StateDelta>(stateA, initState);
    routeUpdateLogger = std::make_unique<RouteUpdateLogger>(
        sw,
        std::make_unique<MockRouteLogger<folly::IPAddressV4>>(),
        std::make_unique<MockRouteLogger<folly::IPAddressV6>>(),
        std::make_unique<MockMplsRouteLogger>());
    mockRouteLoggerV4 = static_cast<MockRouteLogger<folly::IPAddressV4>*>(
        routeUpdateLogger->getRouteLoggerV4());
    mockRouteLoggerV6 = static_cast<MockRouteLogger<folly::IPAddressV6>*>(
        routeUpdateLogger->getRouteLoggerV6());
    mockMplsRouteLogger = static_cast<MockMplsRouteLogger*>(
        routeUpdateLogger->getMplsRouteLogger());
    user = "fboss-user";
  }

  void startLogging(
      const std::string& addr,
      uint8_t mask,
      const std::string& user,
      bool exact) {
    RoutePrefix<folly::IPAddress> prefix{folly::IPAddress{addr}, mask};
    auto req =
        std::make_unique<RouteUpdateLoggingInstance>(prefix, user, exact);
    routeUpdateLogger->startLoggingForPrefix(*req);
  }

  void startLogging(const std::string& addr, uint8_t mask) {
    startLogging(addr, mask, "", false);
  }

  void
  stopLogging(const std::string& addr, uint8_t mask, const std::string& user) {
    routeUpdateLogger->stopLoggingForPrefix(folly::IPAddress{addr}, mask, user);
  }

  void stopLogging(const std::string& addr, uint8_t mask) {
    stopLogging(addr, mask, "");
  }

  void startLogging(LabelForwardingEntry::Label label) {
    startLogging(label, "");
  }

  void startLogging(
      LabelForwardingEntry::Label label,
      const std::string& identifier) {
    routeUpdateLogger->startLoggingForLabel(label, identifier);
  }

  void stopLogging(
      LabelForwardingEntry::Label label,
      const std::string& identifier) {
    routeUpdateLogger->stopLoggingForLabel(label, identifier);
  }

  void logAllRouteUpdates() {
    startLogging("::", 0);
    startLogging("0.0.0.0", 0);
  }

  void expectNoChanged() {
    EXPECT_EQ(0, mockRouteLoggerV4->changed.size());
    EXPECT_EQ(0, mockRouteLoggerV6->changed.size());
  }

  void expectNoRemoved() {
    EXPECT_EQ(0, mockRouteLoggerV4->removed.size());
    EXPECT_EQ(0, mockRouteLoggerV6->removed.size());
  }

  void expectNoAdded() {
    EXPECT_EQ(0, mockRouteLoggerV4->added.size());
    EXPECT_EQ(0, mockRouteLoggerV6->added.size());
  }

  void expectNoLogging() {
    expectNoChanged();
    expectNoRemoved();
    expectNoAdded();
  }

  std::shared_ptr<SwitchState> addLabel(
      const std::shared_ptr<SwitchState>& state,
      LabelForwardingEntry::Label label,
      ClientID client = ClientID::OPENR) {
    auto newState = state->clone();
    auto entry = std::make_shared<LabelForwardingEntry>(
        label,
        client,
        LabelNextHopEntry{RouteForwardAction::DROP,
                          AdminDistance::MAX_ADMIN_DISTANCE});
    auto labelFib =
        newState->getLabelForwardingInformationBase()->modify(&newState);
    labelFib->addNode(entry);
    newState->publish();
    return newState;
  }

  std::shared_ptr<SwitchState> removeLabel(
      const std::shared_ptr<SwitchState>& state,
      LabelForwardingEntry::Label label,
      ClientID client = ClientID::OPENR) {
    auto newState = state->clone();
    auto labelFib =
        newState->getLabelForwardingInformationBase()->modify(&newState);
    labelFib->unprogramLabel(&newState, label, client);
    newState->publish();
    return newState;
  }

  std::shared_ptr<SwitchState> initState;
  std::shared_ptr<SwitchState> stateA;
  std::shared_ptr<StateDelta> deltaAdd;
  std::shared_ptr<StateDelta> deltaRemove;
  SwSwitch* sw;
  std::unique_ptr<HwTestHandle> handle;
  MockRouteLogger<folly::IPAddressV4>* mockRouteLoggerV4;
  MockRouteLogger<folly::IPAddressV6>* mockRouteLoggerV6;
  MockMplsRouteLogger* mockMplsRouteLogger;
  std::unique_ptr<RouteUpdateLogger> routeUpdateLogger;
  std::string user;
};

// Adding some routes will get logged correctly
TEST_F(RouteUpdateLoggerTest, LogAdded) {
  logAllRouteUpdates();
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(5, mockRouteLoggerV4->added.size());
  EXPECT_EQ(3, mockRouteLoggerV6->added.size());
  // Default route changes
  EXPECT_EQ(1, mockRouteLoggerV4->changed.size());
  EXPECT_EQ(1, mockRouteLoggerV6->changed.size());
  expectNoRemoved();
}

// Removing some routes will get logged correctly
TEST_F(RouteUpdateLoggerTest, LogRemoved) {
  logAllRouteUpdates();
  routeUpdateLogger->stateUpdated(*deltaRemove);
  EXPECT_EQ(5, mockRouteLoggerV4->removed.size());
  EXPECT_EQ(3, mockRouteLoggerV6->removed.size());
  // Default route changes
  EXPECT_EQ(1, mockRouteLoggerV4->changed.size());
  EXPECT_EQ(1, mockRouteLoggerV6->changed.size());
  expectNoAdded();
}

// If no logging is enabled, nothing gets logged
TEST_F(RouteUpdateLoggerTest, LogUntracked) {
  routeUpdateLogger->stateUpdated(*deltaAdd);
  routeUpdateLogger->stateUpdated(*deltaRemove);
  expectNoLogging();
}

// If we track an untouched prefix, nothing gets logged
TEST_F(RouteUpdateLoggerTest, TrackWrongPrefix) {
  startLogging("1:1:1:1::", 64);
  startLogging("1.1.1.1", 16);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  expectNoChanged();
}

// If we track exactly an affected prefix, that prefix gets logged,
// even if we aren't using 'exact' mode
TEST_F(RouteUpdateLoggerTest, LogTrackedPrefix) {
  startLogging("192.168.0.0", 24);
  startLogging("2401:db00:2110:3001::", 64);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(1, mockRouteLoggerV4->added.size());
  EXPECT_EQ(1, mockRouteLoggerV6->added.size());
}

// If we track a prefix, a more specific prefix gets logged
TEST_F(RouteUpdateLoggerTest, MoreSpecificPrefix) {
  startLogging("192.168.0.0", 16);
  startLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
}

// If we track a prefix expecting an exact match, a more specific prefix
// should not be logged, but that exact prefix should be
TEST_F(RouteUpdateLoggerTest, MoreSpecificPrefixExactLogging) {
  startLogging("192.168.0.0", 16, "", true);
  startLogging("2401:db00::", 32, "", true);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  expectNoChanged();
  expectNoRemoved();
}

// Turn off logging that has been turned on previously
TEST_F(RouteUpdateLoggerTest, StopLogging) {
  startLogging("192.168.0.0", 16);
  startLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  stopLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(4, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  stopLogging("192.168.0.0", 16);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(4, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  expectNoChanged();
  expectNoRemoved();
}

// Restart logging that has been turned off
TEST_F(RouteUpdateLoggerTest, RestartLogging) {
  startLogging("192.168.0.0", 16);
  startLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  stopLogging("192.168.0.0", 16);
  stopLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  startLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(4, mockRouteLoggerV6->added.size());
  expectNoChanged();
  expectNoRemoved();
}

// If we are logging in "allow more specific" mode and switch to
// exact mode, we should now be logging in exact mode
TEST_F(RouteUpdateLoggerTest, SwitchToExact) {
  startLogging("192.168.0.0", 16);
  startLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  startLogging("192.168.0.0", 16, "", true);
  startLogging("2401:db00::", 32, "", true);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  expectNoChanged();
  expectNoRemoved();
}

// If we are logging in exact mode and switch to
// "allow more specific" mode, we should now be logging permissively
TEST_F(RouteUpdateLoggerTest, SwitchToAllowMoreSpecific) {
  startLogging("192.168.0.0", 16, "", true);
  startLogging("2401:db00::", 32, "", true);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  expectNoLogging();
  startLogging("192.168.0.0", 16);
  startLogging("2401:db00::", 32);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  expectNoChanged();
  expectNoRemoved();
}

// Two users can separately add logging
TEST_F(RouteUpdateLoggerTest, StartLoggingFromDifferentUsers) {
  startLogging("192.168.0.0", 16, "foo", false);
  startLogging("2401:db00::", 32, "bar", false);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  expectNoChanged();
  expectNoRemoved();
}

// One user stopping logging should not affect another
TEST_F(RouteUpdateLoggerTest, StopForOneUser) {
  startLogging("2401:db00::", 32, "foo", false);
  startLogging("2401:db00::", 32, "bar", false);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(0, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  stopLogging("2401:db00::", 32, "bar");
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(0, mockRouteLoggerV4->added.size());
  EXPECT_EQ(4, mockRouteLoggerV6->added.size());
  stopLogging("2401:db00::", 32, "foo");
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(0, mockRouteLoggerV4->added.size());
  EXPECT_EQ(4, mockRouteLoggerV6->added.size());
  expectNoChanged();
  expectNoRemoved();
}

// stop all logging for an identifier
TEST_F(RouteUpdateLoggerTest, ClearForOneUser) {
  startLogging("192.168.0.0", 16, "foo", false);
  startLogging("2401:db00::", 32, "foo", false);
  startLogging("2401:db00::", 32, "bar", false);
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(2, mockRouteLoggerV6->added.size());
  routeUpdateLogger->stopLoggingForIdentifier("foo");
  routeUpdateLogger->stateUpdated(*deltaAdd);
  EXPECT_EQ(2, mockRouteLoggerV4->added.size());
  EXPECT_EQ(4, mockRouteLoggerV6->added.size());
}

TEST_F(RouteUpdateLoggerTest, LabelAdded) {
  startLogging(100);
  startLogging(200);
  startLogging(300);

  auto state = addLabel(initState, 100);
  state = addLabel(state, 200);
  state = addLabel(state, 300);

  routeUpdateLogger->stateUpdated(StateDelta(initState, state));
  EXPECT_EQ(3, mockMplsRouteLogger->added.size());
}

TEST_F(RouteUpdateLoggerTest, LabelRemoved) {
  startLogging(100);
  startLogging(200);
  startLogging(300);

  auto state = addLabel(initState, 100);
  state = addLabel(state, 200);
  state = addLabel(state, 300);
  routeUpdateLogger->stateUpdated(StateDelta(initState, state));
  EXPECT_EQ(3, mockMplsRouteLogger->added.size());

  auto newState = removeLabel(state, 300);
  routeUpdateLogger->stateUpdated(StateDelta(state, newState));
  EXPECT_EQ(1, mockMplsRouteLogger->removed.size());
}

TEST_F(RouteUpdateLoggerTest, LabelChanged) {
  startLogging(100);

  auto state = addLabel(initState, 100);
  routeUpdateLogger->stateUpdated(StateDelta(initState, state));
  EXPECT_EQ(1, mockMplsRouteLogger->added.size());
  auto newState = removeLabel(state, 100);
  newState = addLabel(newState, 100, ClientID::STATIC_ROUTE);
  routeUpdateLogger->stateUpdated(StateDelta(state, newState));
  EXPECT_EQ(1, mockMplsRouteLogger->changed.size());
}

TEST_F(RouteUpdateLoggerTest, MultipleSubscribers) {
  startLogging(100, "foo");
  startLogging(100, "bar");
  startLogging(100, "foobar");

  auto state = addLabel(initState, 100);
  state = addLabel(state, 200);

  routeUpdateLogger->stateUpdated(StateDelta(initState, state));
  EXPECT_EQ(1, mockMplsRouteLogger->added.size());
  EXPECT_EQ(3, mockMplsRouteLogger->addedFor.size());

  stopLogging(100, "foo");
  auto newState = removeLabel(state, 100);
  routeUpdateLogger->stateUpdated(StateDelta(state, newState));
  EXPECT_EQ(1, mockMplsRouteLogger->removed.size());
  EXPECT_EQ(2, mockMplsRouteLogger->removedFor.size());

  startLogging(200, "foo");
  startLogging(200, "bar");
  startLogging(200, "foobar");
  auto anotherNewState = removeLabel(newState, 200);
  anotherNewState = addLabel(anotherNewState, 200, ClientID::STATIC_ROUTE);
  routeUpdateLogger->stateUpdated(StateDelta(newState, anotherNewState));
  EXPECT_EQ(1, mockMplsRouteLogger->changed.size());
  EXPECT_EQ(3, mockMplsRouteLogger->changedFor.size());

  stopLogging(200, "foobar");
  mockMplsRouteLogger->changed.clear();
  mockMplsRouteLogger->changedFor.clear();
  auto oneMoreNewState =
      removeLabel(anotherNewState, 200, ClientID::STATIC_ROUTE);
  oneMoreNewState = addLabel(oneMoreNewState, 200);

  routeUpdateLogger->stateUpdated(StateDelta(anotherNewState, oneMoreNewState));
  EXPECT_EQ(1, mockMplsRouteLogger->changed.size());
  EXPECT_EQ(2, mockMplsRouteLogger->changedFor.size());
}

TEST_F(RouteUpdateLoggerTest, TrackAllLabelsManySubscribers) {
  startLogging(-1, "foo");
  startLogging(-1, "bar");

  auto state = addLabel(initState, 100);
  state = addLabel(state, 200);
  state = addLabel(state, 300);

  routeUpdateLogger->stateUpdated(StateDelta(initState, state));
  EXPECT_EQ(3, mockMplsRouteLogger->added.size());
  EXPECT_EQ(6, mockMplsRouteLogger->addedFor.size());

  stopLogging(-1, "bar");
  auto newState = removeLabel(state, 100);
  newState = addLabel(newState, 100, ClientID::STATIC_ROUTE);
  routeUpdateLogger->stateUpdated(StateDelta(state, newState));
  EXPECT_EQ(1, mockMplsRouteLogger->changed.size());
  EXPECT_EQ(1, mockMplsRouteLogger->changedFor.size());
}
} // namespace
