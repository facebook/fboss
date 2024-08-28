/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/MultiHwSwitchHandler.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/mnpu/MultiSwitchHwSwitchHandler.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <algorithm>

using facebook::fboss::HwSwitchMatcher;
using facebook::fboss::SwitchID;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{
      std::unordered_set<SwitchID>{SwitchID(1), SwitchID(2)}};
}
} // namespace

using namespace facebook::fboss;

class SwSwitchHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_multi_switch = true;
    auto agentConfig = createAgentConfig();
    agentDirUtil_ = std::make_unique<AgentDirectoryUtil>(
        tmpDir_.path().string() + "/volatile",
        tmpDir_.path().string() + "/persistent");
    sw_ = createSwSwitchWithMultiSwitch(
        &agentConfig,
        agentDirUtil_.get(),
        [](const SwitchID& switchId,
           const cfg::SwitchInfo& info,
           SwSwitch* sw) {
          return std::make_unique<MultiSwitchHwSwitchHandler>(
              switchId, info, sw);
        });
    sw_->getHwSwitchHandler()->start();
  }

 protected:
  AgentConfig createAgentConfig() {
    auto config = testConfigB();
    std::optional<cfg::SdkVersion> sdkVersion = cfg::SdkVersion();
    sdkVersion->asicSdk() = "testVersion";
    sdkVersion->saiSdk() = "testSAIVersion";
    config.sdkVersion() = sdkVersion.value();
    auto agentConfig = createEmptyAgentConfig()->thrift;
    agentConfig.sw() = config;
    return AgentConfig(agentConfig);
  }

  std::shared_ptr<SwitchState> getInitialTestState() {
    auto state = std::make_shared<SwitchState>();
    auto multiSwitchSwitchSettings = std::make_unique<MultiSwitchSettings>();
    auto addSwitchSettings = [&multiSwitchSwitchSettings](SwitchID switchId) {
      auto newSwitchSettings = std::make_shared<SwitchSettings>();
      newSwitchSettings->setSwitchIdToSwitchInfo(
          {std::make_pair(switchId, createSwitchInfo(cfg::SwitchType::NPU))});
      multiSwitchSwitchSettings->addNode(
          HwSwitchMatcher(std::unordered_set<SwitchID>({switchId}))
              .matcherString(),
          newSwitchSettings);
    };
    addSwitchSettings(SwitchID(1));
    addSwitchSettings(SwitchID(2));
    state->resetSwitchSettings(std::move(multiSwitchSwitchSettings));
    auto aclEntry = make_shared<AclEntry>(0, std::string("acl1"));
    auto acls = state->getAcls();
    acls->addNode(aclEntry, scope());
    state->resetAcls(acls);
    return state;
  }

  MultiHwSwitchHandler* getHwSwitchHandler() {
    return sw_->getHwSwitchHandler();
  }

  std::unique_ptr<SwSwitch> sw_;
  folly::test::TemporaryDirectory tmpDir_;
  std::unique_ptr<AgentDirectoryUtil> agentDirUtil_;
};

TEST_F(SwSwitchHandlerTest, GetOperDelta) {
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = getInitialTestState();

  auto addRandomDelay = []() {
    std::this_thread::sleep_for(std::chrono::milliseconds{random() % 100});
  };

  auto delta = StateDelta(stateV0, stateV1);
  std::thread stateUpdateThread([this, &delta, &addRandomDelay, &stateV1]() {
    getHwSwitchHandler()->waitUntilHwSwitchConnected();
    addRandomDelay();
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, false);
    EXPECT_EQ(stateReturned, stateV1);
    // Switch 1 cancels request
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(1);
    // Switch 2 has pending request but server stops
    getHwSwitchHandler()->stop();
  });

  auto clientThreadBody = [this, &delta, &addRandomDelay](int64_t switchId) {
    int64_t ackNum{0};
    OperDeltaFilter filter((SwitchID(switchId)));
    // connect and get next state delta
    addRandomDelay();
    auto getEmptyOper = []() {
      auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
      operDelta->operDelta() = fsdb::OperDelta();
      return operDelta;
    };
    auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    EXPECT_EQ(
        operDelta.operDelta(),
        *filter.filterWithSwitchStateRootPath(delta.getOperDelta()));
    // request next state delta. the empty oper passed serves as success
    // indicator for previous delta
    operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    // this request will be cancelled
    EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

TEST_F(SwSwitchHandlerTest, cancelHwSwitchWait) {
  std::thread serverThread([&]() {
    EXPECT_FALSE(getHwSwitchHandler()->waitUntilHwSwitchConnected());
  });
  std::thread serverStopThread([&]() { getHwSwitchHandler()->stop(); });
  serverThread.join();
  serverStopThread.join();
}

/*
 * Test with 2 clients.
 * - Client 1 alone connects initially. Update succeeds
 * - Client 2 connects later. Should get a full sync while
 *   client 1 gets an incremental update.
 */
TEST_F(SwSwitchHandlerTest, partialUpdateAndFullSync) {
  folly::Baton<> client1UpdateCompletedBaton;
  folly::Baton<> client2FinishedBaton;
  folly::Baton<> client1InitialSyncBaton;
  auto stateV0 = std::make_shared<SwitchState>();
  stateV0->publish();
  auto stateV1 = getInitialTestState();
  stateV1->publish();
  /* initial update delta */
  auto delta = StateDelta(stateV0, stateV1);
  auto stateV2 = stateV1->clone();
  auto aclEntry2 = make_shared<AclEntry>(1, std::string("acl2"));
  auto aclsV2 = stateV2->getAcls()->modify(&stateV2);
  aclsV2->addNode(aclEntry2, scope());
  stateV2->publish();
  /* second update delta */
  auto delta2 = StateDelta(stateV1, stateV2);
  /* full update delta */
  auto delta3 = StateDelta(stateV0, stateV2);

  getHwSwitchHandler()->connected(SwitchID(1));
  sw_->init(HwWriteBehavior::WRITE, SwitchFlags::DEFAULT);
  sw_->initialConfigApplied(std::chrono::steady_clock::now());
  getHwSwitchHandler()->stateChanged(
      StateDelta(std::make_shared<SwitchState>(), sw_->getState()), false);

  std::thread stateUpdateThread([this,
                                 &delta,
                                 &stateV1,
                                 &delta2,
                                 &stateV2,
                                 &client2FinishedBaton,
                                 &client1InitialSyncBaton]() {
    // wait for client 1 to do initial sync
    client1InitialSyncBaton.wait();
    /* state update should succeed */
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    auto stateReturned2 = getHwSwitchHandler()->stateChanged(delta2, true);
    EXPECT_EQ(stateReturned2, stateV2);
    // wait for client 2 to do full sync
    client2FinishedBaton.wait();
    getHwSwitchHandler()->stop();
  });

  auto clientThreadBody = [this,
                           &delta,
                           &delta2,
                           &delta3,
                           &client1UpdateCompletedBaton,
                           &client2FinishedBaton,
                           &client1InitialSyncBaton](int64_t switchId) {
    int64_t ackNum{0};
    OperDeltaFilter filter((SwitchID(switchId)));
    auto getEmptyOper = []() {
      auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
      operDelta->operDelta() = fsdb::OperDelta();
      return operDelta;
    };
    // connect and get next state delta
    if (switchId == 2) {
      // wait for first client to send request
      client1UpdateCompletedBaton.wait();
    }
    // initial sync request
    auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    // should get a valid result
    EXPECT_TRUE(operDelta.operDelta().is_set());
    if (switchId == 1) {
      auto initialDelta =
          StateDelta(std::make_shared<SwitchState>(), sw_->getState());
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(initialDelta.getOperDelta()));
      // signal to server to start further updates
      client1InitialSyncBaton.post();
      // request next delta which also serves as ack for initial delta
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      // client 1 should get incremental oper delta
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta.getOperDelta()));
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      // client 1 should get next incremental oper delta
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta2.getOperDelta()));
      // signal client 2 to start
      client1UpdateCompletedBaton.post();
      // ack for previous oper delta
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
    } else {
      // client 2 should get full oper delta
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta3.getOperDelta()));
      // client 2 done. sever can shut down
      client2FinishedBaton.post();
      // ack for initial delta
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
    }
  };
  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

/*
 * Test with 2 clients.
 * - Both clients request oper delta.
 * - Client 1 updates delta to hw fully.
 * - Client 2 updates delta to hw partially.
 * - Server should rollback oper delta. Client1 gets
 *  full rollback update while client2 gets partial rollback update.
 */
TEST_F(SwSwitchHandlerTest, rollbackFailedHwSwitchUpdate) {
  auto stateV0 = std::make_shared<SwitchState>();
  stateV0->publish();
  auto stateV1 = getInitialTestState();
  stateV1->publish();
  auto delta = StateDelta(stateV0, stateV1);
  auto stateV2 = stateV1->clone();
  auto aclEntry2 = make_shared<AclEntry>(1, std::string("acl2"));
  auto aclsV2 = stateV2->getAcls()->modify(&stateV2);
  aclsV2->addNode(aclEntry2, scope());
  stateV2->publish();
  auto delta2 = StateDelta(stateV1, stateV2);
  auto delta3 = StateDelta(stateV0, stateV2);

  std::thread stateUpdateThread([this, &delta, &stateV0]() {
    getHwSwitchHandler()->waitUntilHwSwitchConnected();
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, false);
    // update should rollback
    EXPECT_EQ(stateReturned, stateV0);
    getHwSwitchHandler()->stop();
  });
  auto clientThreadBody = [this,
                           &stateV0,
                           &stateV1,
                           &stateV2,
                           &delta2,
                           &delta3](int64_t switchId) {
    int64_t ackNum{0};
    OperDeltaFilter filter((SwitchID(switchId)));
    // connect and get next state delta
    auto getEmptyOper = []() {
      auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
      operDelta->operDelta() = fsdb::OperDelta();
      return operDelta;
    };
    auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    CHECK(operDelta.operDelta().is_set());
    if (switchId == 1) {
      /* return success */
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      // server should return rollback oper delta
      CHECK(operDelta.operDelta().is_set());
      auto expectedDelta = StateDelta(stateV1, stateV0);
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(expectedDelta.getOperDelta()));
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
    } else {
      /* return failure with partial oper delta */
      auto operDeltaRet = std::make_unique<multiswitch::StateOperDelta>();
      operDeltaRet->operDelta() = delta2.getOperDelta();
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, std::move(operDeltaRet), ackNum++);
      CHECK(operDelta.operDelta().is_set());

      // server should return a rollback oper which remove the partial
      // update
      auto expectedDelta = StateDelta(stateV2, stateV0);
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(expectedDelta.getOperDelta()));
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
    }
  };
  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

/*
 * Test with 2 clients.
 * - Both clients request oper delta and updates state successfully
 * - Client 2 disconnects. Update should continue with client 1
 * - Client 1 reconnects and gets a full oper sync
 */
TEST_F(SwSwitchHandlerTest, reconnectingHwSwitch) {
  folly::Baton<> serverUpdateCompletedBaton;
  folly::Baton<> serverRestartBaton;
  folly::Baton<> client1InitialSyncBaton;
  folly::Baton<> client2InitialSyncBaton;
  folly::Baton<> clientResyncBaton;
  auto stateV0 = std::make_shared<SwitchState>();
  stateV0->publish();
  auto stateV1 = getInitialTestState();
  stateV1->publish();
  auto stateV2 = stateV1->clone();
  auto aclEntry2 = make_shared<AclEntry>(1, std::string("acl2"));
  auto aclsV2 = stateV2->getAcls()->modify(&stateV2);
  aclsV2->addNode(aclEntry2, scope());
  stateV2->publish();
  auto stateV3 = stateV2->clone();
  auto aclEntry3 = make_shared<AclEntry>(2, std::string("acl3"));
  auto aclsV3 = stateV3->getAcls()->modify(&stateV3);
  aclsV3->addNode(aclEntry3, scope());
  stateV3->publish();
  auto delta = StateDelta(stateV0, stateV1);
  auto delta2 = StateDelta(stateV1, stateV2);
  auto delta2Full = StateDelta(stateV0, stateV2);
  auto delta3 = StateDelta(stateV2, stateV3);
  auto delta4 = StateDelta(stateV0, stateV3);

  getHwSwitchHandler()->connected(SwitchID(1));
  getHwSwitchHandler()->connected(SwitchID(2));
  sw_->init(HwWriteBehavior::WRITE, SwitchFlags::DEFAULT);
  sw_->initialConfigApplied(std::chrono::steady_clock::now());
  getHwSwitchHandler()->stateChanged(
      StateDelta(std::make_shared<SwitchState>(), sw_->getState()), false);

  auto agentConfig = createAgentConfig();
  auto agentDirUtil = AgentDirectoryUtil(
      tmpDir_.path().string() + "/volatile",
      tmpDir_.path().string() + "/persist");
  auto newSwSwitch = createSwSwitchWithMultiSwitch(
      &agentConfig,
      &agentDirUtil,
      [](const SwitchID& switchId, const cfg::SwitchInfo& info, SwSwitch* sw) {
        return std::make_unique<MultiSwitchHwSwitchHandler>(switchId, info, sw);
      });
  auto newHwSwitchHandler = newSwSwitch->getHwSwitchHandler();
  newHwSwitchHandler->start();

  std::thread stateUpdateThread([this,
                                 &delta,
                                 &delta2,
                                 &delta3,
                                 &delta4,
                                 &stateV1,
                                 &stateV2,
                                 &stateV3,
                                 &serverUpdateCompletedBaton,
                                 &serverRestartBaton,
                                 &client1InitialSyncBaton,
                                 &client2InitialSyncBaton,
                                 &clientResyncBaton,
                                 &newHwSwitchHandler]() {
    // wait for initial sync to complete on both switches
    client1InitialSyncBaton.wait();
    client2InitialSyncBaton.wait();
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    // Switch 2 cancels request
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(2);
    // Switch 1 continues to operate
    stateReturned = getHwSwitchHandler()->stateChanged(delta2, true);
    EXPECT_EQ(stateReturned, stateV2);
    serverUpdateCompletedBaton.post();
    // wait for client 2 to resync
    clientResyncBaton.wait();
    // resume normal updates
    stateReturned = getHwSwitchHandler()->stateChanged(delta3, true);
    EXPECT_EQ(stateReturned, stateV3);
    getHwSwitchHandler()->stop();
    // server restarts
    serverRestartBaton.post();
    stateReturned = newHwSwitchHandler->stateChanged(delta4, false);
    newHwSwitchHandler->stop();
  });

  auto clientThreadBody = [this,
                           &delta,
                           &delta2,
                           &delta2Full,
                           &delta3,
                           &delta4,
                           &serverUpdateCompletedBaton,
                           &serverRestartBaton,
                           &client1InitialSyncBaton,
                           &client2InitialSyncBaton,
                           &clientResyncBaton,
                           &newHwSwitchHandler](int64_t switchId) {
    int64_t ackNum{0};
    OperDeltaFilter filter((SwitchID(switchId)));
    // connect and get next state delta
    auto getEmptyOper = []() {
      auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
      operDelta->operDelta() = fsdb::OperDelta();
      return operDelta;
    };
    auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    auto initialDelta =
        StateDelta(std::make_shared<SwitchState>(), sw_->getState());
    EXPECT_EQ(
        operDelta.operDelta(),
        *filter.filterWithSwitchStateRootPath(initialDelta.getOperDelta()));

    if (switchId == 1) {
      client1InitialSyncBaton.post();
      // request next state delta. the empty oper passed serves as success
      // indicator for previous delta
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta.getOperDelta()));
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta2.getOperDelta()));
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta3.getOperDelta()));
      // this request will be cancelled
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
    } else {
      client2InitialSyncBaton.post();
      // request next state delta. the empty oper passed serves as success
      // indicator for previous delta
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta.getOperDelta()));
      // ack for previous request. this request will be cancelled
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      // response for cancelled oper request
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());

      // wait for server to finish second update which updates
      // only client1
      serverUpdateCompletedBaton.wait();
      // switch 2 reconnects
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), 0 /*lastUpdateSeqNum*/);
      // expect full response
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta2Full.getOperDelta()));
      // Server should continue from last update seqnum
      EXPECT_EQ(operDelta.seqNum().value(), ackNum);

      clientResyncBaton.post();
      // normal update resumes
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta3.getOperDelta()));

      // this request will be cancelled
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());

      // wait for server to restart
      serverRestartBaton.wait();
      operDelta = newHwSwitchHandler->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      // server should send full response again
      EXPECT_EQ(
          operDelta.operDelta(),
          *filter.filterWithSwitchStateRootPath(delta4.getOperDelta()));
      // seq number will reset
      EXPECT_EQ(operDelta.seqNum().value(), 1);
      // ack - this request will be cancelled
      operDelta = newHwSwitchHandler->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
    }
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

TEST_F(SwSwitchHandlerTest, switchRunStateTest) {
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = getInitialTestState();
  folly::Baton<> client1StartBaton;
  folly::Baton<> client2StartBaton;

  auto delta = StateDelta(stateV0, stateV1);
  auto checkState = [&](auto state,
                        std::optional<int32_t> switchId = std::nullopt) {
    WITH_RETRIES({
      auto hwSwitchState = getHwSwitchHandler()->getHwSwitchRunStates();
      for (const auto& [id, runState] : hwSwitchState) {
        if (switchId.has_value() && id != switchId.value()) {
          continue;
        }
        EXPECT_EVENTUALLY_EQ(runState, state);
      }
    });
  };
  checkState(SwitchRunState::UNINITIALIZED);
  getHwSwitchHandler()->stateChanged(delta, true);

  std::thread stateUpdateThread([this,
                                 &delta,
                                 &stateV1,
                                 &checkState,
                                 &client1StartBaton,
                                 &client2StartBaton]() {
    getHwSwitchHandler()->waitUntilHwSwitchConnected();
    checkState(SwitchRunState::INITIALIZED);
    client1StartBaton.post();
    client2StartBaton.post();
    checkState(SwitchRunState::CONFIGURED);
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, false);
    EXPECT_EQ(stateReturned, stateV1);
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(1);
    checkState(SwitchRunState::EXITING, 0);
    getHwSwitchHandler()->stop();
  });

  auto clientThreadBody =
      [this, &delta, &client1StartBaton, &client2StartBaton](int64_t switchId) {
        int64_t ackNum{0};
        OperDeltaFilter filter((SwitchID(switchId)));
        // connect and get next state delta
        auto getEmptyOper = []() {
          auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
          operDelta->operDelta() = fsdb::OperDelta();
          return operDelta;
        };
        auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        EXPECT_EQ(
            operDelta.operDelta(),
            *filter.filterWithSwitchStateRootPath(delta.getOperDelta()));
        if (switchId == 1) {
          client1StartBaton.wait();
        } else {
          client2StartBaton.wait();
        }
        // request next state delta. the empty oper passed serves as success
        // indicator for previous delta
        operDelta = getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        EXPECT_EQ(
            operDelta.operDelta(),
            *filter.filterWithSwitchStateRootPath(delta.getOperDelta()));
        // this request will be cancelled
        operDelta = getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        // this request will be cancelled
        EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
      };

  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

// client should be able to get full sync when switch is in configured
// state without having to wait for next state delta
TEST_F(SwSwitchHandlerTest, initialSync) {
  folly::Baton<> client1Baton;
  folly::Baton<> client2Baton;
  auto sw = sw_.get();
  getHwSwitchHandler()->connected(SwitchID(1));
  sw_->init(HwWriteBehavior::WRITE, SwitchFlags::DEFAULT);
  getHwSwitchHandler()->stateChanged(
      StateDelta(std::make_shared<SwitchState>(), sw->getState()), false);
  sw_->initialConfigApplied(std::chrono::steady_clock::now());

  auto clientThreadBody =
      [&sw, &client1Baton, &client2Baton](int64_t switchId) {
        int64_t ackNum{0};
        OperDeltaFilter filter((SwitchID(switchId)));
        // connect and get next state delta
        auto getEmptyOper = []() {
          auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
          operDelta->operDelta() = fsdb::OperDelta();
          return operDelta;
        };
        auto operDelta = sw->getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        EXPECT_TRUE(*operDelta.isFullState());
        auto initialDelta =
            StateDelta(std::make_shared<SwitchState>(), sw->getState());
        EXPECT_EQ(
            operDelta.operDelta(),
            *filter.filterWithSwitchStateRootPath(initialDelta.getOperDelta()));
        if (switchId == 1) {
          client1Baton.post();
        } else {
          client2Baton.post();
        }
        // ack for initial delta
        operDelta = sw->getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
      };

  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });
  client1Baton.wait();
  client2Baton.wait();
  sw->getHwSwitchHandler()->stop();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

// client should be able to get full sync in the following case
// - swswitch is not ready when client connects
// - swswitch moves to configured state and client gets a full sync
TEST_F(SwSwitchHandlerTest, initialSyncSwSwitchNotConfigured) {
  folly::Baton<> client1Baton;
  folly::Baton<> client2Baton;
  auto sw = sw_.get();

  auto clientThreadBody =
      [&sw, &client1Baton, &client2Baton](int64_t switchId) {
        int64_t ackNum{0};
        OperDeltaFilter filter((SwitchID(switchId)));
        // connect and get next state delta
        auto getEmptyOper = []() {
          auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
          operDelta->operDelta() = fsdb::OperDelta();
          return operDelta;
        };
        auto initialOperDelta = sw->getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        EXPECT_TRUE(*initialOperDelta.isFullState());
        if (switchId == 1) {
          client1Baton.post();
        } else {
          client2Baton.post();
        }
        // ack for initial delta
        auto operDelta = sw->getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        auto initialDelta =
            StateDelta(std::make_shared<SwitchState>(), sw->getState());
        EXPECT_EQ(
            initialOperDelta.operDelta(),
            *filter.filterWithSwitchStateRootPath(initialDelta.getOperDelta()));
      };

  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });
  getHwSwitchHandler()->waitUntilHwSwitchConnected();
  sw_->init(HwWriteBehavior::WRITE, SwitchFlags::PUBLISH_STATS);
  getHwSwitchHandler()->stateChanged(
      StateDelta(std::make_shared<SwitchState>(), sw->getState()), false);
  sw_->initialConfigApplied(std::chrono::steady_clock::now());

  // check multi_switch status
  CounterCache counters(sw_.get());
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "multi_switch.sum.60", FLAGS_multi_switch);

  client1Baton.wait();
  client2Baton.wait();
  waitForStateUpdates(sw);
  sw->getHwSwitchHandler()->stop();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

TEST_F(SwSwitchHandlerTest, connectionStatusCount) {
  CounterCache counters(sw_.get());
  auto checkConnectionStatus = [&](const auto& status) {
    AgentStats agentStats;
    getHwSwitchHandler()->fillHwAgentConnectionStatus(agentStats);
    EXPECT_EQ(agentStats.hwagentConnectionStatus()[0], status);
  };
  checkConnectionStatus(0);
  getHwSwitchHandler()->connected(SwitchID(1));
  getHwSwitchHandler()->connected(SwitchID(2));
  counters.update();
  // Ideally the absolute value of connection status can be checked here.
  // However it would fail if all tests are run in a single shot as the
  // counter updates from previous tests can be carried over to this test.
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "switch.0.connection_status", 1);
  checkConnectionStatus(1);
  getHwSwitchHandler()->disconnected(SwitchID(1));
  counters.update();
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "switch.0.connection_status", -1);
  checkConnectionStatus(0);
}

TEST_F(SwSwitchHandlerTest, operAckTimeoutCount) {
  FLAGS_oper_delta_ack_timeout = 5;
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = getInitialTestState();

  auto delta = StateDelta(stateV0, stateV1);
  std::thread stateUpdateThread([this, &delta]() {
    getHwSwitchHandler()->waitUntilHwSwitchConnected();
    CounterCache counters(sw_.get());
    counters.update();
    auto prevCounter = counters.value(
        SwitchStats::kCounterPrefix + "switch." + folly::to<std::string>(0) +
        ".hwupdate_timeouts.sum.60");
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, false);
    WITH_RETRIES({
      counters.update();
      EXPECT_EVENTUALLY_EQ(
          counters.value(
              SwitchStats::kCounterPrefix + "switch." +
              folly::to<std::string>(0) + ".hwupdate_timeouts.sum.60"),
          prevCounter + 1);
    });
    getHwSwitchHandler()->stop();
  });

  auto clientThreadBody = [this](int64_t switchId) {
    int64_t ackNum{0};
    OperDeltaFilter filter((SwitchID(switchId)));
    // connect and get next state delta
    auto getEmptyOper = []() {
      auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
      operDelta->operDelta() = fsdb::OperDelta();
      return operDelta;
    };
    auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    // only second switch sends ack
    if (switchId == 2) {
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
    }
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(1); });
  std::thread clientRequestThread2([&]() { clientThreadBody(2); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}
