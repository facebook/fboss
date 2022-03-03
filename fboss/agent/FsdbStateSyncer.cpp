// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FsdbStateSyncer.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

namespace facebook::fboss {
FsdbStateSyncer::FsdbStateSyncer(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "FsdbStateSyncer"),
      sw_(sw),
      fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>("wedge_agent")) {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createDeltaPublisher(
        {"agent"}, [](auto /*oldState*/, auto /*newState*/) {});
  }
}

FsdbStateSyncer::~FsdbStateSyncer() {}

void FsdbStateSyncer::stateUpdated(const StateDelta& /*stateDelta*/) {
  // Sync updates.
}

void FsdbStateSyncer::cfgUpdated(const cfg::SwitchConfig& /*newConfig*/) {
  // TODO
}
void FsdbStateSyncer::fsdbConnectionStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // schedule a full sync
  }
  if (newState != fsdb::FsdbStreamClient::State::CONNECTED) {
    // stop publishing
  }
}
} // namespace facebook::fboss
