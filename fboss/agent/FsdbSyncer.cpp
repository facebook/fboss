// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FsdbSyncer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {
FsdbSyncer::FsdbSyncer(SwSwitch* sw)
    : sw_(sw),
      fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>("wedge_agent")) {
  std::vector<std::string> path{"agent"};
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createStateDeltaPublisher(
        path, [this](auto oldState, auto newState) {
          fsdbStatePublisherStateChanged(oldState, newState);
        });
  }
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbPubSubMgr_->createStatPathPublisher(
        path, [this](auto oldState, auto newState) {
          fsdbStatPublisherStateChanged(oldState, newState);
        });
  }
  sw_->registerStateObserver(this, "FsdbSyncer");
}

FsdbSyncer::~FsdbSyncer() {
  sw_->unregisterStateObserver(this);
  readyForStatePublishing_.store(false);
  readyForStatPublishing_.store(false);
  fsdbPubSubMgr_.reset();
}

void FsdbSyncer::stateUpdated(const StateDelta& /*stateDelta*/) {
  CHECK(sw_->getUpdateEvb()->isInEventBaseThread());
  // Sync updates.
  if (!readyForStatePublishing_.load()) {
    return;
  }
}

void FsdbSyncer::cfgUpdated(const cfg::SwitchConfig& /*newConfig*/) {
  sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this]() {
    if (!readyForStatePublishing_.load()) {
      return;
    }
    // sync config
  });
}

void FsdbSyncer::statsUpdated(const AgentStats& stats) {
  if (!readyForStatPublishing_.load()) {
    return;
  }
  fsdb::OperState stateUnit;
  stateUnit.contents_ref() =
      apache::thrift::BinarySerializer::serialize<std::string>(stats);
  stateUnit.protocol_ref() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishStat(std::move(stateUnit));
}

void FsdbSyncer::fsdbStatePublisherStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // schedule a full sync
    sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this] {
      // TODO - do a full state sync on connect
      readyForStatePublishing_.store(true);
    });
  }
  if (newState != fsdb::FsdbStreamClient::State::CONNECTED) {
    // stop publishing
    sw_->getUpdateEvb()->runInEventBaseThreadAndWait(
        [this] { readyForStatePublishing_.store(false); });
  }
}

void FsdbSyncer::fsdbStatPublisherStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // Stats sync at regular intervals, so let the sync
    // happen in that sequence after a connection.
    readyForStatPublishing_.store(true);
  } else {
    readyForStatPublishing_.store(false);
  }
}
} // namespace facebook::fboss
