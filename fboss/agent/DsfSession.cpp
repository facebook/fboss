// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSession.h"

#include "folly/logging/xlog.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

using fsdb::FsdbSubscriptionState;

DsfSession::DsfSession(std::string nodeName) : nodeName_(std::move(nodeName)) {}

void DsfSession::localSubStateChanged(fsdb::FsdbSubscriptionState newState) {
  localSubState_ = newState;
  if (localSubState_ == FsdbSubscriptionState::DISCONNECTED) {
    // When our local state is disconnected, also move remote state to
    // disconnected so if local state comes back we don't immediately
    // call the session as ESTABLISHED
    remoteSubState_ = FsdbSubscriptionState::DISCONNECTED;
  }

  auto newSessionState = calculateSessionState(localSubState_, remoteSubState_);
  changeSessionState(newSessionState);
}

void DsfSession::remoteSubStateChanged(fsdb::FsdbSubscriptionState newState) {
  remoteSubState_ = newState;
  auto newSessionState = calculateSessionState(localSubState_, remoteSubState_);
  changeSessionState(newSessionState);
}

void DsfSession::changeSessionState(DsfSessionState newState) {
  if (newState == state_) {
    return;
  }
  XLOG(DBG2) << nodeName_ << ": " << apache::thrift::util::enumNameSafe(state_)
             << " --> " << apache::thrift::util::enumNameSafe(newState);
  state_ = newState;
}

DsfSessionState DsfSession::calculateSessionState(
    FsdbSubscriptionState localState,
    FsdbSubscriptionState remoteState) {
  if (localState != FsdbSubscriptionState::CONNECTED) {
    return DsfSessionState::CONNECT;
  } else if (remoteState == FsdbSubscriptionState::CONNECTED) {
    return DsfSessionState::ESTABLISHED;
  } else {
    return DsfSessionState::WAIT_FOR_REMOTE;
  }
}

DsfSessionThrift DsfSession::toThrift() const {
  DsfSessionThrift ret;
  ret.remoteName() = nodeName_;
  ret.state() = state_;
  ret.lastEstablishedAt().from_optional(lastEstablishedAt_);
  ret.lastDisconnectedAt().from_optional(lastDisconnectedAt_);
  return ret;
}

} // namespace facebook::fboss
