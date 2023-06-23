// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/fsdb/common/Utils.h>
#include <fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h>
#include <fboss/thrift_cow/visitors/DeltaVisitor.h>
#include <fboss/thrift_storage/CowStorage.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/thrift_storage/CowStorageMgr.h"

#include <folly/io/async/EventBase.h>
#include <atomic>
#include <memory>

namespace facebook::fboss::fsdb {

template <typename PubRootT>
class FsdbSyncManager {
 public:
  using CowStorageManager = CowStorageMgr<PubRootT>;
  using CowState = typename CowStorageManager::CowState;
  using PubRoot = PubRootT;

  FsdbSyncManager(
      const std::string& clientId,
      const std::vector<std::string>& basePath,
      bool isStats,
      bool publishDeltas)
      : FsdbSyncManager(
            std::make_shared<fsdb::FsdbPubSubManager>(clientId),
            basePath,
            isStats,
            publishDeltas) {}

  FsdbSyncManager(
      const std::shared_ptr<fsdb::FsdbPubSubManager>& pubSubMr,
      const std::vector<std::string>& basePath,
      bool isStats,
      bool publishDeltas)
      : pubSubMgr_(pubSubMr),
        basePath_(basePath),
        isStats_(isStats),
        publishDeltas_(publishDeltas),
        storage_([this](const auto& oldState, const auto& newState) {
          processDelta(oldState, newState);
        }) {
    CHECK(pubSubMr);
    // make sure publisher is not already created for shared pubSubMgr
    if (publishDeltas_) {
      CHECK(!(pubSubMgr_->getDeltaPublisher(isStats)));
    } else {
      CHECK(!(pubSubMgr_->getPathPublisher(isStats)));
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
      if (publishDeltas_) {
        pubSubMgr_->createStatDeltaPublisher(
            basePath_, std::move(stateChangeCb));
      } else {
        pubSubMgr_->createStatPathPublisher(
            basePath_, std::move(stateChangeCb));
      }
    } else if (!isStats_ && FLAGS_publish_state_to_fsdb) {
      if (publishDeltas_) {
        pubSubMgr_->createStateDeltaPublisher(
            basePath_, std::move(stateChangeCb));
      } else {
        pubSubMgr_->createStatePathPublisher(
            basePath_, std::move(stateChangeCb));
      }
    }
  }

  void stop() {
    storage_.getEventBase()->runInEventBaseThreadAndWait([this]() {
      readyForPublishing_.store(false);
      stopInternal();
    });
  }

  //  update internal storage of SyncManager which will then automatically be
  //  queued to be published
  void updateState(typename CowStorageManager::CowStateUpdateFn updateFun) {
    // TODO: might be able to do coalescing before publishing has started
    storage_.updateStateNoCoalescing(
        "Update internal state to publish", updateFun);
  }

  FsdbPubSubManager* pubSubMgr() {
    return pubSubMgr_.get();
  }

  const std::shared_ptr<CowState> getState() const {
    return storage_.getState();
  }

 private:
  void processDelta(
      const std::shared_ptr<CowState>& oldState,
      const std::shared_ptr<CowState>& newState) {
    // TODO: hold lock here to sync with stop()?
    if (readyForPublishing_.load()) {
      if (publishDeltas_) {
        publishDelta(oldState, newState);
      } else {
        publishPath(newState);
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
    if (publishDeltas_) {
      auto deltaUnit = buildOperDeltaUnit(
          basePath_,
          std::shared_ptr<CowState>(),
          currentState,
          OperProtocol::BINARY);
      publish(createDelta({deltaUnit}));
    } else {
      publishPath(currentState);
    }
  }

  OperDelta createDelta(std::vector<OperDeltaUnit>&& deltaUnits) {
    OperDelta delta;
    delta.changes() = deltaUnits;
    delta.protocol() = OperProtocol::BINARY;
    return delta;
  }

  void publishDelta(
      const std::shared_ptr<CowState>& oldState,
      const std::shared_ptr<CowState>& newState) {
    std::vector<OperDeltaUnit> deltas;
    auto processChange = [this, &deltas](
                             const std::vector<std::string>& path,
                             auto oldNode,
                             auto newNode,
                             thrift_cow::DeltaElemTag /* visitTag */) {
      std::vector<std::string> fullPath;
      fullPath.reserve(basePath_.size() + path.size());
      fullPath.insert(fullPath.end(), basePath_.begin(), basePath_.end());
      fullPath.insert(fullPath.end(), path.begin(), path.end());
      // TODO: metadata
      deltas.push_back(
          buildOperDeltaUnit(fullPath, oldNode, newNode, OperProtocol::BINARY));
    };

    thrift_cow::RootDeltaVisitor::visit(
        oldState,
        newState,
        thrift_cow::DeltaVisitOptions(thrift_cow::DeltaVisitMode::MINIMAL),
        std::move(processChange));

    publish(createDelta(std::move(deltas)));
  }

  void publishPath(const std::shared_ptr<CowState>& newState) {
    OperState state;
    state.contents() = newState->encode(OperProtocol::BINARY);
    state.protocol() = fsdb::OperProtocol::BINARY;
    publish(std::move(state));
  }

  template <typename T>
  void publish(T&& state) {
    if (isStats_) {
      pubSubMgr_->publishStat(std::forward<T>(state));
    } else {
      pubSubMgr_->publishState(std::forward<T>(state));
    }
  }

  void stopInternal() {
    if (isStats_ && FLAGS_publish_stats_to_fsdb) {
      if (publishDeltas_) {
        pubSubMgr_->removeStatDeltaPublisher();
      } else {
        pubSubMgr_->removeStatPathPublisher();
      }
    } else if (!isStats_ && FLAGS_publish_state_to_fsdb) {
      if (publishDeltas_) {
        pubSubMgr_->removeStateDeltaPublisher();
      } else {
        pubSubMgr_->removeStatePathPublisher();
      }
    }
    pubSubMgr_.reset();
  }

  std::shared_ptr<FsdbPubSubManager> pubSubMgr_;
  std::vector<std::string> basePath_;
  bool isStats_;
  bool publishDeltas_;
  CowStorageManager storage_;
  std::atomic_bool readyForPublishing_ = false;
};

} // namespace facebook::fboss::fsdb
