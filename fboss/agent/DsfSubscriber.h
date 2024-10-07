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

  folly::EventBase* getReconnectThreadEvb() {
    return streamConnectPool_->getEventBase();
  }

  folly::EventBase* getStreamThreadEvb() {
    return streamServePool_->getEventBase();
  }

  // Used in tests for asserting on modifications
  // made by DsfSubscriber
  const std::shared_ptr<SwitchState> cachedState() const {
    return cachedState_;
  }

  const std::vector<fsdb::SubscriptionInfo> getSubscriptionInfo() const {
    std::vector<fsdb::SubscriptionInfo> infos;
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

 private:
  void destroySubscription(std::unique_ptr<DsfSubscription> subscription);
  bool isLocal(SwitchID nodeSwitchId) const;
  SwSwitch* sw_;
  std::shared_ptr<SwitchState> cachedState_;
  std::string localNodeName_;
  folly::Synchronized<
      folly::F14FastMap<std::string, std::unique_ptr<DsfSubscription>>>
      subscriptions_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamConnectPool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamServePool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> hwUpdatePool_;
  bool stopped_{false};
};

} // namespace facebook::fboss
