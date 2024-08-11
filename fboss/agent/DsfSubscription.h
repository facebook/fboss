// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/DsfSession.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <string>

namespace facebook::fboss {
class SwitchStats;
class InterfaceMap;
class SystemPortMap;
class SwSwitch;
class SwitchState;

class DsfSubscription {
 public:
  using GrHoldExpiredCb = std::function<void()>;

  DsfSubscription(
      fsdb::SubscriptionOptions options,
      folly::EventBase* reconnectEvb,
      folly::EventBase* subscriberEvb,
      folly::EventBase* hwUpdateEvb,
      std::string localNodeName,
      std::string remoteNodeName,
      std::set<SwitchID> remoteNodeSwitchIds,
      folly::IPAddress localIp,
      folly::IPAddress remoteIp,
      SwSwitch* sw,
      GrHoldExpiredCb grHoldExpiredCb);

  ~DsfSubscription();

  static std::string makeRemoteEndpoint(
      const std::string& remoteNode,
      const folly::IPAddress& remoteIP);

  DsfSessionThrift dsfSessionThrift() {
    return session_.toThrift();
  }

  const fsdb::FsdbPubSubManager::SubscriptionInfo getSubscriptionInfo() const;
  // Used for tests only
  const std::shared_ptr<SwitchState> cachedState() const {
    return cachedState_;
  }

 private:
  void updateWithRollbackProtection(
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
          switchId2SystemPorts,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs);
  bool isLocal(SwitchID nodeSwitchId) const;
  void handleFsdbSubscriptionStateUpdate(
      fsdb::SubscriptionState oldState,
      fsdb::SubscriptionState newState);
  void handleFsdbUpdate(fsdb::OperSubPathUnit&& operStateUnit);
  fsdb::FsdbStreamClient::State getStreamState() const;

  fsdb::SubscriptionOptions opts_;
  folly::EventBase* hwUpdateEvb_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::string localNodeName_;
  std::string remoteNodeName_;
  std::set<SwitchID> remoteNodeSwitchIds_;
  folly::IPAddress localIp_;
  folly::IPAddress remoteIp_;
  SwSwitch* sw_;
  GrHoldExpiredCb grHoldExpiredCb_;
  DsfSession session_;
  // Used for tests only
  std::shared_ptr<SwitchState> cachedState_;
  FRIEND_TEST(DsfSubscriptionTest, updateWithRollbackProtection);
  FRIEND_TEST(DsfSubscriptionTest, setupNeighbors);
};

} // namespace facebook::fboss
