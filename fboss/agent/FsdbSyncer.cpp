// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FsdbSyncer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <optional>

namespace facebook::fboss {
FsdbSyncer::FsdbSyncer(SwSwitch* sw)
    : sw_(sw),
      fsdbPubSubMgr_(std::make_shared<fsdb::FsdbPubSubManager>("agent")) {
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createStateDeltaPublisher(
        getAgentStatePath(), [this](auto oldState, auto newState) {
          fsdbStatePublisherStateChanged(oldState, newState);
        });
  }
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbPubSubMgr_->createStatPathPublisher(
        getAgentStatsPath(), [this](auto oldState, auto newState) {
          fsdbStatPublisherStateChanged(oldState, newState);
        });
  }
  sw_->registerStateObserver(this, "FsdbSyncer");
}

FsdbSyncer::~FsdbSyncer() {
  sw_->unregisterStateObserver(this);
  CHECK(!readyForStatePublishing_.load());
  CHECK(!readyForStatPublishing_.load());
}

void FsdbSyncer::stop() {
  // Disable state updates in updateEvb, so this synchronizes
  // with any inflight updates happening in updateEvb
  sw_->getUpdateEvb()->runInEventBaseThreadAndWait(
      [this] { readyForStatePublishing_.store(false); });
  readyForStatPublishing_.store(false);
  fsdbPubSubMgr_.reset();
}

void FsdbSyncer::stateUpdated(const StateDelta& stateDelta) {
  CHECK(sw_->getUpdateEvb()->isInEventBaseThread());
  // Sync updates.
  if (!readyForStatePublishing_.load()) {
    return;
  }

  publishDeltas(deltaConverter_.computeDeltas(stateDelta));
}

void FsdbSyncer::cfgUpdated(
    const cfg::SwitchConfig& oldConfig,
    const cfg::SwitchConfig& newConfig) {
  sw_->getUpdateEvb()->runInEventBaseThreadAndWait([&]() {
    if (!readyForStatePublishing_.load()) {
      return;
    }

    publishDeltas({deltaConverter_.createConfigDelta(
        std::make_optional(oldConfig), std::make_optional(newConfig))});
  });
}

void FsdbSyncer::statsUpdated(const AgentStats& stats) {
  if (!readyForStatPublishing_.load()) {
    return;
  }
  fsdb::OperState stateUnit;
  stateUnit.contents() =
      apache::thrift::BinarySerializer::serialize<std::string>(stats);
  stateUnit.protocol() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishStat(std::move(stateUnit));
}

void FsdbSyncer::publishDeltas(std::vector<fsdb::OperDeltaUnit>&& deltas) {
  fsdb::OperDelta delta;
  delta.changes() = std::move(deltas);
  delta.protocol() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishState(std::move(delta));
}

void FsdbSyncer::fsdbStatePublisherStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // schedule a full sync
    sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this] {
      auto switchStateDelta = deltaConverter_.createSwitchStateDelta(
          std::optional<state::SwitchState>(),
          std::make_optional(sw_->getState()->toThrift()));
      auto configDelta = deltaConverter_.createConfigDelta(
          std::optional<cfg::SwitchConfig>(),
          std::make_optional(sw_->getConfig()));
      auto bitsflowDelta = deltaConverter_.createBitsflowLockdownLevelDelta(
          std::optional<std::string>(), getBitsflowLockdownLevel());
      publishDeltas({switchStateDelta, configDelta, bitsflowDelta});

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
