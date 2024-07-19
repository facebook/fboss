// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AgentFsdbSyncManager.h"
#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

class DsfSubscriptionTest : public ::testing::Test {
 public:
  void SetUp() override {
    fsdbTestServer_ = std::make_unique<fsdb::test::FsdbTestServer>();
    FLAGS_fsdbPort = fsdbTestServer_->getFsdbPort();
    pubSub_ = std::make_unique<fsdb::FsdbPubSubManager>("test-client");
    switchStats_ = std::make_unique<SwitchStats>(1);
    FLAGS_publish_state_to_fsdb = true;
    FLAGS_fsdb_sync_full_state = true;
  }

  void TearDown() override {
    fsdbTestServer_.reset();
    stopPublisher();
  }

  void createPublisher() {
    publisher_ = std::make_unique<AgentFsdbSyncManager>();
    publisher_->start();
  }

  void publishSwitchState(std::shared_ptr<SwitchState> state) {
    CHECK(publisher_);
    publisher_->stateUpdated(StateDelta(std::shared_ptr<SwitchState>(), state));
  }

  void updateDsfSubscriberState(
      const std::string& nodeName,
      fsdb::FsdbSubscriptionState newState) {
    publisher_->updateDsfSubscriberState(
        DsfSubscription::makeRemoteEndpoint(nodeName, folly::IPAddress("::1")),
        fsdb::FsdbSubscriptionState::DISCONNECTED, // old state doesn't matter
        newState);
  }

  void stopPublisher(bool gr = false) {
    if (publisher_) {
      publisher_->stop(gr);
      publisher_.reset();
    }
  }

  std::unique_ptr<DsfSubscription> createSubscription(
      typename DsfSubscription::DsfSubscriberStateCb dsfSubscriberStateCb,
      typename DsfSubscription::GrHoldExpiredCb grHoldExpiredCb,
      typename DsfSubscription::StateUpdateCb stateUpdateCb) {
    return std::make_unique<DsfSubscription>(
        pubSub_.get(),
        "local",
        "remote",
        SwitchID(0),
        folly::IPAddress("::1"),
        folly::IPAddress("::1"),
        switchStats_.get(),
        std::move(dsfSubscriberStateCb),
        std::move(grHoldExpiredCb),
        std::move(stateUpdateCb));
  }

  HwSwitchMatcher matcher(uint32_t switchID = 0) const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(switchID)}));
  }

 private:
  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<AgentFsdbSyncManager> publisher_;
  std::unique_ptr<fsdb::FsdbPubSubManager> pubSub_;
  std::unique_ptr<SwitchStats> switchStats_;
  std::optional<ReconnectingThriftClient::ServerOptions> serverOptions_;
};

TEST_F(DsfSubscriptionTest, Connect) {
  createPublisher();
  auto state = std::make_shared<SwitchState>();
  publishSwitchState(state);
  std::optional<fsdb::FsdbSubscriptionState> lastState;
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  auto subscription = createSubscription(
      // DsfSubscriberStateCb
      [&lastState](
          fsdb::FsdbSubscriptionState, fsdb::FsdbSubscriptionState newState) {
        lastState = newState;
      },
      // GrHoldExpiredCb
      []() {},
      // StateUpdateCb
      [&](const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
              switchId2SystemPorts,
          const std::map<SwitchID, std::shared_ptr<InterfaceMap>>&
              switchId2Intfs) {
        recvSysPorts = switchId2SystemPorts;
        recvIntfs = switchId2Intfs;
      });
  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(lastState.has_value());
    ASSERT_EVENTUALLY_EQ(
        lastState.value(), fsdb::FsdbSubscriptionState::CONNECTED);
    ASSERT_EVENTUALLY_TRUE(recvSysPorts.has_value());
    ASSERT_EVENTUALLY_TRUE(recvIntfs.has_value());
    ASSERT_EVENTUALLY_EQ(recvSysPorts->size(), 0);
    ASSERT_EVENTUALLY_EQ(recvIntfs->size(), 0);
    EXPECT_EQ(
        *subscription->dsfSessionThrift().state(),
        DsfSessionState::WAIT_FOR_REMOTE);
  });

  updateDsfSubscriberState("local", fsdb::FsdbSubscriptionState::CONNECTED);
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      *subscription->dsfSessionThrift().state(), DsfSessionState::ESTABLISHED));
}

TEST_F(DsfSubscriptionTest, ConnectDisconnect) {
  createPublisher();
  std::optional<fsdb::FsdbSubscriptionState> lastState;
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  auto subscription = createSubscription(
      // DsfSubscriberStateCb
      [&lastState](
          fsdb::FsdbSubscriptionState, fsdb::FsdbSubscriptionState newState) {
        lastState = newState;
      },
      // GrHoldExpiredCb
      []() {},
      // StateUpdateCb
      [&](const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
              switchId2SystemPorts,
          const std::map<SwitchID, std::shared_ptr<InterfaceMap>>&
              switchId2Intfs) {
        recvSysPorts = switchId2SystemPorts;
        recvIntfs = switchId2Intfs;
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(lastState.has_value());
    ASSERT_EVENTUALLY_EQ(
        lastState.value(), fsdb::FsdbSubscriptionState::CONNECTED);
    ASSERT_EVENTUALLY_TRUE(recvSysPorts.has_value());
    ASSERT_EVENTUALLY_TRUE(recvIntfs.has_value());
    ASSERT_EVENTUALLY_EQ(recvSysPorts->size(), 0);
    ASSERT_EVENTUALLY_EQ(recvIntfs->size(), 0);
    EXPECT_EQ(
        *subscription->dsfSessionThrift().state(),
        DsfSessionState::WAIT_FOR_REMOTE);
  });

  stopPublisher();
  WITH_RETRIES_N_TIMED(
      10,
      std::chrono::milliseconds(
          100), // check aggressively in case we try to reconnect
      ASSERT_EVENTUALLY_EQ(
          lastState.value(), fsdb::FsdbSubscriptionState::DISCONNECTED));

  EXPECT_EQ(
      *subscription->dsfSessionThrift().state(), DsfSessionState::CONNECT);
}

TEST_F(DsfSubscriptionTest, GR) {
  createPublisher();
  FLAGS_dsf_gr_hold_time = 5;
  std::optional<fsdb::FsdbSubscriptionState> lastState;
  bool grExpired = false;
  int subStateUpdates = 0;
  int updates = 0;
  auto subscription = createSubscription(
      // DsfSubscriberStateCb
      [&](fsdb::FsdbSubscriptionState, fsdb::FsdbSubscriptionState newState) {
        lastState = newState;
        subStateUpdates++;
      },
      // GrHoldExpiredCb
      [&]() { grExpired = true; },
      // StateUpdateCb
      [&](const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&,
          const std::map<SwitchID, std::shared_ptr<InterfaceMap>>&) {
        updates++;
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(lastState.has_value());
    ASSERT_EVENTUALLY_EQ(
        lastState.value(), fsdb::FsdbSubscriptionState::CONNECTED);
    ASSERT_EVENTUALLY_EQ(updates, 1);
  });

  int subStateUpdatesBefore = subStateUpdates;
  stopPublisher(true);
  createPublisher();

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(updates, 2);
    ASSERT_EVENTUALLY_EQ(
        lastState.value(), fsdb::FsdbSubscriptionState::CONNECTED);
  });
  // should not have gotten callback from the gr
  EXPECT_EQ(grExpired, false);

  stopPublisher(true);
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        lastState.value(), fsdb::FsdbSubscriptionState::DISCONNECTED);
    EXPECT_EVENTUALLY_EQ(grExpired, true);
  });
}

TEST_F(DsfSubscriptionTest, DataUpdate) {
  createPublisher();
  auto state = std::make_shared<SwitchState>();

  auto sysPort1 = std::make_shared<SystemPort>(SystemPortID(1001));
  auto intf1 = std::make_shared<Interface>(
      InterfaceID(1001),
      RouterID(0),
      std::optional<VlanID>(std::nullopt),
      folly::StringPiece("1001"),
      folly::MacAddress{},
      9000,
      false,
      false,
      cfg::InterfaceType::SYSTEM_PORT);
  state->getInterfaces()->addNode(intf1, matcher());
  state->getSystemPorts()->addNode(sysPort1, matcher());
  publishSwitchState(state);

  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  auto subscription = createSubscription(
      // DsfSubscriberStateCb
      [](fsdb::FsdbSubscriptionState, fsdb::FsdbSubscriptionState) {},
      // GrHoldExpiredCb
      []() {},
      // StateUpdateCb
      [&](const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
              switchId2SystemPorts,
          const std::map<SwitchID, std::shared_ptr<InterfaceMap>>&
              switchId2Intfs) {
        recvSysPorts = switchId2SystemPorts;
        recvIntfs = switchId2Intfs;
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(recvSysPorts.has_value());
    ASSERT_EVENTUALLY_TRUE(recvIntfs.has_value());
    ASSERT_EVENTUALLY_EQ(recvSysPorts->size(), 1);
    ASSERT_EVENTUALLY_EQ(recvIntfs->size(), 1);
    ASSERT_EVENTUALLY_EQ(recvSysPorts->at(SwitchID(0))->size(), 1);
  });

  auto sysPort2 = std::make_shared<SystemPort>(SystemPortID(1002));
  auto portMap = state->getSystemPorts()->modify(&state);
  portMap->addNode(sysPort2, matcher());
  publishSwitchState(state);

  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(recvSysPorts->at(SwitchID(0))->size(), 2));
}

} // namespace facebook::fboss
