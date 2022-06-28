// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbSyncManager.h"
#include "fboss/fsdb/client/FsdbComponentSyncer.h"

namespace facebook::fboss::fsdb {

FsdbSyncManager::FsdbSyncManager(
    const std::string& clientId,
    const std::vector<std::string>& statePath,
    const std::vector<std::string>& statsPath)
    : fsdbPubSubMgr_(std::make_unique<fsdb::FsdbPubSubManager>(clientId)),
      statePath_(statePath),
      statsPath_(statsPath) {}

FsdbSyncManager::~FsdbSyncManager() {
  CHECK(!started);
}

void FsdbSyncManager::start() {
  started = true;
  if (FLAGS_publish_state_to_fsdb) {
    fsdbPubSubMgr_->createStateDeltaPublisher(
        statePath_, [this](auto oldState, auto newState) {
          statePublisherStateChanged(oldState, newState);
        });
  }
  if (FLAGS_publish_stats_to_fsdb) {
    fsdbPubSubMgr_->createStatPathPublisher(
        statsPath_, [this](auto oldState, auto newState) {
          statsPublisherStateChanged(oldState, newState);
        });
  }
}

void FsdbSyncManager::stop() {
  for (const auto& updater : stateSyncers_) {
    updater->stop();
  }
  for (const auto& updater : statSyncers_) {
    updater->stop();
  }
  fsdbPubSubMgr_.reset();
  started = false;
}

void FsdbSyncManager::registerStateSyncer(FsdbComponentSyncer* syncer) {
  CHECK(!started);
  stateSyncers_.push_back(syncer);
  syncer->registerSyncManager(this);
}

void FsdbSyncManager::registerStatsSyncer(FsdbComponentSyncer* syncer) {
  CHECK(!started);
  statSyncers_.push_back(syncer);
  syncer->registerSyncManager(this);
}

void FsdbSyncManager::statePublisherStateChanged(
    FsdbStreamClient::State oldState,
    FsdbStreamClient::State newState) {
  for (const auto& syncer : stateSyncers_) {
    syncer->publisherStateChanged(oldState, newState);
  }
}

void FsdbSyncManager::statsPublisherStateChanged(
    FsdbStreamClient::State oldState,
    FsdbStreamClient::State newState) {
  for (const auto& syncer : statSyncers_) {
    syncer->publisherStateChanged(oldState, newState);
  }
}

} // namespace facebook::fboss::fsdb
