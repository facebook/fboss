/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/if/FsdbModel.h" // @manual

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/agent/HwSwitchMatcher.h"

using namespace facebook::fboss;

namespace {
constexpr auto kIntfNodeStart = 100;
}

namespace facebook::fboss {
class DsfSubscriberTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    // Create a separate instance of DsfSubscriber (vs
    // using one from SwSwitch) for ease of testing.
    dsfSubscriber_ = std::make_unique<DsfSubscriber>(sw_);
    FLAGS_dsf_num_parallel_sessions_per_remote_interface_node =
        std::numeric_limits<uint32_t>::max();
  }

  HwSwitchMatcher matcher(uint32_t switchID = 0) const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(switchID)}));
  }

  void updateDsfInNode(
      MultiSwitchDsfNodeMap* dsfNodes,
      cfg::DsfNode& dsfConfig,
      bool add) {
    if (add) {
      auto dsfNode = std::make_shared<DsfNode>(SwitchID(*dsfConfig.switchId()));
      dsfNode->setName(*dsfConfig.name());
      dsfNode->setType(*dsfConfig.type());
      dsfNode->setLoopbackIps(*dsfConfig.loopbackIps());
      dsfNodes->addNode(dsfNode, matcher());
    } else {
      dsfNodes->removeNode(*dsfConfig.switchId());
    }
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
  std::unique_ptr<DsfSubscriber> dsfSubscriber_;
};

TEST_F(DsfSubscriberTest, addSubscription) {
  auto verifySubscriptionState = [&](cfg::DsfNode& nodeConfig,
                                     const auto& subscriptionInfoList) {
    auto ipv6Loopback = (*nodeConfig.loopbackIps())[0];
    auto serverStr = ipv6Loopback.substr(0, ipv6Loopback.find("/"));
    for (const auto& subscriptionInfo : subscriptionInfoList) {
      if (subscriptionInfo.server == serverStr) {
        EXPECT_EQ(subscriptionInfo.paths.size(), 3);
        EXPECT_EQ(
            subscriptionInfo.state,
            fsdb::FsdbStreamClient::State::DISCONNECTED);
        return true;
      }
    }
    return false;
  };

  auto verifyDsfSessionState = [&](cfg::DsfNode& nodeConfig,
                                   const auto dsfSessionsThrift) {
    std::set<std::string> remoteEndpoints;
    std::for_each(
        nodeConfig.loopbackIps()->begin(),
        nodeConfig.loopbackIps()->end(),
        [&](const auto loopbackSubnet) {
          auto loopbackIp = folly::IPAddress::createNetwork(
                                loopbackSubnet, -1 /*defaultCidr*/, false)
                                .first;
          remoteEndpoints.insert(DsfSubscription::makeRemoteEndpoint(
              *nodeConfig.name(), loopbackIp));
        });
    for (const auto& dsfSession : dsfSessionsThrift) {
      if (remoteEndpoints.find(*dsfSession.remoteName()) !=
          remoteEndpoints.end()) {
        EXPECT_EQ(*dsfSession.state(), DsfSessionState::CONNECT);
        return true;
      }
    }
    return false;
  };

  EXPECT_EQ(sw_->getDsfSubscriber()->getSubscriptionInfo().size(), 0);

  // Insert 2 IN nodes
  auto intfNodeCfg0 = makeDsfNodeCfg(kIntfNodeStart);
  sw_->updateStateBlocking(
      "Add IN node", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg0,
            /* add */ true);
        return newState;
      });
  EXPECT_EQ(
      sw_->getDsfSubscriber()->getSubscriptionInfo().size(),
      intfNodeCfg0.loopbackIps()->size());

  EXPECT_TRUE(verifySubscriptionState(
      intfNodeCfg0, sw_->getDsfSubscriber()->getSubscriptionInfo()));
  EXPECT_TRUE(verifyDsfSessionState(
      intfNodeCfg0, sw_->getDsfSubscriber()->getDsfSessionsThrift()));

  auto intfNodeCfg1 = makeDsfNodeCfg(kIntfNodeStart + 1);
  sw_->updateStateBlocking(
      "Add IN node", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg1,
            /* add */ true);
        return newState;
      });

  EXPECT_EQ(
      sw_->getDsfSubscriber()->getSubscriptionInfo().size(),
      intfNodeCfg0.loopbackIps()->size() + intfNodeCfg1.loopbackIps()->size());

  EXPECT_TRUE(verifySubscriptionState(
      intfNodeCfg1, sw_->getDsfSubscriber()->getSubscriptionInfo()));
  EXPECT_TRUE(verifyDsfSessionState(
      intfNodeCfg1, sw_->getDsfSubscriber()->getDsfSessionsThrift()));

  // Remove 2 IN nodes
  sw_->updateStateBlocking(
      "Remove IN nodes", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg0,
            /* add */ false);
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg1,
            /* add */ false);
        return newState;
      });
  EXPECT_EQ(sw_->getDsfSubscriber()->getSubscriptionInfo().size(), 0);
  EXPECT_EQ(sw_->getDsfSubscriber()->getDsfSessionsThrift().size(), 0);
}

TEST_F(DsfSubscriberTest, failedDsfCounter) {
  // Remove the other subscriber to avoid double counting
  dsfSubscriber_.reset();

  CounterCache counters(sw_);
  auto failedDsfCounter = SwitchStats::kCounterPrefix + "failedDsfSubscription";
  auto intfNodeCfg0 = makeDsfNodeCfg(kIntfNodeStart);
  sw_->updateStateBlocking(
      "Add IN node", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg0,
            /* add */ true);
        return newState;
      });
  counters.update();

  EXPECT_TRUE(counters.checkExist(failedDsfCounter));
  EXPECT_EQ(
      counters.value(failedDsfCounter), intfNodeCfg0.loopbackIps()->size());

  auto intfNodeCfg1 = makeDsfNodeCfg(kIntfNodeStart + 1);
  sw_->updateStateBlocking(
      "Add IN node", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg1,
            /* add */ true);
        return newState;
      });
  counters.update();

  EXPECT_EQ(
      counters.value(failedDsfCounter),
      intfNodeCfg0.loopbackIps()->size() + intfNodeCfg1.loopbackIps()->size());

  // Remove 2 IN nodes
  sw_->updateStateBlocking(
      "Remove IN nodes", [&](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg0,
            /* add */ false);
        updateDsfInNode(
            newState->getDsfNodes()->modify(&newState),
            intfNodeCfg1,
            /* add */ false);
        return newState;
      });
  counters.update();
  EXPECT_EQ(counters.value(failedDsfCounter), 0);
}
} // namespace facebook::fboss
