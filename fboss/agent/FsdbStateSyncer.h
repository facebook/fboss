// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/StateDelta.h"
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
class FsdbStateSyncer : public AutoRegisterStateObserver {
 public:
  explicit FsdbStateSyncer(SwSwitch* sw);
  ~FsdbStateSyncer() override;
  void stateUpdated(const StateDelta& stateDelta) override;
  // TODO - change to AgentConfig once SwSwitch can pass us that
  void cfgUpdated(const cfg::SwitchConfig& /*newConfig*/);

 private:
  void fsdbConnectionStateChanged(
      fsdb::FsdbStreamClient::State oldState,
      fsdb::FsdbStreamClient::State newState);
  SwSwitch* sw_;
  std::unique_ptr<fsdb::FsdbPubSubManager> fsdbPubSubMgr_;
  std::atomic<bool> readyForPublishing_{false};
};

} // namespace facebook::fboss
