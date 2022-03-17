// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FsdbSyncer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"

namespace facebook::fboss {
FsdbSyncer::FsdbSyncer(SwSwitch* sw)
    : sw_(sw),
      fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>("wedge_agent")) {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createStateDeltaPublisher(
        {"agent"}, [this](auto oldState, auto newState) {
          fsdbConnectionStateChanged(oldState, newState);
        });
  }
  sw_->registerStateObserver(this, "FsdbSyncer");
}

FsdbSyncer::~FsdbSyncer() {
  sw_->unregisterStateObserver(this);
  readyForPublishing_.store(false);
  fsdbPubSubMgr_.reset();
}

void FsdbSyncer::stateUpdated(const StateDelta& /*stateDelta*/) {
  CHECK(sw_->getUpdateEvb()->isInEventBaseThread());
  // Sync updates.
  if (!readyForPublishing_.load()) {
    return;
  }
}

void FsdbSyncer::cfgUpdated(const cfg::SwitchConfig& /*newConfig*/) {
  sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this]() {
    if (!readyForPublishing_.load()) {
      return;
    }
    // sync config
  });
}
void FsdbSyncer::fsdbConnectionStateChanged(
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
