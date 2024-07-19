// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "folly/Synchronized.h"

#include <gtest/gtest.h>
#include <memory>

DECLARE_bool(dsf_subscriber_skip_hw_writes);
DECLARE_bool(dsf_subscriber_cache_updated_state);

namespace facebook::fboss {
class SwSwitch;
class SwitchState;
class InterfaceMap;
class SystemPortMap;
namespace fsdb {
class FsdbPubSubManager;
}

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
    if (fsdbPubSubMgr_) {
      return fsdbPubSubMgr_->getSubscriptionInfo();
    }
    return {};
  }

  std::string getClientId() const {
    if (fsdbPubSubMgr_) {
      return fsdbPubSubMgr_->getClientId();
    }
    throw FbossError("DsfSubscriber: fsdbPubSubMgr_ is null");
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
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::shared_ptr<SwitchState> cachedState_;
  std::string localNodeName_;
  folly::Synchronized<
      folly::F14FastMap<std::string, std::unique_ptr<DsfSubscription>>>
      subscriptions_;

  FRIEND_TEST(DsfSubscriberTest, updateWithRollbackProtection);
  FRIEND_TEST(DsfSubscriberTest, setupNeighbors);
};

} // namespace facebook::fboss
