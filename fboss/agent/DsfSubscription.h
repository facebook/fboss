// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/DsfSession.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <string>

namespace facebook::fboss {
class SwitchStats;
class InterfaceMap;
class SystemPortMap;

class DsfSubscription {
 public:
  using DsfSubscriberStateCb = std::function<void(
      fsdb::FsdbSubscriptionState /* oldState */,
      fsdb::FsdbSubscriptionState /* newState */)>;
  using GrHoldExpiredCb = std::function<void()>;
  using StateUpdateCb = std::function<void(
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>&)>;

  DsfSubscription(
      fsdb::SubscriptionOptions options,
      folly::EventBase* reconnectEvb,
      folly::EventBase* subscriberEvb,
      folly::EventBase* hwUpdateEvb,
      std::string localNodeName,
      std::string remoteNodeName,
      folly::IPAddress localIp,
      folly::IPAddress remoteIp,
      SwitchStats* stats,
      DsfSubscriberStateCb dsfSubscriberStateCb,
      GrHoldExpiredCb grHoldExpiredCb,
      StateUpdateCb stateUpdateCb);

  ~DsfSubscription();

  static std::string makeRemoteEndpoint(
      const std::string& remoteNode,
      const folly::IPAddress& remoteIP);

  DsfSessionThrift dsfSessionThrift() {
    return session_.toThrift();
  }

  const fsdb::FsdbPubSubManager::SubscriptionInfo getSubscriptionInfo() const;

 private:
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
  folly::IPAddress localIp_;
  folly::IPAddress remoteIp_;
  SwitchStats* stats_;
  DsfSubscriberStateCb dsfSubscriberStateCb_;
  GrHoldExpiredCb grHoldExpiredCb_;
  StateUpdateCb stateUpdateCb_;
  DsfSession session_;
};

} // namespace facebook::fboss
