// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <memory>

namespace facebook::fboss {

DsfSubscriber::DsfSubscriber(SwSwitch* sw)
    : sw_(sw),
      fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>("agent")) {}

DsfSubscriber::~DsfSubscriber() {
  stop();
}

void DsfSubscriber::stateUpdated(const StateDelta& /*stateDelta*/) {
  if (!fsdbPubSubMgr_) {
    XLOG(WARN) << " Dsf subscriber already stopped, dropping state update";
  }
  // TODO process state delta
}

void DsfSubscriber::stop() {
  fsdbPubSubMgr_.reset();
}

} // namespace facebook::fboss
