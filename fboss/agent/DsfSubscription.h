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
      fsdb::FsdbPubSubManager* pubSubManager,
      std::string localNodeName,
      std::string remoteNodeName,
      SwitchID remoteNodeSwitchId,
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

 private:
  void handleFsdbSubscriptionStateUpdate(
      fsdb::SubscriptionState oldState,
      fsdb::SubscriptionState newState);
  void handleFsdbUpdate(fsdb::OperSubPathUnit&& operStateUnit);
  fsdb::FsdbStreamClient::State getStreamState() const;

  fsdb::FsdbPubSubManager* fsdbPubSubMgr_;
  std::string localNodeName_;
  std::string remoteNodeName_;
  SwitchID remoteNodeSwitchId_;
  folly::IPAddress localIp_;
  folly::IPAddress remoteIp_;
  SwitchStats* stats_;
  DsfSubscriberStateCb dsfSubscriberStateCb_;
  GrHoldExpiredCb grHoldExpiredCb_;
  StateUpdateCb stateUpdateCb_;
  DsfSession session_;
};

} // namespace facebook::fboss
