// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/DsfSession.h"
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

 private:
  void scheduleUpdate(
      const std::shared_ptr<SystemPortMap>& newSysPorts,
      const std::shared_ptr<InterfaceMap>& newRifs,
      const std::string& nodeName,
      SwitchID nodeSwitchId);
  bool isLocal(SwitchID nodeSwitchId) const;
  // Paths
  static std::vector<std::string> getSystemPortsPath();
  static std::vector<std::string> getInterfacesPath();
  static std::vector<std::string> getDsfSubscriptionsPath(
      const std::string& localNodeName);
  static std::vector<std::vector<std::string>> getAllSubscribePaths(
      const std::string& localNodeName);

  SwSwitch* sw_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::shared_ptr<SwitchState> cachedState_;
  std::string localNodeName_;
  folly::Synchronized<std::unordered_map<std::string, DsfSession>> dsfSessions_;

  FRIEND_TEST(DsfSubscriberTest, scheduleUpdate);
  FRIEND_TEST(DsfSubscriberTest, setupNeighbors);
};

} // namespace facebook::fboss
