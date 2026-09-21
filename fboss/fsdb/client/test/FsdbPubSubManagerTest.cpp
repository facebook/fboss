// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#define FsdbPubSubManager_TEST_FRIENDS \
  FRIEND_TEST(PubSubManagerTest, passEvbOrNot);

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <folly/coro/AsyncPipe.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <algorithm>
#include <set>

namespace facebook::fboss::fsdb {
void stateChangeCb(
    FsdbStreamClient::State /*old*/,
    FsdbStreamClient::State /*new*/) {}
void subscriptionStateChangeCb(
    SubscriptionState /*old*/,
    SubscriptionState /*new*/,
    std::optional<bool> /*initialSyncHasData*/) {}
void operDeltaCb(OperDelta /*delta*/) {}
void operStateCb(OperState /*state*/) {}
void operPatchCb(const SubscriberChunk& /*patch*/) {}

class PubSubManagerTest : public ::testing::Test {
 protected:
  void addStateDeltaSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStateDeltaSubscription(
        path,
        subscriptionStateChangeCb,
        operDeltaCb,
        utils::ConnectionOptions(host, FLAGS_fsdbPort));
  }
  void addStatePathSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStatePathSubscription(
        path,
        subscriptionStateChangeCb,
        operStateCb,
        utils::ConnectionOptions(host, FLAGS_fsdbPort));
  }
  void addStatPathSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStatPathSubscription(
        path,
        subscriptionStateChangeCb,
        operStateCb,
        utils::ConnectionOptions(host, FLAGS_fsdbPort));
  }
  void addStatePatchSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    RawOperPath rawPath;
    rawPath.path() = path;
    pubSubManager_.addStatePatchSubscription(
        {{42, rawPath}},
        subscriptionStateChangeCb,
        operPatchCb,
        utils::ConnectionOptions(host, FLAGS_fsdbPort));
  }
  FsdbPubSubManager pubSubManager_{"testMgr"};
};

TEST_F(PubSubManagerTest, createStateAndStatDeltaPublisher) {
  pubSubManager_.createStateDeltaPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStateDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatePathPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatePatchPublisher({}, stateChangeCb),
      std::runtime_error);
  pubSubManager_.createStatDeltaPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStatDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatPathPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatPatchPublisher({}, stateChangeCb),
      std::runtime_error);
}

TEST_F(PubSubManagerTest, createStateAndStatPathPublisher) {
  pubSubManager_.createStatePathPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStatePathPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStateDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatePatchPublisher({}, stateChangeCb),
      std::runtime_error);
  pubSubManager_.createStatPathPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStatPathPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatPatchPublisher({}, stateChangeCb),
      std::runtime_error);
}

TEST_F(PubSubManagerTest, publishStateAndStatDelta) {
  pubSubManager_.createStateDeltaPublisher({}, stateChangeCb);
  pubSubManager_.publishState(OperDelta{});
  EXPECT_THROW(pubSubManager_.publishState(OperState{}), std::runtime_error);
  pubSubManager_.createStatDeltaPublisher({}, stateChangeCb);
  pubSubManager_.publishStat(OperDelta{});
  EXPECT_THROW(pubSubManager_.publishStat(OperState{}), std::runtime_error);
}

TEST_F(PubSubManagerTest, publishPathStatState) {
  pubSubManager_.createStatePathPublisher({}, stateChangeCb);
  pubSubManager_.publishState(OperState{});
  EXPECT_THROW(pubSubManager_.publishState(OperDelta{}), std::runtime_error);
  EXPECT_THROW(pubSubManager_.publishState(Patch{}), std::runtime_error);
  pubSubManager_.createStatPathPublisher({}, stateChangeCb);
  pubSubManager_.publishStat(OperState{});
  EXPECT_THROW(pubSubManager_.publishStat(OperDelta{}), std::runtime_error);
  EXPECT_THROW(pubSubManager_.publishStat(Patch{}), std::runtime_error);
}

TEST_F(PubSubManagerTest, addRemoveSubscriptions) {
  addStateDeltaSubscription({});
  addStateDeltaSubscription({"foo"});
  // multiple delta subscriptions to same host, path
  EXPECT_THROW(addStateDeltaSubscription({}), std::runtime_error);
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 2);
  pubSubManager_.removeStateDeltaSubscription(std::vector<std::string>{});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 1);
  addStateDeltaSubscription({});
  // Same subscripton path, different host
  addStateDeltaSubscription({"foo"}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);
  // remove non existent state subscription. No effect
  pubSubManager_.removeStatePathSubscription(std::vector<std::string>{});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);

  // Add state, stat subscription for same path, host as delta
  addStatePathSubscription({});
  addStatPathSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 5);
  // multiple state, stat subscriptions to same host, path
  EXPECT_THROW(addStatePathSubscription({}), std::runtime_error);
  EXPECT_THROW(addStatPathSubscription({}), std::runtime_error);
  // Same subscription path, different host
  addStatePathSubscription({}, "::2");
  addStatPathSubscription({}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 7);

  // Verify getSubscriptionInfo
  const auto subscriptionInfoList = pubSubManager_.getSubscriptionInfo();
  EXPECT_EQ(subscriptionInfoList.size(), 7);
  for (const auto& subscriptionInfo : subscriptionInfoList) {
    EXPECT_TRUE(
        subscriptionInfo.server == "::1" || subscriptionInfo.server == "::2");
    EXPECT_EQ(subscriptionInfo.state, FsdbStreamClient::State::DISCONNECTED);
    if (subscriptionInfo.paths.size()) {
      EXPECT_EQ(subscriptionInfo.paths.size(), 1);
      EXPECT_EQ(subscriptionInfo.paths[0], "foo");
    }
  }
}

TEST_F(PubSubManagerTest, passEvbOrNot) {
  // local thread manager
  auto& localThreadManager = pubSubManager_;

  // create manager with client evb pass through
  auto externalThread = std::make_unique<folly::ScopedEventBaseThread>();
  auto externalThreadManager = std::make_unique<FsdbPubSubManager>(
      "externalThreadMgr",
      externalThread->getEventBase(),
      externalThread->getEventBase(),
      externalThread->getEventBase(),
      externalThread->getEventBase());

  // external event base manager won't spawn local threads
  EXPECT_EQ(externalThreadManager->reconnectEvbThread_, nullptr);
  EXPECT_EQ(externalThreadManager->subscriberEvbThread_, nullptr);
  EXPECT_EQ(externalThreadManager->statsPublisherStreamEvbThread_, nullptr);
  EXPECT_EQ(externalThreadManager->statePublisherStreamEvbThread_, nullptr);

  // local event base manager will have local threads
  EXPECT_NE(localThreadManager.reconnectEvbThread_, nullptr);
  EXPECT_NE(localThreadManager.subscriberEvbThread_, nullptr);
  EXPECT_NE(localThreadManager.statsPublisherStreamEvbThread_, nullptr);
  EXPECT_NE(localThreadManager.statePublisherStreamEvbThread_, nullptr);
}

TEST_F(PubSubManagerTest, removeAllSubscriptions) {
  addStateDeltaSubscription({"foo"});
  addStatPathSubscription({"foo"});
  EXPECT_THROW(addStateDeltaSubscription({"foo"}), std::runtime_error);
  EXPECT_THROW(addStatPathSubscription({"foo"}), std::runtime_error);

  pubSubManager_.clearStateSubscriptions();
  // resub should succeed
  addStateDeltaSubscription({"foo"});
  // still have stat sub registered
  EXPECT_THROW(addStatPathSubscription({"foo"}), std::runtime_error);

  pubSubManager_.clearStatSubscriptions();
  addStatPathSubscription({"foo"});
}

TEST_F(PubSubManagerTest, repointSubscriptionsMovesSubscribersToNewHost) {
  // Three subscriptions targeting ::1 (state delta, state path, stat path) plus
  // one on a different host that must be left untouched.
  addStateDeltaSubscription({"foo"}, "::1");
  addStatePathSubscription({"bar"}, "::1");
  addStatPathSubscription({"baz"}, "::1");
  addStateDeltaSubscription({"other"}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 4);

  // Repoint reports how many it moved (3) -- a silently-ineffective flip would
  // otherwise be indistinguishable from a real one.
  EXPECT_EQ(
      pubSubManager_.repointSubscriptions(
          "::1", utils::ConnectionOptions("::3", FLAGS_fsdbPort)),
      3);

  // Repoint reuses the existing subscribers: nothing is added or dropped.
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 4);

  // Verify the per-host path *sets*, not just counts: each former ::1
  // subscriber now reports ::3 with its original path intact, the ::2 one is
  // untouched, and none remain on ::1.
  std::set<std::string> pathsOnNewHost, pathsOnUntouchedHost;
  size_t onOldHost = 0;
  for (const auto& info : pubSubManager_.getSubscriptionInfo()) {
    if (info.server == "::3") {
      pathsOnNewHost.insert(info.paths.begin(), info.paths.end());
    } else if (info.server == "::2") {
      pathsOnUntouchedHost.insert(info.paths.begin(), info.paths.end());
    } else if (info.server == "::1") {
      ++onOldHost;
    }
  }
  EXPECT_EQ(pathsOnNewHost, (std::set<std::string>{"foo", "bar", "baz"}));
  EXPECT_EQ(pathsOnUntouchedHost, (std::set<std::string>{"other"}));
  EXPECT_EQ(onOldHost, 0);

  // Each moved key was rewritten to ::3: re-adding the same (type, path) at ::3
  // collides (new key present) while ::1 is free again (old key released) --
  // catching a key-suffix corruption that the count/server checks would miss.
  addStateDeltaSubscription({"foo"}, "::1");
  EXPECT_THROW(addStateDeltaSubscription({"foo"}, "::3"), std::runtime_error);
  EXPECT_THROW(addStatePathSubscription({"bar"}, "::3"), std::runtime_error);
  EXPECT_THROW(addStatPathSubscription({"baz"}, "::3"), std::runtime_error);
}

TEST_F(PubSubManagerTest, repointSubscriptionsToSameHostIsNoop) {
  addStateDeltaSubscription({"foo"}, "::1");
  // Same host: nothing to move.
  EXPECT_EQ(
      pubSubManager_.repointSubscriptions(
          "::1", utils::ConnectionOptions("::1", FLAGS_fsdbPort)),
      0);
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 1);
  // Re-adding the same key must still collide (subscriber was not dropped).
  EXPECT_THROW(addStateDeltaSubscription({"foo"}, "::1"), std::runtime_error);
}

TEST_F(PubSubManagerTest, repointSubscriptionsReturnsZeroWhenNothingMatches) {
  addStateDeltaSubscription({"foo"}, "::1");
  // No subscription targets ::9, so the repoint matches nothing and reports 0
  // (so the caller can warn instead of logging a successful-looking flip).
  EXPECT_EQ(
      pubSubManager_.repointSubscriptions(
          "::9", utils::ConnectionOptions("::3", FLAGS_fsdbPort)),
      0);
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 1);
}

TEST_F(
    PubSubManagerTest,
    repointSubscriptionsThrowsAndIsAtomicOnDestCollision) {
  // Two subscriptions on ::1 plus one already on the destination ::3 with the
  // same state-delta path "foo" -- so foo's destination key is already taken.
  addStateDeltaSubscription({"foo"}, "::1");
  addStatePathSubscription({"bar"}, "::1");
  addStateDeltaSubscription({"foo"}, "::3");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);

  // The destination key collides, so the repoint throws...
  EXPECT_THROW(
      pubSubManager_.repointSubscriptions(
          "::1", utils::ConnectionOptions("::3", FLAGS_fsdbPort)),
      std::runtime_error);

  // ...and leaves everything where it was (no partial move): ::1 still owns
  // both its keys and the pre-existing ::3/foo is intact, so re-adding any
  // collides.
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);
  EXPECT_THROW(addStateDeltaSubscription({"foo"}, "::1"), std::runtime_error);
  EXPECT_THROW(addStatePathSubscription({"bar"}, "::1"), std::runtime_error);
  EXPECT_THROW(addStateDeltaSubscription({"foo"}, "::3"), std::runtime_error);
}

TEST_F(PubSubManagerTest, repointSubscriptionsMovesPreferredPatchSubscriber) {
  // Patch is the preferred subscription type; verify it repoints like the rest.
  addStatePatchSubscription({"qux"}, "::1");
  EXPECT_EQ(
      pubSubManager_.repointSubscriptions(
          "::1", utils::ConnectionOptions("::3", FLAGS_fsdbPort)),
      1);
  const auto infos = pubSubManager_.getSubscriptionInfo();
  ASSERT_EQ(infos.size(), 1);
  EXPECT_EQ(infos[0].server, "::3");
  EXPECT_EQ(infos[0].subscriptionType, SubscriptionType::PATCH);
  // The patch key was rewritten to ::3: re-adding at ::3 collides, ::1 is
  // freed.
  EXPECT_THROW(addStatePatchSubscription({"qux"}, "::3"), std::runtime_error);
  addStatePatchSubscription({"qux"}, "::1");
}

TEST_F(PubSubManagerTest, TestSubscriptionInfo) {
  EXPECT_EQ(FsdbPatchSubscriber::subscriptionType(), SubscriptionType::PATCH);
  EXPECT_EQ(FsdbStateSubscriber::subscriptionType(), SubscriptionType::PATH);
  EXPECT_EQ(FsdbDeltaSubscriber::subscriptionType(), SubscriptionType::DELTA);
}

} // namespace facebook::fboss::fsdb
