// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <memory>

namespace facebook::fboss {
class SwSwitch;
class AgentFsdbSyncManager;

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

  void stop();

  bool isReadyForStatePublishing() const {
    return readyForStatePublishing_.load();
  }

  bool isReadyForStatPublishing() const {
    return readyForStatPublishing_.load();
  }

  fsdb::FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_.get();
  }

 private:
  // Paths
  static std::vector<std::string> getAgentStatePath();
  static std::vector<std::string> getAgentStatsPath();
  static std::vector<std::string> getAgentSwitchStatePath();
  static std::vector<std::string> getAgentSwitchConfigPath();

  void fsdbStatPublisherStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);
  std::optional<std::string> getBitsflowLockdownLevel();

  SwSwitch* sw_;
  std::shared_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::atomic<bool> readyForStatePublishing_{false};
  std::atomic<bool> readyForStatPublishing_{false};
  std::unique_ptr<AgentFsdbSyncManager> agentFsdbSyncManager_;
};

} // namespace facebook::fboss
