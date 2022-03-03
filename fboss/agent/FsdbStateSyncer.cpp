// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FsdbStateSyncer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

namespace facebook::fboss {
FsdbStateSyncer::FsdbStateSyncer(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "FsdbStateSyncer"),
      sw_(sw),
      fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>("wedge_agent")) {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createDeltaPublisher(
        {"agent"}, [this](auto oldState, auto newState) {
          fsdbConnectionStateChanged(oldState, newState);
        });
  }
}

FsdbStateSyncer::~FsdbStateSyncer() {
  readyForPublishing_.store(false);
  fsdbPubSubMgr_.reset();
}

void FsdbStateSyncer::stateUpdated(const StateDelta& /*stateDelta*/) {
  CHECK(sw_->getUpdateEvb()->isInEventBaseThread());
  // Sync updates.
  if (!readyForPublishing_.load()) {
    return;
  }
}

void FsdbStateSyncer::cfgUpdated(const cfg::SwitchConfig& /*newConfig*/) {
  sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this]() {
    if (!readyForPublishing_.load()) {
      return;
    }
    // sync config
  });
}
void FsdbStateSyncer::fsdbConnectionStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // schedule a full sync
    sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this] {
      // TODO - do a full state sync on connect
      readyForPublishing_.store(true);
    });
  }
  if (newState != fsdb::FsdbStreamClient::State::CONNECTED) {
    // stop publishing
    sw_->getUpdateEvb()->runInEventBaseThreadAndWait(
        [this] { readyForPublishing_.store(false); });
  }
}
} // namespace facebook::fboss
