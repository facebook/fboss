// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/fsdb/common/Utils.h>
#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_cow/visitors/PatchBuilder.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/thrift_cow/storage/CowStorageMgr.h"

#include <folly/io/async/EventBase.h>
#include <atomic>
#include <memory>

DECLARE_bool(publish_use_id_paths);

DECLARE_bool(fsdb_publish_state_with_patches);

namespace facebook::fboss::fsdb {

PubSubType getFsdbStatePubType();

template <typename PubRootT, bool EnablePatchAPIs = false>
class FsdbSyncManager {
 public:
  using CowStorageManager = CowStorageMgr<PubRootT>;
  using CowState = typename CowStorageManager::CowState;
  using PubRoot = PubRootT;

  FsdbSyncManager(
      const std::string& clientId,
      const std::vector<std::string>& basePath,
      bool isStats,
      PubSubType pubType,
      std::optional<std::string> clientCounterPrefix = std::nullopt,
      bool useIdPaths = FLAGS_publish_use_id_paths)
      : FsdbSyncManager(
            std::make_shared<fsdb::FsdbPubSubManager>(clientId),
            basePath,
            isStats,
            pubType,
            clientCounterPrefix,
            useIdPaths) {
    XLOG_IF(FATAL, pubType == PubSubType::PATCH && !EnablePatchAPIs)
        << "Patch pub requested but not enabled";
  }

  FsdbSyncManager(
      const std::shared_ptr<fsdb::FsdbPubSubManager>& pubSubMr,
      const std::vector<std::string>& basePath,
      bool isStats,
      PubSubType pubType,
      std::optional<std::string> clientCounterPrefix = std::nullopt,
      bool useIdPaths = FLAGS_publish_use_id_paths)
      : pubSubMgr_(pubSubMr),
        basePath_(basePath),
        isStats_(isStats),
        pubType_(pubType),
        storage_(
            [this](const auto& oldState, const auto& newState) {
              processDelta(oldState, newState);
            },
            clientCounterPrefix),
        useIdPaths_(useIdPaths) {
    CHECK(pubSubMr);
    // make sure publisher is not already created for shared pubSubMgr
    if (pubType_ == PubSubType::DELTA) {
      switch (pubType_) {
        case PubSubType::DELTA:
          CHECK(!(pubSubMgr_->getDeltaPublisher(isStats)));
          break;
        case PubSubType::PATH:
          CHECK(!(pubSubMgr_->getPathPublisher(isStats)));
          break;
        case PubSubType::PATCH:
          CHECK(!(pubSubMgr_->getPatchPublisher(isStats)));
          break;
      }
    }
  }

  ~FsdbSyncManager() {
    CHECK(!pubSubMgr_) << "Syncer not stopped";
    CHECK(!readyForPublishing_.load());
  }

  // Starts publishing to fsdb. This method should only be called once all state
  // components are ready and the SyncManager's internal storage has be
  // populated to a sane state. Initial sync will follow asyncronously after
  // start is called.
  void start() {
    CHECK(pubSubMgr_) << "Syncer already stopped";
    auto stateChangeCb = [this](auto oldState, auto newState) {
      publisherStateChanged(oldState, newState);
    };

    if (isStats_ && FLAGS_publish_stats_to_fsdb) {
      switch (pubType_) {
        case PubSubType::DELTA:
          pubSubMgr_->createStatDeltaPublisher(
              basePath_, std::move(stateChangeCb));
          break;
        case PubSubType::PATH:
          pubSubMgr_->createStatPathPublisher(
              basePath_, std::move(stateChangeCb));
          break;
        case PubSubType::PATCH:
          pubSubMgr_->createStatPatchPublisher(
              basePath_, std::move(stateChangeCb));
          break;
      }
    } else if (!isStats_ && FLAGS_publish_state_to_fsdb) {
      switch (pubType_) {
        case PubSubType::DELTA:
          pubSubMgr_->createStateDeltaPublisher(
              basePath_, std::move(stateChangeCb));
          break;
        case PubSubType::PATH:
          pubSubMgr_->createStatePathPublisher(
              basePath_, std::move(stateChangeCb));
          break;
        case PubSubType::PATCH:
          pubSubMgr_->createStatePatchPublisher(
              basePath_, std::move(stateChangeCb));
          break;
      }
    }
  }

  void stop(bool gracefulStop = false) {
    storage_.getEventBase()->runInEventBaseThreadAndWait(
        [this, gracefulStop]() {
          readyForPublishing_.store(false);
          stopInternal(gracefulStop);
        });
  }

  //  update internal storage of SyncManager which will then automatically be
  //  queued to be published
  void updateState(
      typename CowStorageManager::CowStateUpdateFn updateFun,
      bool printUpdateDelay = false) {
    storage_.updateState(
        "Update internal state to publish", updateFun, printUpdateDelay);
  }

  FsdbPubSubManager* pubSubMgr() {
    return pubSubMgr_.get();
  }

  const std::shared_ptr<CowState> getState() const {
    return storage_.getState();
  }

  uint64_t getPendingUpdatesQueueLength() const {
    return storage_.getPendingUpdatesQueueLength();
  }

 private:
  void processDelta(
      const std::shared_ptr<CowState>& oldState,
      const std::shared_ptr<CowState>& newState) {
    // TODO: hold lock here to sync with stop()?
    if (readyForPublishing_.load()) {
      switch (pubType_) {
        case PubSubType::DELTA:
          publishDelta(oldState, newState);
          break;
        case PubSubType::PATH:
          publishPath(newState);
          break;
        case PubSubType::PATCH:
          publishPatch(oldState, newState);
          break;
      }
    }
  }

  void publisherStateChanged(
      FsdbStreamClient::State /* oldState */,
      FsdbStreamClient::State newState) {
    if (newState == FsdbStreamClient::State::CONNECTED) {
      storage_.getEventBase()->runInEventBaseThreadAndWait([this]() {
        doInitialSync();
        readyForPublishing_.store(true);
      });
    } else {
      // TODO: sync b/w here and processDelta?
      readyForPublishing_.store(false);
    }
  }

  void doInitialSync() {
    CHECK(storage_.getEventBase()->isInEventBaseThread());
    const auto currentState = storage_.getState();
    switch (pubType_) {
      case PubSubType::DELTA:
        publish(createDelta({buildOperDeltaUnit(
            basePath_,
            std::shared_ptr<CowState>(),
            currentState,
            OperProtocol::BINARY)}));
        break;
      case PubSubType::PATH:
        publishPath(currentState);
        break;
      case PubSubType::PATCH:
        XLOG_IF(FATAL, !EnablePatchAPIs) << "Patch APIs not enabled";
        Patch patch;
        thrift_cow::PatchNode root;
        // TODO: switch to binary?
        root.set_val(currentState->encodeBuf(OperProtocol::COMPACT));
        patch.patch() = std::move(root);
        patch.basePath() = basePath_;
        publish(std::move(patch));
        break;
    }
  }

  void publishDelta(
      const std::shared_ptr<CowState>& oldState,
      const std::shared_ptr<CowState>& newState) {
    publish(computeOperDelta(oldState, newState, basePath_, useIdPaths_));
  }

  void publishPath(const std::shared_ptr<CowState>& newState) {
    OperState state;
    state.contents() = newState->encode(OperProtocol::BINARY);
    state.protocol() = fsdb::OperProtocol::BINARY;
    publish(std::move(state));
  }

  void publishPatch(
      const std::shared_ptr<CowState>& oldState,
      const std::shared_ptr<CowState>& newState)
      // conditionally compile expensive patch API for the sake of clients who
      // don't need it
    requires(EnablePatchAPIs)
  {
    publish(
        thrift_cow::PatchBuilder::build(
            oldState,
            newState,
            basePath_,
            fsdb::OperProtocol::COMPACT,
            true /* incrementallyCompress */));
  }

  void publishPatch(
      const std::shared_ptr<CowState>&,
      const std::shared_ptr<CowState>&)
    requires(!EnablePatchAPIs)
  {
    XLOG(FATAL) << "Patch APIs not enabled";
  }

  template <typename T>
  void publish(T&& state) {
    if (isStats_) {
      pubSubMgr_->publishStat(std::forward<T>(state));
    } else {
      pubSubMgr_->publishState(std::forward<T>(state));
    }
  }

  void stopInternal(bool gracefulStop = false) {
    if (isStats_ && FLAGS_publish_stats_to_fsdb) {
      switch (pubType_) {
        case PubSubType::DELTA:
          pubSubMgr_->removeStatDeltaPublisher(gracefulStop);
          break;
        case PubSubType::PATH:
          pubSubMgr_->removeStatPathPublisher(gracefulStop);
          break;
        case PubSubType::PATCH:
          pubSubMgr_->removeStatPatchPublisher(gracefulStop);
          break;
      }
    } else if (!isStats_ && FLAGS_publish_state_to_fsdb) {
      switch (pubType_) {
        case PubSubType::DELTA:
          pubSubMgr_->removeStateDeltaPublisher(gracefulStop);
          break;
        case PubSubType::PATH:
          pubSubMgr_->removeStatePathPublisher(gracefulStop);
          break;
        case PubSubType::PATCH:
          pubSubMgr_->removeStatePatchPublisher(gracefulStop);
          break;
      }
    }
    pubSubMgr_.reset();
  }

  std::shared_ptr<FsdbPubSubManager> pubSubMgr_;
  std::vector<std::string> basePath_;
  bool isStats_;
  PubSubType pubType_;
  CowStorageManager storage_;
  std::atomic_bool readyForPublishing_ = false;
  bool useIdPaths_ = false;
};

} // namespace facebook::fboss::fsdb
