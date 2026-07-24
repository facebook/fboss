// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/cgo/FsdbCgoPubSubWrapper.h"

#include <fmt/core.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
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

  EXPECT_NO_THROW(
      wrapper->subscribeStatsPath({"agent"}, fsdbTestServer_->getFsdbPort()));
  EXPECT_TRUE(wrapper->hasStatsSubscription());

  // Second subscription should log warning but not throw
  EXPECT_NO_THROW(
      wrapper->subscribeStatsPath({"agent"}, fsdbTestServer_->getFsdbPort()));
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

  EXPECT_NO_THROW(
      wrapper->subscribeStatsPath({"agent"}, fsdbTestServer_->getFsdbPort()));
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
  int32_t initialPortId = -1;
  int32_t deltaPortId = -1;
  bool deltaOperState = false;

  std::thread readerThread([&wrapper,
                            &receivedInitialUpdate,
                            &receivedIncrementalUpdate,
                            &initialPortId,
                            &deltaPortId,
                            &deltaOperState]() {
    try {
      // Wait for initial update; capture the port_id carried in the dump.
      auto updates = wrapper->waitForStateUpdates();
      if (!updates.empty()) {
        initialPortId = std::get<1>(updates.front());
      }
      receivedInitialUpdate.post();

      // Wait for incremental update; port_id must be present on deltas too.
      updates = wrapper->waitForStateUpdates();
      if (!updates.empty()) {
        deltaPortId = std::get<1>(updates.front());
        deltaOperState = std::get<2>(updates.front());
      }
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
  // port_id (== portMaps key 1) must be carried in both the initial dump and
  // the delta update.
  EXPECT_EQ(initialPortId, 1);
  EXPECT_EQ(deltaPortId, 1);
  EXPECT_TRUE(deltaOperState);
}

TEST_F(FsdbCgoPubSubWrapperTest, ReceiveStatsUpdate) {
  // Create stats publisher
  createStatsPublisher();

  // Create wrapper and subscribe
  auto wrapper = std::make_unique<FsdbCgoPubSubWrapper>("test-client");
  wrapper->subscribeStatsPath({"agent"}, fsdbTestServer_->getFsdbPort());

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

  wrapper->subscribeStatsPath({"agent"}, fsdbTestServer_->getFsdbPort());

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

// ============================================================================
// Integration tests for the extern "C" CGO surface (the actual API AMD calls).
// ============================================================================

// Helper: build a SwitchState with N ports (all ENABLED, oper UP).
state::SwitchState makeSwitchStateWithPorts(int numPorts) {
  state::SwitchState switchState;
  std::map<int16_t, state::PortFields> portMap;
  for (int i = 1; i <= numPorts; ++i) {
    state::PortFields p;
    p.portId() = i;
    p.portName() = fmt::format("eth1/{}/1", i);
    p.portState() = "ENABLED";
    p.portOperState() = true;
    portMap[i] = std::move(p);
  }
  switchState.portMaps()["0"] = std::move(portMap);
  return switchState;
}

TEST_F(FsdbCgoPubSubWrapperTest, ExternCRoundTrip) {
  // Publish-before-subscribe: FSDB delivers the latest state to the new
  // subscriber via initial sync. Avoids racing the publish against subscriber
  // connect (which can take seconds under CI load).
  createStatePublisher();
  publishState(makeSwitchStateWithPorts(/*numPorts=*/1));

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("extern-c-roundtrip");
  ASSERT_NE(handle, nullptr);
  SubscribeToPortMaps(handle, nullptr, fsdbTestServer_->getFsdbPort());
  ASSERT_EQ(HasStateSubscription(handle), 1);

  FsdbPortStateUpdate out[16];
  int32_t count = WaitForStateUpdates(handle, out, 16);
  ASSERT_GE(count, 1);
  EXPECT_STREQ(out[0].port_name, "eth1/1/1");
  EXPECT_EQ(out[0].port_id, 1);
  EXPECT_EQ(out[0].oper_state, 1);

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, GetPortSnapshotRoundTrip) {
  // GetPortSnapshot is a synchronous one-shot GET (no subscription) returning
  // every port as (port_name, port_id, oper_state).
  createStatePublisher();
  publishState(makeSwitchStateWithPorts(/*numPorts=*/2));

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("get-port-snapshot");
  ASSERT_NE(handle, nullptr);

  // The publish may not be served the instant we connect; poll until present.
  FsdbPortStateUpdate out[16];
  int32_t count = 0;
  for (int attempt = 0; attempt < 50 && count < 2; ++attempt) {
    count = GetPortSnapshot(
        handle, nullptr, fsdbTestServer_->getFsdbPort(), out, 16);
    if (count < 2) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  ASSERT_EQ(count, 2);

  std::map<std::string, std::pair<int32_t, int32_t>> got;
  for (int32_t i = 0; i < count; ++i) {
    got[out[i].port_name] = {out[i].port_id, out[i].oper_state};
  }
  const std::map<std::string, std::pair<int32_t, int32_t>> expected = {
      {"eth1/1/1", {1, 1}},
      {"eth1/2/1", {2, 1}},
  };
  EXPECT_EQ(got, expected);

  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, GetPortSnapshotBadArgs) {
  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("get-port-snapshot-badargs");
  ASSERT_NE(handle, nullptr);
  FsdbPortStateUpdate out[1];
  EXPECT_EQ(GetPortSnapshot(nullptr, nullptr, 5908, out, 1), -1);
  EXPECT_EQ(
      GetPortSnapshot(
          handle, nullptr, fsdbTestServer_->getFsdbPort(), nullptr, 1),
      -1);
  EXPECT_EQ(
      GetPortSnapshot(handle, nullptr, fsdbTestServer_->getFsdbPort(), out, 0),
      -1);
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, ExternCExplicitHostRoundTrip) {
  // Same as ExternCRoundTrip but passes an explicit host string: it must be
  // plumbed into ConnectionOptions rather than ignored.
  createStatePublisher();
  publishState(makeSwitchStateWithPorts(/*numPorts=*/1));

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("extern-c-hostport");
  ASSERT_NE(handle, nullptr);
  SubscribeToPortMaps(handle, "::1", fsdbTestServer_->getFsdbPort());
  ASSERT_EQ(HasStateSubscription(handle), 1);

  FsdbPortStateUpdate out[16];
  int32_t count = WaitForStateUpdates(handle, out, 16);
  ASSERT_GE(count, 1);
  EXPECT_STREQ(out[0].port_name, "eth1/1/1");
  EXPECT_EQ(out[0].port_id, 1);

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, ExternCPortIdFromFieldNotMapKey) {
  // port_id must be sourced from PortFields.portId(), not the portMaps map key.
  // Publish a port whose map key (7) differs from its portId field (42).
  createStatePublisher();
  state::SwitchState switchState;
  state::PortFields port;
  port.portId() = 42;
  port.portName() = "eth1/1/1";
  port.portState() = "ENABLED";
  port.portOperState() = true;
  std::map<int16_t, state::PortFields> portMap;
  portMap[7] = std::move(port);
  switchState.portMaps()["0"] = std::move(portMap);
  publishState(switchState);

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("extern-c-portid-field");
  ASSERT_NE(handle, nullptr);
  SubscribeToPortMaps(handle, nullptr, fsdbTestServer_->getFsdbPort());
  ASSERT_EQ(HasStateSubscription(handle), 1);

  FsdbPortStateUpdate out[16];
  int32_t count = WaitForStateUpdates(handle, out, 16);
  ASSERT_GE(count, 1);
  EXPECT_STREQ(out[0].port_name, "eth1/1/1");
  EXPECT_EQ(out[0].port_id, 42); // PortFields.portId(), not the map key (7)

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, ExternCShutdownWakesBlockedWaiter) {
  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("extern-c-shutdown");
  ASSERT_NE(handle, nullptr);
  SubscribeToPortMaps(handle, nullptr, fsdbTestServer_->getFsdbPort());
  ASSERT_EQ(HasStateSubscription(handle), 1);

  // Condition-based: waiter posts when WaitForStateUpdates returns. Allows up
  // to ~5x the wrapper's 100ms internal poll interval to wake on Shutdown.
  std::atomic<int32_t> waiterResult{-2};
  folly::Baton<> waiterDone;
  std::thread waiter([&] {
    FsdbPortStateUpdate out[16];
    waiterResult.store(WaitForStateUpdates(handle, out, 16));
    waiterDone.post();
  });

  ShutdownFsdbWrapper(handle);
  ASSERT_TRUE(waiterDone.try_wait_for(500ms))
      << "waiter did not return within 500ms of ShutdownFsdbWrapper";
  EXPECT_EQ(waiterResult.load(), 0);

  waiter.join();
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, ExternCOverflowStaysQueued) {
  // Asserts the no-data-loss property of WaitFor*'s no-over-drain semantic:
  // if FSDB delivers N port updates and we ask for them with max_count=2 at
  // a time, we receive all N across however many WaitFor* calls it takes.
  // Stronger atomicity (all-on-queue-at-once → first call returns 2, second
  // returns 1) cannot be reliably tested against FSDB because subscriber
  // CONNECTED handshake is non-deterministic under CI load — so we test the
  // weaker "no data loss" property instead.
  createStatePublisher();
  publishState(makeSwitchStateWithPorts(/*numPorts=*/3));

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("extern-c-overflow");
  ASSERT_NE(handle, nullptr);
  SubscribeToPortMaps(handle, nullptr, fsdbTestServer_->getFsdbPort());

  // Capture the (port_name -> port_id) pairing to confirm port_id is carried
  // and not cross-wired across ports when draining the bounded queue.
  std::map<std::string, int32_t> received;
  const auto deadline = std::chrono::steady_clock::now() + 30s;
  while (received.size() < 3 && std::chrono::steady_clock::now() < deadline) {
    FsdbPortStateUpdate out[2];
    int32_t n = WaitForStateUpdates(handle, out, 2);
    ASSERT_GE(n, 0); // never -1 (no error) and never panics
    for (int32_t i = 0; i < n; ++i) {
      received[out[i].port_name] = out[i].port_id;
    }
  }
  const std::map<std::string, int32_t> expected{
      {"eth1/1/1", 1}, {"eth1/2/1", 2}, {"eth1/3/1", 3}};
  EXPECT_EQ(received, expected)
      << "Expected all 3 ports with matching port_ids; got " << received.size();

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, ExternCGetConnectionStateReportsDataReceived) {
  createStatePublisher();
  publishState(makeSwitchStateWithPorts(/*numPorts=*/1));

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle = CreateFsdbWrapper("extern-c-connstate");
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(GetConnectionState(handle), FSDB_CONNECTION_DISCONNECTED);

  SubscribeToPortMaps(handle, nullptr, fsdbTestServer_->getFsdbPort());

  // Data was published, so the state should advance past CONNECTED to
  // DATA_RECEIVED once the initial sync is delivered.
  int32_t state = GetConnectionState(handle);
  const auto deadline = std::chrono::steady_clock::now() + 30s;
  while (state != FSDB_CONNECTION_DATA_RECEIVED &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(50ms);
    state = GetConnectionState(handle);
  }
  EXPECT_EQ(state, FSDB_CONNECTION_DATA_RECEIVED);

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
}

TEST_F(
    FsdbCgoPubSubWrapperTest,
    ExternCGetConnectionStateUnreachableStaysConnecting) {
  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle =
      CreateFsdbWrapper("extern-c-connstate-unreachable");
  ASSERT_NE(handle, nullptr);

  // Port 1 has no FSDB server; the connect never succeeds so no state-change
  // callback fires and the wrapper stays in its initial CONNECTING state.
  SubscribeToPortMaps(handle, "::1", 1);

  const auto until = std::chrono::steady_clock::now() + 500ms;
  while (std::chrono::steady_clock::now() < until) {
    ASSERT_NE(GetConnectionState(handle), FSDB_CONNECTION_CONNECTED);
    std::this_thread::sleep_for(50ms);
  }
  EXPECT_EQ(GetConnectionState(handle), FSDB_CONNECTION_CONNECTING);

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
}

TEST_F(FsdbCgoPubSubWrapperTest, DestroyDuringSnapshotDelivery) {
  // Overflow the state queue so the FSDB callback is parked in enqueueState
  // (in flight) at teardown; 2x because DSPSCQueue over-provisions its bound.
  const int kNumPorts = 2 * FsdbCgoPubSubWrapper::kStateQueueCapacity;
  createStatePublisher();
  publishState(makeSwitchStateWithPorts(kNumPorts));

  FsdbInit(FSDB_CGO_ABI_VERSION);
  FsdbWrapperHandle handle =
      CreateFsdbWrapper("extern-c-destroy-during-snapshot");
  ASSERT_NE(handle, nullptr);
  SubscribeToPortMaps(handle, nullptr, fsdbTestServer_->getFsdbPort());
  ASSERT_EQ(HasStateSubscription(handle), 1);

  // Wait until delivery starts so the callback is mid-snapshot at teardown.
  FsdbPortStateUpdate out[16];
  const auto deadline = std::chrono::steady_clock::now() + 30s;
  int32_t first = 0;
  while (first <= 0 && std::chrono::steady_clock::now() < deadline) {
    first = WaitForStateUpdates(handle, out, 16);
  }
  ASSERT_GT(first, 0) << "snapshot delivery never started";
  std::this_thread::sleep_for(200ms);

  ShutdownFsdbWrapper(handle);
  DestroyFsdbWrapper(handle);
  // No crash under ASAN is the assertion.
  SUCCEED();
}

} // namespace facebook::fboss::fsdb::test
