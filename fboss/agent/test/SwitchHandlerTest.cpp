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
#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <algorithm>

using facebook::fboss::HwSwitchMatcher;
using facebook::fboss::SwitchID;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{
      std::unordered_set<SwitchID>{SwitchID(0), SwitchID(1)}};
}
} // namespace

using namespace facebook::fboss;

class SwSwitchHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto agentConfig = createAgentConfig();
    agentDirUtil_ = std::make_unique<AgentDirectoryUtil>(
        tmpDir_.path().string() + "/volatile",
        tmpDir_.path().string() + "/persistent");
    sw_ = createSwSwitchWithMultiSwitch(&agentConfig, agentDirUtil_.get());
    sw_->getHwSwitchHandler()->start();
  }

 protected:
  AgentConfig createAgentConfig() {
    auto config = testConfigA();
    config.switchSettings()->switchIdToSwitchInfo() = {
        std::make_pair(0, createSwitchInfo(cfg::SwitchType::NPU)),
        std::make_pair(1, createSwitchInfo(cfg::SwitchType::NPU)),
    };
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
    addSwitchSettings(SwitchID(0));
    addSwitchSettings(SwitchID(1));
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
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    addRandomDelay();
    // Switch 1 cancels request
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(0);
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
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // request next state delta. the empty oper passed serves as success
    // indicator for previous delta
    operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // this request will be cancelled
    operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    // this request will be cancelled
    EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

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
  folly::Baton<> client1Baton;
  folly::Baton<> client2Baton;
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

  std::thread stateUpdateThread(
      [this, &delta, &stateV1, &delta2, &stateV2, &client2Baton]() {
        /* client 1 connects */
        getHwSwitchHandler()->waitUntilHwSwitchConnected();
        /* state update should succeed */
        auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
        EXPECT_EQ(stateReturned, stateV1);
        /* let client2 start. it should get full sync */
        client2Baton.wait();
        auto stateReturned2 = getHwSwitchHandler()->stateChanged(delta2, true);
        EXPECT_EQ(stateReturned2, stateV2);
        getHwSwitchHandler()->stop();
      });

  auto clientThreadBody =
      [this, &delta, &delta2, &delta3, &client1Baton, &client2Baton](
          int64_t switchId) {
        int64_t ackNum{0};
        OperDeltaFilter filter((SwitchID(switchId)));
        // connect and get next state delta
        auto getEmptyOper = []() {
          auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
          operDelta->operDelta() = fsdb::OperDelta();
          return operDelta;
        };
        if (switchId == 1) {
          // wait for first client to send request
          client1Baton.wait();
          // signal to server that second client is ready
          client2Baton.post();
        }
        auto operDelta = getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
        // should get a valid result
        CHECK(operDelta.operDelta().is_set());
        if (switchId == 0) {
          // initial request from client 1 completed. client2 starts now
          client1Baton.post();

          // client 1 should get initial oper delta
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));

          // request next oper delta for client 1
          operDelta = getHwSwitchHandler()->getNextStateOperDelta(
              switchId, getEmptyOper(), ackNum++);
          CHECK(operDelta.operDelta().is_set());
          // client 1 should get incremental update
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta2.getOperDelta(), 1));
        } else {
          // client 2 should get full oper delta as this is the first update
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta3.getOperDelta(), 1));
        }
        operDelta = getHwSwitchHandler()->getNextStateOperDelta(
            switchId, getEmptyOper(), ackNum++);
      };
  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

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
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    // update should rollback
    EXPECT_EQ(stateReturned, stateV0);
    getHwSwitchHandler()->stop();
  });
  auto clientThreadBody =
      [this, &stateV0, &stateV1, &stateV2, &delta2, &delta3](int64_t switchId) {
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
        if (switchId == 0) {
          /* return success */
          operDelta = getHwSwitchHandler()->getNextStateOperDelta(
              switchId, getEmptyOper(), ackNum++);
          // server should return rollback oper delta
          CHECK(operDelta.operDelta().is_set());
          auto expectedDelta = StateDelta(stateV1, stateV0);
          EXPECT_EQ(
              operDelta.operDelta(),
              *filter.filter(expectedDelta.getOperDelta(), 1));
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
              *filter.filter(expectedDelta.getOperDelta(), 1));
          operDelta = getHwSwitchHandler()->getNextStateOperDelta(
              switchId, getEmptyOper(), ackNum++);
        }
      };
  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

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
  folly::Baton<> serverBaton;
  folly::Baton<> serverRestartBaton;
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
  auto delta3 = StateDelta(stateV2, stateV3);
  auto delta4 = StateDelta(stateV0, stateV3);

  auto agentConfig = createAgentConfig();
  auto agentDirUtil = AgentDirectoryUtil(
      tmpDir_.path().string() + "/volatile",
      tmpDir_.path().string() + "/persist");
  auto newSwSwitch = createSwSwitchWithMultiSwitch(&agentConfig, &agentDirUtil);
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
                                 &serverBaton,
                                 &serverRestartBaton,
                                 &newHwSwitchHandler]() {
    getHwSwitchHandler()->waitUntilHwSwitchConnected();
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    // Switch 2 cancels request
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(1);
    // Switch 1 continues to operate
    stateReturned = getHwSwitchHandler()->stateChanged(delta2, true);
    EXPECT_EQ(stateReturned, stateV2);
    // wait for switch 2 to reconnect
    serverBaton.post();
    stateReturned = getHwSwitchHandler()->stateChanged(delta3, true);
    EXPECT_EQ(stateReturned, stateV3);
    getHwSwitchHandler()->stop();
    // server restarts
    serverRestartBaton.post();
    newHwSwitchHandler->waitUntilHwSwitchConnected();
    stateReturned = newHwSwitchHandler->stateChanged(delta4, true);
    newHwSwitchHandler->stop();
  });

  auto clientThreadBody = [this,
                           &delta,
                           &delta2,
                           &delta3,
                           &delta4,
                           &serverBaton,
                           &serverRestartBaton,
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
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));

    if (switchId == 0) {
      // request next state delta. the empty oper passed serves as success
      // indicator for previous delta
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(
          operDelta.operDelta(), *filter.filter(delta2.getOperDelta(), 1));
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(
          operDelta.operDelta(), *filter.filter(delta3.getOperDelta(), 1));
      // this request will be cancelled
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
    } else {
      // response for cancelled oper request
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), ackNum++);
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());

      // wait for server to finish second update which updates
      // only client1
      serverBaton.wait();
      // switch 2 reconnects
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), 0 /*lastUpdateSeqNum*/);
      // expect full response
      EXPECT_EQ(
          operDelta.operDelta(), *filter.filter(delta4.getOperDelta(), 1));
      // Server should continue from last update seqnum
      EXPECT_EQ(operDelta.seqNum().value(), ackNum);

      // this request will be cancelled
      operDelta = getHwSwitchHandler()->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
      serverRestartBaton.wait();
      operDelta = newHwSwitchHandler->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      // server should send full response again
      EXPECT_EQ(
          operDelta.operDelta(), *filter.filter(delta4.getOperDelta(), 1));
      // seq number will reset
      EXPECT_EQ(operDelta.seqNum().value(), 1);
      // this request will be cancelled
      operDelta = newHwSwitchHandler->getNextStateOperDelta(
          switchId, getEmptyOper(), operDelta.seqNum().value());
      EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
    }
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}

TEST_F(SwSwitchHandlerTest, switchRunStateTest) {
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = getInitialTestState();

  auto delta = StateDelta(stateV0, stateV1);
  std::thread stateUpdateThread([this, &delta, &stateV1]() {
    auto checkState = [&](auto state) {
      WITH_RETRIES({
        auto hwSwitchState = getHwSwitchHandler()->getHwSwitchRunStates();
        for (const auto& [id, runState] : hwSwitchState) {
          EXPECT_EVENTUALLY_EQ(runState, state);
        }
      });
    };
    getHwSwitchHandler()->waitUntilHwSwitchConnected();
    checkState(SwitchRunState::INITIALIZED);
    auto stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    checkState(SwitchRunState::CONFIGURED);
    stateReturned = getHwSwitchHandler()->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(0);
    getHwSwitchHandler()->notifyHwSwitchGracefulExit(1);
    checkState(SwitchRunState::EXITING);
    getHwSwitchHandler()->stop();
  });

  auto clientThreadBody = [this, &delta](int64_t switchId) {
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
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // request next state delta. the empty oper passed serves as success
    // indicator for previous delta
    operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // this request will be cancelled
    operDelta = getHwSwitchHandler()->getNextStateOperDelta(
        switchId, getEmptyOper(), ackNum++);
    // this request will be cancelled
    EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
  };

  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}
