// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <folly/io/async/EventBase.h>
#include <memory>

namespace facebook::fboss::fsdb {

class FsdbBaseComponentSyncer;

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

  void registerStateSyncer(FsdbBaseComponentSyncer* syncer);
  void registerStatsSyncer(FsdbBaseComponentSyncer* syncer);

 private:
  void statePublisherStateChanged(
      FsdbStreamClient::State oldState,
      FsdbStreamClient::State newState);

  void statsPublisherStateChanged(
      FsdbStreamClient::State oldState,
      FsdbStreamClient::State newState);

  std::unique_ptr<FsdbPubSubManager> fsdbPubSubMgr_;
  std::vector<FsdbBaseComponentSyncer*> stateSyncers_;
  std::vector<FsdbBaseComponentSyncer*> statSyncers_;
  std::vector<std::string> statePath_;
  std::vector<std::string> statsPath_;
  bool started = false;
};

} // namespace facebook::fboss::fsdb
