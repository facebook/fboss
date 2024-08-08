// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <folly/Synchronized.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/executors/thread_factory/NamedThreadFactory.h>
#include <gtest/gtest.h>
#include <memory>

DECLARE_bool(dsf_subscriber_skip_hw_writes);
DECLARE_bool(dsf_subscriber_cache_updated_state);

namespace facebook::fboss {
class SwSwitch;
class SwitchState;
class InterfaceMap;
class SystemPortMap;

class DsfSubscriber : public StateObserver {
 public:
  explicit DsfSubscriber(SwSwitch* sw);
  ~DsfSubscriber() override;
  void stateUpdated(const StateDelta& stateDelta) override;

  void stop();

  // Used in tests for asserting on modifications
  // made by DsfSubscriber
  const std::shared_ptr<SwitchState> cachedState() const {
    return cachedState_;
  }

  const std::vector<fsdb::FsdbPubSubManager::SubscriptionInfo>
  getSubscriptionInfo() const {
    std::vector<fsdb::FsdbPubSubManager::SubscriptionInfo> infos;
    auto subscriptionsLocked = subscriptions_.rlock();
    infos.reserve(subscriptionsLocked->size());
    for (const auto& [_, subscription] : *subscriptionsLocked) {
      infos.push_back(subscription->getSubscriptionInfo());
    }
    return infos;
  }

  std::string getClientId() const {
    return folly::sformat("{}:agent", localNodeName_);
  }

  std::vector<DsfSessionThrift> getDsfSessionsThrift() const;
  static std::string makeRemoteEndpoint(
      const std::string& remoteNode,
      const folly::IPAddress& remoteIP) {
    return DsfSubscription::makeRemoteEndpoint(remoteNode, remoteIP);
  }

 private:
  void updateWithRollbackProtection(
      const std::string& nodeName,
      const SwitchID& nodeSwitchId,
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
          switchId2SystemPorts,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs);
  bool isLocal(SwitchID nodeSwitchId) const;
  void processGRHoldTimerExpired(
      const std::string& nodeName,
      const std::set<SwitchID>& allNodeSwitchIDs);

  SwSwitch* sw_;
  std::shared_ptr<SwitchState> cachedState_;
  std::string localNodeName_;
  folly::Synchronized<
      folly::F14FastMap<std::string, std::unique_ptr<DsfSubscription>>>
      subscriptions_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamConnectPool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamServePool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> hwUpdatePool_;

  FRIEND_TEST(DsfSubscriberTest, updateWithRollbackProtection);
  FRIEND_TEST(DsfSubscriberTest, setupNeighbors);
};

} // namespace facebook::fboss
