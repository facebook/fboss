// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/cgo/FsdbCgoPubSubWrapper.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"

using namespace facebook::fboss::fsdb;
using namespace facebook::fboss::fsdb::test;
using namespace std::chrono_literals;

namespace facebook::fboss::fsdb::test {

constexpr auto kPublisherId = "fsdb_cgo_test_publisher";
const std::vector<std::string> kPublishRoot{"agent"};

class FsdbCgoPubSubWrapperTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Create test FSDB server
    fsdbTestServer_ = std::make_unique<FsdbTestServer>();

    XLOG(INFO) << "FSDB test server started on port: "
               << fsdbTestServer_->getFsdbPort();
  }

  void TearDown() override {
    // Cleanup publishers
    if (pubSubManager_) {
      pubSubManager_.reset();
    }

    // Cleanup server
    if (fsdbTestServer_) {
      fsdbTestServer_.reset();
    }
  }

  // Helper to create publisher for state
  void createStatePublisher() {
    if (!pubSubManager_) {
      pubSubManager_ = std::make_unique<FsdbPubSubManager>(kPublisherId);
    }

    auto stateChangeCb =
        [this](FsdbStreamClient::State, FsdbStreamClient::State newState) {
          if (newState == FsdbStreamClient::State::CONNECTED) {
            publisherConnected_.post();
          }
        };

    pubSubManager_->createStatePathPublisher(
        kPublishRoot, stateChangeCb, fsdbTestServer_->getFsdbPort());

    // Wait for publisher to connect
    publisherConnected_.wait();
    publisherConnected_.reset();
  }

  // Helper to create publisher for stats
  void createStatsPublisher() {
    if (!pubSubManager_) {
      pubSubManager_ = std::make_unique<FsdbPubSubManager>(kPublisherId);
    }

    auto stateChangeCb =
        [this](FsdbStreamClient::State, FsdbStreamClient::State newState) {
          if (newState == FsdbStreamClient::State::CONNECTED) {
            publisherConnected_.post();
          }
        };

    pubSubManager_->createStatPathPublisher(
        kPublishRoot, stateChangeCb, fsdbTestServer_->getFsdbPort());

    // Wait for publisher to connect
    publisherConnected_.wait();
    publisherConnected_.reset();
  }

  // Helper to publish state
  void publishState(const state::SwitchState& switchState) {
    auto operState = makeSwitchStateOperState(switchState);
    pubSubManager_->publishState(std::move(operState));
  }

  // Helper to publish stats
  void publishStats(const folly::F14FastMap<std::string, HwPortStats>& stats) {
    auto operState = makeState(stats);
    pubSubManager_->publishStat(std::move(operState));
  }

 protected:
  std::unique_ptr<FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<FsdbPubSubManager> pubSubManager_;
  folly::Baton<> publisherConnected_;
};

// Basic unit tests (no server needed)
TEST_F(FsdbCgoPubSubWrapperTest, CreateWrapper) {
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");

  EXPECT_EQ(wrapper->getClientId(), "test-client");
  EXPECT_FALSE(wrapper->hasStateSubscription());
  EXPECT_FALSE(wrapper->hasStatsSubscription());
}

TEST_F(FsdbCgoPubSubWrapperTest, SubscribeToOperStatePortMaps) {
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");

  // Subscribe to portMaps - no return value
  EXPECT_NO_THROW(
      wrapper->subscribeToOperState_portMaps(fsdbTestServer_->getFsdbPort()));

  EXPECT_TRUE(wrapper->hasStateSubscription());

  // Second subscription should log warning but not throw
  EXPECT_NO_THROW(
      wrapper->subscribeToOperState_portMaps(fsdbTestServer_->getFsdbPort()));
  EXPECT_TRUE(wrapper->hasStateSubscription());
}

TEST_F(FsdbCgoPubSubWrapperTest, SubscribeToStatsPath) {
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");

  EXPECT_NO_THROW(wrapper->subscribeStatsPath(fsdbTestServer_->getFsdbPort()));
  EXPECT_TRUE(wrapper->hasStatsSubscription());

  // Second subscription should log warning but not throw
  EXPECT_NO_THROW(wrapper->subscribeStatsPath(fsdbTestServer_->getFsdbPort()));
  EXPECT_TRUE(wrapper->hasStatsSubscription());
}

TEST_F(FsdbCgoPubSubWrapperTest, WaitWithoutSubscription) {
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");

  EXPECT_THROW(wrapper->waitForStateUpdates(), std::runtime_error);
  EXPECT_THROW(wrapper->waitForStatsUpdates(), std::runtime_error);
}

TEST_F(FsdbCgoPubSubWrapperTest, StateAndStatsSubscriptions) {
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");

  EXPECT_NO_THROW(
      wrapper->subscribeToOperState_portMaps(fsdbTestServer_->getFsdbPort()));
  EXPECT_TRUE(wrapper->hasStateSubscription());

  EXPECT_NO_THROW(wrapper->subscribeStatsPath(fsdbTestServer_->getFsdbPort()));
  EXPECT_TRUE(wrapper->hasStatsSubscription());
}

// Integration tests with FSDB server and publisher
TEST_F(FsdbCgoPubSubWrapperTest, ReceiveStateUpdate) {
  // Create publisher
  createStatePublisher();

  // Create wrapper and subscribe
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");
  wrapper->subscribeToOperState_portMaps(fsdbTestServer_->getFsdbPort());

  // Give subscription time to establish
  std::this_thread::sleep_for(500ms);

  // Publish initial port state
  state::PortFields port1;
  port1.portId() = 1;
  port1.portName() = "eth1/1/1";
  port1.portState() = "ENABLED";
  port1.portOperState() = false; // Initially down

  state::SwitchState switchState;
  switchState.portMaps() = {};
  std::map<int16_t, state::PortFields> portMap;
  portMap[1] = port1;
  switchState.portMaps()["0"] = portMap;

  publishState(switchState);

  // Wait for initial update
  folly::Baton<> receivedInitialUpdate;
  folly::Baton<> receivedIncrementalUpdate;

  std::thread readerThread(
      [&wrapper, &receivedInitialUpdate, &receivedIncrementalUpdate]() {
        try {
          // Wait for initial update
          auto updates = wrapper->waitForStateUpdates();
          receivedInitialUpdate.post();

          // Wait for incremental update
          updates = wrapper->waitForStateUpdates();
          receivedIncrementalUpdate.post();
        } catch (const std::exception& e) {
          XLOG(ERR) << "[Reader] Error receiving state update: " << e.what();
        }
      });

  // Wait for initial update
  receivedInitialUpdate.wait();

  // Publish incremental update (change port oper state)
  port1.portOperState() = true; // Port comes up
  portMap[1] = port1;
  switchState.portMaps()["0"] = portMap;

  publishState(switchState);

  // Wait for incremental update
  receivedIncrementalUpdate.wait();

  // Join the thread
  if (readerThread.joinable()) {
    readerThread.join();
  }

  EXPECT_TRUE(receivedInitialUpdate.ready())
      << "Failed to receive initial state update from FSDB";
  EXPECT_TRUE(receivedIncrementalUpdate.ready())
      << "Failed to receive incremental state update from FSDB";
}

TEST_F(FsdbCgoPubSubWrapperTest, ReceiveStatsUpdate) {
  // Create stats publisher
  createStatsPublisher();

  // Create wrapper and subscribe
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");
  wrapper->subscribeStatsPath(fsdbTestServer_->getFsdbPort());

  // Give subscription time to establish
  std::this_thread::sleep_for(500ms);

  // Publish port stats
  auto portStats = makePortStats(1000, "eth1/1/1");

  // Publish the stats
  publishStats(portStats);

  // Wait for update with timeout
  std::atomic<bool> receivedUpdate{false};
  std::thread readerThread([&wrapper, &receivedUpdate]() {
    try {
      auto updates = wrapper->waitForStatsUpdates();
      XLOG(INFO) << "Received " << updates.size() << " stats updates";
      receivedUpdate = true;
    } catch (const std::exception& e) {
      XLOG(ERR) << "Error receiving stats update: " << e.what();
    }
  });

  // Wait up to 10 seconds
  for (int i = 0; i < 100 && !receivedUpdate; ++i) {
    std::this_thread::sleep_for(100ms);
  }

  if (readerThread.joinable()) {
    readerThread.join();
  }

  EXPECT_TRUE(receivedUpdate.load())
      << "Failed to receive stats update from FSDB";
}

TEST_F(FsdbCgoPubSubWrapperTest, BothStateAndStatsUpdates) {
  // Create both publishers
  createStatePublisher();
  createStatsPublisher();

  // Create wrapper and subscribe to both
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");
  wrapper->subscribeToOperState_portMaps(fsdbTestServer_->getFsdbPort());

  wrapper->subscribeStatsPath(fsdbTestServer_->getFsdbPort());

  // Give subscriptions time to establish
  std::this_thread::sleep_for(500ms);

  std::atomic<bool> receivedState{false};
  std::atomic<bool> receivedStats{false};

  // Reader thread for state
  std::thread stateReaderThread([&wrapper, &receivedState]() {
    try {
      auto updates = wrapper->waitForStateUpdates();
      XLOG(INFO) << "Received " << updates.size() << " state updates";
      receivedState = true;
    } catch (const std::exception& e) {
      XLOG(ERR) << "Error receiving state update: " << e.what();
    }
  });

  // Reader thread for stats
  std::thread statsReaderThread([&wrapper, &receivedStats]() {
    try {
      auto updates = wrapper->waitForStatsUpdates();
      XLOG(INFO) << "Received " << updates.size() << " stats updates";
      receivedStats = true;
    } catch (const std::exception& e) {
      XLOG(ERR) << "Error receiving stats update: " << e.what();
    }
  });

  // Publish state
  state::PortFields port;
  port.portId() = 1;
  port.portName() = "eth1/1/1";
  port.portState() = "ENABLED";
  port.portOperState() = true;

  state::SwitchState switchState;
  switchState.portMaps() = {};
  std::map<int16_t, state::PortFields> portMap;
  portMap[1] = port;
  switchState.portMaps()["0"] = portMap;

  publishState(switchState);

  // Publish stats
  auto portStats = makePortStats(2000, "eth1/1/1");
  publishStats(portStats);

  // Wait up to 10 seconds for both
  for (int i = 0; i < 100 && (!receivedState || !receivedStats); ++i) {
    std::this_thread::sleep_for(100ms);
  }

  if (stateReaderThread.joinable()) {
    stateReaderThread.join();
  }
  if (statsReaderThread.joinable()) {
    statsReaderThread.join();
  }

  EXPECT_TRUE(receivedState.load()) << "Failed to receive state update";
  EXPECT_TRUE(receivedStats.load()) << "Failed to receive stats update";
}

} // namespace facebook::fboss::fsdb::test
