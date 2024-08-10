// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AgentFsdbSyncManager.h"
#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/executors/thread_factory/NamedThreadFactory.h>
#include <gtest/gtest.h>

namespace facebook::fboss {
namespace {
auto constexpr kRemoteSwitchId = 5;
}

class DsfSubscriptionTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_publish_state_to_fsdb = true;
    FLAGS_fsdb_sync_full_state = true;
    FLAGS_dsf_subscriber_cache_updated_state = true;
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    fsdbTestServer_ = std::make_unique<fsdb::test::FsdbTestServer>();
    FLAGS_fsdbPort = fsdbTestServer_->getFsdbPort();
    pubSub_ = std::make_unique<fsdb::FsdbPubSubManager>("test-client");
    streamConnectPool_ = std::make_unique<folly::IOThreadPoolExecutor>(
        1,
        std::make_shared<folly::NamedThreadFactory>(
            "DsfSubscriberStreamConnect"));
    streamServePool_ = std::make_unique<folly::IOThreadPoolExecutor>(
        1,
        std::make_shared<folly::NamedThreadFactory>(
            "DsfSubscriberStreamServe"));
    hwUpdatePool_ = std::make_unique<folly::IOThreadPoolExecutor>(
        1, std::make_shared<folly::NamedThreadFactory>("DsfHwUpdate"));
  }

  void TearDown() override {
    fsdbTestServer_.reset();
    stopPublisher();
  }

  void createPublisher() {
    publisher_ = std::make_unique<AgentFsdbSyncManager>();
    publisher_->start();
  }

  std::shared_ptr<SwitchState> makeSwitchState() const {
    auto constexpr kSysPort = 1000;
    auto state = std::make_shared<SwitchState>();
    auto sysPorts = std::make_shared<MultiSwitchSystemPortMap>();
    auto sysPort = makeSysPort(std::nullopt, kSysPort, kRemoteSwitchId);
    sysPorts->addNode(sysPort, matcher());
    state->resetSystemPorts(sysPorts);
    auto intfs = std::make_shared<MultiSwitchInterfaceMap>();
    auto intf = std::make_shared<Interface>(
        InterfaceID(kSysPort),
        RouterID(0),
        std::optional<VlanID>(std::nullopt),
        folly::StringPiece("1001"),
        folly::MacAddress{},
        9000,
        false,
        false,
        cfg::InterfaceType::SYSTEM_PORT);
    intf->setScope(cfg::Scope::GLOBAL);
    intfs->addNode(intf, matcher());
    state->resetIntfs(intfs);
    return state;
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
      typename DsfSubscription::GrHoldExpiredCb grHoldExpiredCb) {
    fsdb::SubscriptionOptions opts{
        "test-sub", false /* subscribeStats */, FLAGS_dsf_gr_hold_time};
    return std::make_unique<DsfSubscription>(
        std::move(opts),
        streamConnectPool_->getEventBase(),
        streamServePool_->getEventBase(),
        hwUpdatePool_->getEventBase(),
        "local",
        "remote",
        folly::IPAddress("::1"),
        folly::IPAddress("::1"),
        sw_,
        std::move(grHoldExpiredCb));
  }

  HwSwitchMatcher matcher(uint32_t switchID = kRemoteSwitchId) const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(switchID)}));
  }

  std::shared_ptr<SystemPortMap> getRemoteSystemPorts() const {
    return cachedState()->getRemoteSystemPorts()->getAllNodes();
  }
  std::shared_ptr<InterfaceMap> getRemoteInterfaces() const {
    return cachedState()->getRemoteInterfaces()->getAllNodes();
  }
  std::shared_ptr<SwitchState> cachedState() const {
    return subscription_->cachedState();
  }
  DsfSessionState dsfSessionState() const {
    return *subscription_->dsfSessionThrift().state();
  }

 protected:
  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<AgentFsdbSyncManager> publisher_;
  std::unique_ptr<fsdb::FsdbPubSubManager> pubSub_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamConnectPool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamServePool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> hwUpdatePool_;
  std::optional<ReconnectingThriftClient::ServerOptions> serverOptions_;
  std::unique_ptr<HwTestHandle> handle_;
  std::shared_ptr<DsfSubscription> subscription_;
  SwSwitch* sw_;
};

TEST_F(DsfSubscriptionTest, Connect) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  subscription_ = createSubscription(
      // GrHoldExpiredCb
      []() {});
  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(cachedState());
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  updateDsfSubscriberState("local", fsdb::FsdbSubscriptionState::CONNECTED);
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(dsfSessionState(), DsfSessionState::ESTABLISHED));
}

TEST_F(DsfSubscriptionTest, ConnectDisconnect) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  subscription_ = createSubscription(
      // GrHoldExpiredCb
      []() {});

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(cachedState());
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  stopPublisher();
  EXPECT_EQ(dsfSessionState(), DsfSessionState::CONNECT);
}

TEST_F(DsfSubscriptionTest, GR) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);
  FLAGS_dsf_gr_hold_time = 5;
  bool grExpired = false;
  int subStateUpdates = 0;
  int updates = 0;
  subscription_ = createSubscription(
      // GrHoldExpiredCb
      [&]() { grExpired = true; });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(cachedState());
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  int subStateUpdatesBefore = subStateUpdates;
  stopPublisher(true);
  createPublisher();
  publishSwitchState(state);

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(cachedState());
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    ASSERT_EVENTUALLY_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });
  // should not have gotten callback from the gr
  EXPECT_EQ(grExpired, false);

  stopPublisher(true);
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(dsfSessionState(), DsfSessionState::CONNECT);
    EXPECT_EVENTUALLY_EQ(grExpired, true);
  });
}

TEST_F(DsfSubscriptionTest, DataUpdate) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);

  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  subscription_ = createSubscription(
      // GrHoldExpiredCb
      []() {});

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(cachedState());
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  auto sysPort2 =
      makeSysPort(std::nullopt, SystemPortID(1002), kRemoteSwitchId);
  auto portMap = state->getSystemPorts()->modify(&state);
  portMap->addNode(sysPort2, matcher());
  publishSwitchState(state);

  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 2));
}

} // namespace facebook::fboss
