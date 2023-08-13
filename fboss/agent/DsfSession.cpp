// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSession.h"

namespace facebook::fboss {

DsfSession::DsfSession(std::string nodeName) : nodeName_(std::move(nodeName)) {}

void DsfSession::localSubStateChanged(fsdb::FsdbSubscriptionState newState) {
  localSubState_ = newState;
  // TODO: fill in state machine logic here
}
void DsfSession::remoteSubStateChanged(fsdb::FsdbSubscriptionState newState) {
  remoteSubState_ = newState;
  // TODO: fill in state machine logic here
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
