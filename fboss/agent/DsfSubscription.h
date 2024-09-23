// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/thrift_cow/storage/CowStorage.h>
#include "fboss/agent/DsfSession.h"
#include "fboss/agent/FsdbAdaptedSubManager.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"

#include <string>

namespace facebook::fboss {
class SwitchStats;
class InterfaceMap;
class SystemPortMap;
class SwSwitch;
class SwitchState;
class DsfUpdateValidator;

class DsfSubscription {
 public:
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
      SwSwitch* sw);

  ~DsfSubscription();

  static std::string makeRemoteEndpoint(
      const std::string& remoteNode,
      const folly::IPAddress& remoteIP);

  DsfSessionThrift dsfSessionThrift() {
    return session_.toThrift();
  }
  void stop();

  const fsdb::SubscriptionInfo getSubscriptionInfo() const;
  // Used for tests only
  const std::shared_ptr<SwitchState> cachedState() const {
    return cachedState_;
  }

 private:
  struct DsfUpdate {
    bool isEmpty() const {
      return switchId2SystemPorts.empty() && switchId2Intfs.empty();
    }
    void clear() {
      switchId2SystemPorts.clear();
      switchId2Intfs.clear();
    }
    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;
  };
  void updateDsfState(
      const std::function<std::shared_ptr<SwitchState>(
          const std::shared_ptr<SwitchState>&)>& updateDsfStateFn);
  std::string remoteEndpointStr() const;
  void updateWithRollbackProtection(
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
          switchId2SystemPorts,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs);
  void processGRHoldTimerExpired();
  void setupSubscription();
  void tearDownSubscription();

  bool isLocal(SwitchID nodeSwitchId) const;
  void handleFsdbSubscriptionStateUpdate(
      fsdb::SubscriptionState oldState,
      fsdb::SubscriptionState newState);
  void handleFsdbUpdate(fsdb::OperSubPathUnit&& operStateUnit);
  void queueRemoteStateChanged(
      const MultiSwitchSystemPortMap& newPortMap,
      const MultiSwitchInterfaceMap& newInterfaceMap);
  void queueDsfUpdate(DsfUpdate&& dsfUpdate);

  fsdb::FsdbStreamClient::State getStreamState() const;

  fsdb::SubscriptionOptions opts_;
  folly::EventBase* hwUpdateEvb_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::unique_ptr<FsdbAdaptedSubManager> subMgr_;
  std::unique_ptr<DsfUpdateValidator> validator_;
  std::string localNodeName_;
  std::string remoteNodeName_;
  std::set<SwitchID> remoteNodeSwitchIds_;
  folly::IPAddress localIp_;
  folly::IPAddress remoteIp_;
  SwSwitch* sw_;
  DsfSession session_;
  folly::Synchronized<std::unique_ptr<DsfUpdate>> nextDsfUpdate_;
  // Cache current state of sysports and intfs received from remote
  // node. Since after the initial update we are not guaranteed to
  // receive the entire set of sysports and rifs in any update. For
  // instance on a nbr change, we will get the entire set of RIFs
  // but not sys ports. However when applying the update, we
  // apply RIFs and sys ports together. To that end we need to
  // cache the current sysports and rifs and then patch the
  // receieved update on top of it.
  // TODO: kill this code after we cutover to patch subscriptions.
  MultiSwitchSystemPortMap curMswitchSysPorts_;
  MultiSwitchInterfaceMap curMswitchIntfs_;
  bool stopped_{false};
  // Used for tests only
  std::shared_ptr<SwitchState> cachedState_;
  template <typename T>
  FRIEND_TEST(DsfSubscriptionTest, updateWithRollbackProtection);
  template <typename T>
  FRIEND_TEST(DsfSubscriptionTest, updateFailed);
  template <typename T>
  FRIEND_TEST(DsfSubscriptionTest, setupNeighbors);
};

} // namespace facebook::fboss
