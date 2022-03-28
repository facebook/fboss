// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FsdbStateDeltaConverter.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/gen-cpp2/agent_stats_types.h"
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

  void publishDeltas(std::vector<fsdb::OperDeltaUnit>&& deltas);

  // Paths
  std::vector<std::string> getAgentStatePath() const;
  std::vector<std::string> getAgentStatsPath() const;

  SwSwitch* sw_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::atomic<bool> readyForStatePublishing_{false};
  std::atomic<bool> readyForStatPublishing_{false};
  FsdbStateDeltaConverter deltaConverter_;
};

} // namespace facebook::fboss
