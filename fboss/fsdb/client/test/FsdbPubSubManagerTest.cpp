// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/AsyncPipe.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>

namespace facebook::fboss::fsdb::test {
void stateChangeCb(
    FsdbStreamClient::State /*old*/,
    FsdbStreamClient::State /*new*/) {}
void operDeltaCb(OperDelta /*delta*/) {}
void operStateCb(OperState /*state*/) {}

class PubSubManagerTest : public ::testing::Test {
 protected:
  void addStateDeltaSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStateDeltaSubscription(
        path, stateChangeCb, operDeltaCb, host);
  }
  void addStatePathSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addStatePathSubscription(
        path, stateChangeCb, operStateCb, host);
  }
  FsdbPubSubManager pubSubManager_{"testMgr"};
};

TEST_F(PubSubManagerTest, createStateDeltaPublisher) {
  pubSubManager_.createStateDeltaPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStateDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatePathPublisher({}, stateChangeCb),
      std::runtime_error);
}

TEST_F(PubSubManagerTest, createStatePathPublisher) {
  pubSubManager_.createStatePathPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStatePathPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStateDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
}

TEST_F(PubSubManagerTest, publishDelta) {
  pubSubManager_.createStateDeltaPublisher({}, stateChangeCb);
  pubSubManager_.publishState(OperDelta{});
  EXPECT_THROW(pubSubManager_.publishState(OperState{}), std::runtime_error);
}

TEST_F(PubSubManagerTest, publishState) {
  pubSubManager_.createStatePathPublisher({}, stateChangeCb);
  pubSubManager_.publishState(OperState{});
  EXPECT_THROW(pubSubManager_.publishState(OperDelta{}), std::runtime_error);
}

TEST_F(PubSubManagerTest, addRemoveSubscriptions) {
  addStateDeltaSubscription({});
  addStateDeltaSubscription({"foo"});
  // multiple delta subscriptions to same host, path
  EXPECT_THROW(addStateDeltaSubscription({}), std::runtime_error);
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 2);
  pubSubManager_.removeStateDeltaSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 1);
  addStateDeltaSubscription({});
  // Same subscripton path, different host
  addStateDeltaSubscription({"foo"}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);
  // remove non existent state subscription. No effect
  pubSubManager_.removeStatePathSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);

  // Add state subscription for same path, host as delta
  addStatePathSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 4);
  // multiple delta subscriptions to same host, path
  EXPECT_THROW(addStatePathSubscription({}), std::runtime_error);
  // Same subscripton path, different host
  addStatePathSubscription({}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 5);
}

} // namespace facebook::fboss::fsdb::test
