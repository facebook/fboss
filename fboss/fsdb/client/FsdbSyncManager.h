// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <folly/io/async/EventBase.h>
#include <memory>

namespace facebook::fboss::fsdb {

class FsdbComponentSyncer;

class FsdbSyncManager {
 public:
  FsdbSyncManager(
      const std::string& clientId,
      const std::vector<std::string>& statePath,
      const std::vector<std::string>& statsPath);

  ~FsdbSyncManager();

  FsdbPubSubManager* pubSubMgr() {
    return fsdbPubSubMgr_.get();
  }

  void start();
  void stop();

  void registerStateSyncer(FsdbComponentSyncer* syncer);

  // Convinence publish apis
  void publishState(OperDelta&& pubUnit) {
    fsdbPubSubMgr_->publishState(std::move(pubUnit));
  }
  void publishState(OperState&& pubUnit) {
    fsdbPubSubMgr_->publishState(std::move(pubUnit));
  }

 private:
  void statePublisherStateChanged(
      FsdbStreamClient::State oldState,
      FsdbStreamClient::State newState);

  std::unique_ptr<FsdbPubSubManager> fsdbPubSubMgr_;
  std::vector<FsdbComponentSyncer*> stateSyncers_;
  std::vector<std::string> statePath_;
  std::vector<std::string> statsPath_;
  bool started = false;
};

} // namespace facebook::fboss::fsdb
