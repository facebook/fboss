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

#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"
#include "fboss/agent/test/TestUtils.h"

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
    const std::map<int64_t, cfg::SwitchInfo> switchInfoMap = {
        {0, cfg::SwitchInfo()}, {1, cfg::SwitchInfo()}};
    std::optional<cfg::SdkVersion> sdkVersion = cfg::SdkVersion();
    sdkVersion->asicSdk() = "testVersion";
    sdkVersion->saiSdk() = "testSAIVersion";
    hwSwitchHandler_ = std::make_unique<MultiHwSwitchHandler>(
        switchInfoMap,
        [](const SwitchID& switchId,
           const cfg::SwitchInfo& info,
           SwSwitch* sw) {
          return std::make_unique<
              facebook::fboss::NonMonolithicHwSwitchHandler>(
              switchId, info, sw);
        },
        nullptr,
        sdkVersion);
    hwSwitchHandler_->start();
  }

 protected:
  std::shared_ptr<SwitchState> getInitialTestState() {
    auto state = std::make_shared<SwitchState>();
    auto newSwitchSettings = std::make_shared<SwitchSettings>();
    auto multiSwitchSwitchSettings = std::make_unique<MultiSwitchSettings>();
    multiSwitchSwitchSettings->addNode(
        HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}))
            .matcherString(),
        newSwitchSettings);
    multiSwitchSwitchSettings->addNode(
        HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(1)}))
            .matcherString(),
        newSwitchSettings);
    state->resetSwitchSettings(std::move(multiSwitchSwitchSettings));
    auto aclEntry = make_shared<AclEntry>(0, std::string("acl1"));
    auto acls = state->getAcls();
    acls->addNode(aclEntry, scope());
    state->resetAcls(acls);
    return state;
  }

  std::unique_ptr<MultiHwSwitchHandler> hwSwitchHandler_{nullptr};
};

TEST_F(SwSwitchHandlerTest, GetOperDelta) {
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = getInitialTestState();

  auto addRandomDelay = []() {
    std::this_thread::sleep_for(std::chrono::milliseconds{random() % 100});
  };

  auto delta = StateDelta(stateV0, stateV1);
  std::thread stateUpdateThread([this, &delta, &addRandomDelay, &stateV1]() {
    hwSwitchHandler_->waitUntilHwSwitchConnected();
    addRandomDelay();
    auto stateReturned = hwSwitchHandler_->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    stateReturned = hwSwitchHandler_->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    addRandomDelay();
    // Switch 1 cancels request
    hwSwitchHandler_->notifyHwSwitchGracefulExit(0);
    // Switch 2 has pending request but server stops
    hwSwitchHandler_->stop();
  });

  auto clientThreadBody = [this, &delta, &addRandomDelay](int64_t switchId) {
    OperDeltaFilter filter((SwitchID(switchId)));
    // connect and get next state delta
    addRandomDelay();
    auto getEmptyOper = []() {
      auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
      operDelta->operDelta() = fsdb::OperDelta();
      return operDelta;
    };
    auto operDelta = hwSwitchHandler_->getNextStateOperDelta(
        switchId, getEmptyOper(), true /*initialSync*/);
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // request next state delta. the empty oper passed serves as success
    // indicator for previous delta
    operDelta = hwSwitchHandler_->getNextStateOperDelta(
        switchId, getEmptyOper(), false /*initialSync*/);
    EXPECT_EQ(operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));
    // this request will be cancelled
    operDelta = hwSwitchHandler_->getNextStateOperDelta(
        switchId, getEmptyOper(), false /*initialSync*/);
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
  std::thread serverThread(
      [&]() { EXPECT_FALSE(hwSwitchHandler_->waitUntilHwSwitchConnected()); });
  std::thread serverStopThread([&]() { hwSwitchHandler_->stop(); });
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
        hwSwitchHandler_->waitUntilHwSwitchConnected();
        /* state update should succeed */
        auto stateReturned = hwSwitchHandler_->stateChanged(delta, true);
        EXPECT_EQ(stateReturned, stateV1);
        /* let client2 start. it should get full sync */
        client2Baton.wait();
        auto stateReturned2 = hwSwitchHandler_->stateChanged(delta2, true);
        EXPECT_EQ(stateReturned2, stateV2);
        hwSwitchHandler_->stop();
      });

  auto clientThreadBody =
      [this, &delta, &delta2, &delta3, &client1Baton, &client2Baton](
          int64_t switchId) {
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
        auto operDelta = hwSwitchHandler_->getNextStateOperDelta(
            switchId, getEmptyOper(), true /*initialSync*/);
        // should get a valid result
        CHECK(operDelta.operDelta().is_set());
        if (switchId == 0) {
          // initial request from client 1 completed. client2 starts now
          client1Baton.post();

          // client 1 should get initial oper delta
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));

          // request next oper delta for client 1
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          CHECK(operDelta.operDelta().is_set());
          // client 1 should get incremental update
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta2.getOperDelta(), 1));
        } else {
          // client 2 should get full oper delta as this is the first update
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta3.getOperDelta(), 1));
        }
        operDelta = hwSwitchHandler_->getNextStateOperDelta(
            switchId, getEmptyOper(), false /*initialSync*/);
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
    hwSwitchHandler_->waitUntilHwSwitchConnected();
    auto stateReturned = hwSwitchHandler_->stateChanged(delta, true);
    // update should rollback
    EXPECT_EQ(stateReturned, stateV0);
    hwSwitchHandler_->stop();
  });
  auto clientThreadBody =
      [this, &stateV0, &stateV1, &stateV2, &delta2, &delta3](int64_t switchId) {
        OperDeltaFilter filter((SwitchID(switchId)));
        // connect and get next state delta
        auto getEmptyOper = []() {
          auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
          operDelta->operDelta() = fsdb::OperDelta();
          return operDelta;
        };
        auto operDelta = hwSwitchHandler_->getNextStateOperDelta(
            switchId, getEmptyOper(), true /*initialSync*/);
        CHECK(operDelta.operDelta().is_set());
        if (switchId == 0) {
          /* return success */
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          // server should return rollback oper delta
          CHECK(operDelta.operDelta().is_set());
          auto expectedDelta = StateDelta(stateV1, stateV0);
          EXPECT_EQ(
              operDelta.operDelta(),
              *filter.filter(expectedDelta.getOperDelta(), 1));
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
        } else {
          /* return failure with partial oper delta */
          auto operDeltaRet = std::make_unique<multiswitch::StateOperDelta>();
          operDeltaRet->operDelta() = delta2.getOperDelta();
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, std::move(operDeltaRet), false /*initialSync*/);
          CHECK(operDelta.operDelta().is_set());

          // server should return a rollback oper which remove the partial
          // update
          auto expectedDelta = StateDelta(stateV2, stateV0);
          EXPECT_EQ(
              operDelta.operDelta(),
              *filter.filter(expectedDelta.getOperDelta(), 1));
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
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

  std::thread stateUpdateThread([this,
                                 &delta,
                                 &delta2,
                                 &delta3,
                                 &stateV1,
                                 &stateV2,
                                 &stateV3,
                                 &serverBaton]() {
    hwSwitchHandler_->waitUntilHwSwitchConnected();
    auto stateReturned = hwSwitchHandler_->stateChanged(delta, true);
    EXPECT_EQ(stateReturned, stateV1);
    // Switch 2 cancels request
    hwSwitchHandler_->notifyHwSwitchGracefulExit(1);
    // Switch 1 continues to operate
    stateReturned = hwSwitchHandler_->stateChanged(delta2, true);
    EXPECT_EQ(stateReturned, stateV2);
    // wait for switch 2 to reconnect
    serverBaton.post();
    stateReturned = hwSwitchHandler_->stateChanged(delta3, true);
    EXPECT_EQ(stateReturned, stateV3);
    hwSwitchHandler_->stop();
  });

  auto clientThreadBody =
      [this, &delta, &delta2, &delta3, &delta4, &serverBaton](
          int64_t switchId) {
        OperDeltaFilter filter((SwitchID(switchId)));
        // connect and get next state delta
        auto getEmptyOper = []() {
          auto operDelta = std::make_unique<multiswitch::StateOperDelta>();
          operDelta->operDelta() = fsdb::OperDelta();
          return operDelta;
        };
        auto operDelta = hwSwitchHandler_->getNextStateOperDelta(
            switchId, getEmptyOper(), true /*initialSync*/);
        EXPECT_EQ(
            operDelta.operDelta(), *filter.filter(delta.getOperDelta(), 1));

        if (switchId == 0) {
          // request next state delta. the empty oper passed serves as success
          // indicator for previous delta
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta2.getOperDelta(), 1));
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta3.getOperDelta(), 1));
          // this request will be cancelled
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
        } else {
          // response for cancelled oper request
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());

          // wait for server to finish second update which updates
          // only client1
          serverBaton.wait();
          // switch 2 reconnects
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), true /*initialSync*/);
          // expect full response
          EXPECT_EQ(
              operDelta.operDelta(), *filter.filter(delta4.getOperDelta(), 1));
          // this request will be cancelled
          operDelta = hwSwitchHandler_->getNextStateOperDelta(
              switchId, getEmptyOper(), false /*initialSync*/);
          EXPECT_EQ(operDelta.operDelta(), fsdb::OperDelta());
        }
      };

  std::thread clientRequestThread1([&]() { clientThreadBody(0); });
  std::thread clientRequestThread2([&]() { clientThreadBody(1); });

  stateUpdateThread.join();
  clientRequestThread1.join();
  clientRequestThread2.join();
}
