// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/FsdbSyncer.h"
#include <optional>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {
FsdbSyncer::FsdbSyncer(SwSwitch* sw)
    : sw_(sw),
      fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>("wedge_agent")) {
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

  std::vector<fsdb::OperDeltaUnit> deltas;

  processNodeMapDelta(deltas, stateDelta.getPortsDelta(), getPortMapPath());

  publishDeltas(std::move(deltas));
}

template <typename MapDelta>
void FsdbSyncer::processNodeMapDelta(
    std::vector<fsdb::OperDeltaUnit>& operDeltas,
    const MapDelta& nodeMapDelta,
    const std::vector<std::string>& basePath) const {
  DeltaFunctions::forEachChanged(
      nodeMapDelta,
      [&](const auto& oldNode, const auto& newNode) {
        processNodeDelta(
            operDeltas,
            basePath,
            folly::to<std::string>(
                MapDelta::MapType::getNodeThriftKey(oldNode)),
            oldNode,
            newNode);
      },
      [&](const auto& newNode) {
        processNodeDelta(
            operDeltas,
            basePath,
            folly::to<std::string>(
                MapDelta::MapType::getNodeThriftKey(newNode)),
            decltype(newNode)(nullptr),
            newNode);
      },
      [&](const auto& oldNode) {
        processNodeDelta(
            operDeltas,
            basePath,
            folly::to<std::string>(
                MapDelta::MapType::getNodeThriftKey(oldNode)),
            oldNode,
            decltype(oldNode)(nullptr));
      });
}

template <typename T>
void FsdbSyncer::processNodeDelta(
    std::vector<fsdb::OperDeltaUnit>& deltas,
    const std::vector<std::string>& basePath,
    const std::string& nodeID,
    const std::shared_ptr<T>& oldNode,
    const std::shared_ptr<T>& newNode) const {
  std::vector<std::string> fullPath = basePath;
  fullPath.push_back(nodeID);
  fsdb::OperDeltaUnit deltaUnit = createDeltaUnit(
      fullPath,
      oldNode ? oldNode->toThrift()
              : std::optional<decltype(oldNode->toThrift())>(),
      newNode ? newNode->toThrift()
              : std::optional<decltype(newNode->toThrift())>());
  deltas.push_back(deltaUnit);
}

void FsdbSyncer::cfgUpdated(
    const cfg::SwitchConfig& oldConfig,
    const cfg::SwitchConfig& newConfig) {
  sw_->getUpdateEvb()->runInEventBaseThreadAndWait([&]() {
    if (!readyForStatePublishing_.load()) {
      return;
    }
    publishDeltas({createDeltaUnit(
        getSwConfigPath(),
        std::make_optional(oldConfig),
        std::make_optional(newConfig))});
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

template <typename T>
fsdb::OperDeltaUnit FsdbSyncer::createDeltaUnit(
    const std::vector<std::string>& path,
    const std::optional<T>& oldState,
    const std::optional<T>& newState) const {
  fsdb::OperPath deltaPath;
  deltaPath.raw_ref() = path;
  fsdb::OperDeltaUnit deltaUnit;
  deltaUnit.path_ref() = deltaPath;
  if (oldState.has_value()) {
    deltaUnit.oldState_ref() =
        apache::thrift::BinarySerializer::serialize<std::string>(
            oldState.value());
  }
  if (newState.has_value()) {
    deltaUnit.newState_ref() =
        apache::thrift::BinarySerializer::serialize<std::string>(
            newState.value());
  }
  return deltaUnit;
}

void FsdbSyncer::publishDeltas(std::vector<fsdb::OperDeltaUnit>&& deltas) {
  fsdb::OperDelta delta;
  delta.changes_ref() = deltas;
  delta.protocol_ref() = fsdb::OperProtocol::BINARY;
  fsdbPubSubMgr_->publishState(delta);
}

void FsdbSyncer::fsdbStatePublisherStateChanged(
    fsdb::FsdbStreamClient::State oldState,
    fsdb::FsdbStreamClient::State newState) {
  CHECK(oldState != newState);
  if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
    // schedule a full sync
    sw_->getUpdateEvb()->runInEventBaseThreadAndWait([this] {
      auto switchStateDelta = createDeltaUnit(
          getSwitchStatePath(),
          std::optional<state::SwitchState>(),
          std::make_optional(sw_->getState()->toThrift()));
      auto configDelta = createDeltaUnit(
          getSwConfigPath(),
          std::optional<cfg::SwitchConfig>(),
          std::make_optional(sw_->getConfig()));
      publishDeltas({switchStateDelta, configDelta});

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
