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
  void addDeltaSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addSubscription(path, stateChangeCb, operDeltaCb, host);
  }
  void addStateSubscription(
      const std::vector<std::string>& path,
      const std::string& host = "::1") {
    pubSubManager_.addSubscription(path, stateChangeCb, operStateCb, host);
  }
  FsdbPubSubManager pubSubManager_{"testMgr"};
};

TEST_F(PubSubManagerTest, createDeltaPublisher) {
  pubSubManager_.createDeltaPublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createStatePublisher({}, stateChangeCb),
      std::runtime_error);
}

TEST_F(PubSubManagerTest, createStatePublisher) {
  pubSubManager_.createStatePublisher({}, stateChangeCb);
  EXPECT_THROW(
      pubSubManager_.createStatePublisher({}, stateChangeCb),
      std::runtime_error);
  EXPECT_THROW(
      pubSubManager_.createDeltaPublisher({}, stateChangeCb),
      std::runtime_error);
}

TEST_F(PubSubManagerTest, publishDelta) {
  pubSubManager_.createDeltaPublisher({}, stateChangeCb);
  pubSubManager_.publish(OperDelta{});
  EXPECT_THROW(pubSubManager_.publish(OperState{}), std::runtime_error);
}

TEST_F(PubSubManagerTest, publishState) {
  pubSubManager_.createStatePublisher({}, stateChangeCb);
  pubSubManager_.publish(OperState{});
  EXPECT_THROW(pubSubManager_.publish(OperDelta{}), std::runtime_error);
}

TEST_F(PubSubManagerTest, addRemoveSubscriptions) {
  addDeltaSubscription({});
  addDeltaSubscription({"foo"});
  // multiple delta subscriptions to same host, path
  EXPECT_THROW(addDeltaSubscription({}), std::runtime_error);
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 2);
  pubSubManager_.removeDeltaSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 1);
  addDeltaSubscription({});
  // Same subscripton path, different host
  addDeltaSubscription({"foo"}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);
  // remove non existent state subscription. No effect
  pubSubManager_.removeStateSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 3);

  // Add state subscription for same path, host as delta
  addStateSubscription({});
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 4);
  // multiple delta subscriptions to same host, path
  EXPECT_THROW(addStateSubscription({}), std::runtime_error);
  // Same subscripton path, different host
  addStateSubscription({}, "::2");
  EXPECT_EQ(pubSubManager_.numSubscriptions(), 5);
}

} // namespace facebook::fboss::fsdb::test
