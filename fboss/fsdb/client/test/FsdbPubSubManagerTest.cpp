// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#define FsdbPubSubManager_TEST_FRIENDS \
  FRIEND_TEST(PubSubManagerTest, passEvbOrNot);

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/AsyncPipe.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>

#include <algorithm>
#include <atomic>

namespace facebook::fboss::fsdb {
void stateChangeCb(
    FsdbStreamClient::State /*old*/,
    FsdbStreamClient::State /*new*/) {}
void subscriptionStateChangeCb(
    SubscriptionState /*old*/,
    SubscriptionState /*new*/) {}
void operDeltaCb(OperDelta /*delta*/) {}
void operStateCb(OperState /*state*/) {}

class PubSubManagerTest : public ::testing::Test {
 protected:
  void addStateDeltaSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStateDeltaSubscription(
        path,
        subscriptionStateChangeCb,
        operDeltaCb,
        FsdbStreamClient::ServerOptions(host, FLAGS_fsdbPort));
  }
  void addStatDeltaSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStatDeltaSubscription(
        path,
        subscriptionStateChangeCb,
        operDeltaCb,
        FsdbStreamClient::ServerOptions(host, FLAGS_fsdbPort));
  }
  void addStatePathSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStatePathSubscription(
        path,
        subscriptionStateChangeCb,
        operStateCb,
        FsdbStreamClient::ServerOptions(host, FLAGS_fsdbPort));
  }
  void addStatPathSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStatPathSubscription(
        path,
        subscriptionStateChangeCb,
        operStateCb,
        FsdbStreamClient::ServerOptions(host, FLAGS_fsdbPort));
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
  // Stat delta subscriptions to same path as state delta ok
  addStatDeltaSubscription({"foo"});
  // Dup stat delta subscription should throw
  EXPECT_THROW(addStatDeltaSubscription({"foo"}), std::runtime_error);
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);
  pubSubManager_.removeStateDeltaSubscription(std::vector<std::string>{});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 2);
  addStateDeltaSubscription({});
  // Same subscripton path, different host
  addStateDeltaSubscription({"foo"}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 4);
  // remove non existent state subscription. No effect
  pubSubManager_.removeStatePathSubscription(std::vector<std::string>{});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 4);

  // Add state, stat subscription for same path, host as delta
  addStatePathSubscription({});
  addStatPathSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 6);
  // multiple state, stat subscriptions to same host, path
  EXPECT_THROW(addStatePathSubscription({}), std::runtime_error);
  EXPECT_THROW(addStatPathSubscription({}), std::runtime_error);
  // Same subscription path, different host
  addStatePathSubscription({}, "::2");
  addStatPathSubscription({}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 8);

  // Verify getSubscriptionInfo
  const auto subscriptionInfoList = pubSubManager_.getSubscriptionInfo();
  EXPECT_EQ(subscriptionInfoList.size(), 8);
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
  addStatDeltaSubscription({"foo"});
  EXPECT_THROW(addStateDeltaSubscription({"foo"}), std::runtime_error);
  EXPECT_THROW(addStatDeltaSubscription({"foo"}), std::runtime_error);

  pubSubManager_.clearStateSubscriptions();
  // resub should succeed
  addStateDeltaSubscription({"foo"});
  // still have stat sub registered
  EXPECT_THROW(addStatDeltaSubscription({"foo"}), std::runtime_error);

  pubSubManager_.clearStatSubscriptions();
  addStatDeltaSubscription({"foo"});
}

TEST_F(PubSubManagerTest, TestSubscriptionInfo) {
  EXPECT_EQ(FsdbPatchSubscriber::subscriptionType(), SubscriptionType::PATCH);
  EXPECT_EQ(FsdbStateSubscriber::subscriptionType(), SubscriptionType::PATH);
  EXPECT_EQ(FsdbDeltaSubscriber::subscriptionType(), SubscriptionType::DELTA);
}

} // namespace facebook::fboss::fsdb
