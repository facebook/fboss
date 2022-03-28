// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <memory>

namespace facebook::fboss {
class SwSwitch;
namespace fsdb {
class FsdbPubSubManager;
}
namespace cfg {
class SwitchConfig;
}
class FsdbSyncer : public StateObserver {
 public:
  explicit FsdbSyncer(SwSwitch* sw);
  ~FsdbSyncer() override;
  void stateUpdated(const StateDelta& stateDelta) override;
  void statsUpdated(const AgentStats& stats);

  // TODO - change to AgentConfig once SwSwitch can pass us that
  void cfgUpdated(
      const cfg::SwitchConfig& oldConfig,
      const cfg::SwitchConfig& newConfig);

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_.get();
  }

  void stop();

 private:
  void fsdbStatePublisherStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);
  void fsdbStatPublisherStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);

  template <typename T>
  fsdb::OperDeltaUnit createDeltaUnit(
      const std::vector<std::string>& path,
      const std::optional<T>& oldState,
      const std::optional<T>& newState) const;

  void publishDeltas(std::vector<fsdb::OperDeltaUnit>&& deltas);

  // State delta handlers

  // creates deltas for each node in node map, does not traverse to child nodes
  template <typename MapDelta>
  void processNodeMapDelta(
      std::vector<fsdb::OperDeltaUnit>& operDeltas,
      const MapDelta& nodeMapDelta,
      const std::vector<std::string>& basePath) const;

  template <typename T>
  void processNodeDelta(
      std::vector<fsdb::OperDeltaUnit>& deltas,
      const std::vector<std::string>& basePath,
      const std::string& nodeID,
      const std::shared_ptr<T>& oldNode,
      const std::shared_ptr<T>& newNode) const;

  // Paths
  std::vector<std::string> getAgentStatePath() const;
  std::vector<std::string> getAgentStatsPath() const;
  std::vector<std::string> getSwitchStatePath() const;
  std::vector<std::string> getSwConfigPath() const;
  std::vector<std::string> getPortMapPath() const;

  SwSwitch* sw_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::atomic<bool> readyForStatePublishing_{false};
  std::atomic<bool> readyForStatPublishing_{false};
};

} // namespace facebook::fboss
