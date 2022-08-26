// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <memory>
#include <type_traits>

namespace facebook::fboss::fsdb {

class FsdbPubSubManager;
class FsdbSyncManager;

class FsdbBaseComponentSyncer {
 public:
  FsdbBaseComponentSyncer(std::vector<std::string>&& basePath, bool isStats)
      : basePath_(std::move(basePath)), isStats_(isStats) {}

  virtual ~FsdbBaseComponentSyncer() {
    CHECK(!readyForPublishing_.load());
  }

  void publishDelta(OperDelta&& deltas, bool initialSync = false);

  void publishPath(OperState&& data, bool initialSync = false);

  void registerPubSubMgr(FsdbPubSubManager* pubSubManager) {
    pubSubManager_ = pubSubManager;
  }

  virtual void start() {
    readyForPublishing_.store(true);
  }

  virtual void stop() {
    readyForPublishing_.store(false);
  }

  bool isReady() {
    return readyForPublishing_.load();
  }

  OperDelta createDelta(std::vector<OperDeltaUnit>&& deltas) const {
    OperDelta delta;
    delta.changes() = deltas;
    delta.protocol() = OperProtocol::BINARY;
    return delta;
  }

  template <typename Path, typename Node>
  OperDeltaUnit createDeltaUnit(
      const Path& path,
      const std::optional<Node>& oldState,
      const std::optional<Node>& newState) const {
    static_assert(std::is_same_v<typename Path::DataT, Node>);
    return createDeltaUnit(path.tokens(), oldState, newState);
  }

  template <typename Node>
  OperDeltaUnit createDeltaUnit(
      const std::vector<std::string>& path,
      const std::optional<Node>& oldState,
      const std::optional<Node>& newState) const {
    fsdb::OperPath deltaPath;
    deltaPath.raw() = path;
    OperDeltaUnit deltaUnit;
    deltaUnit.path() = deltaPath;
    if (oldState.has_value()) {
      deltaUnit.oldState() =
          apache::thrift::BinarySerializer::serialize<std::string>(
              oldState.value());
    }
    if (newState.has_value()) {
      deltaUnit.newState() =
          apache::thrift::BinarySerializer::serialize<std::string>(
              newState.value());
    }
    return deltaUnit;
  }

  const std::vector<std::string>& getBasePath() {
    return basePath_;
  }

  virtual void publisherStateChanged(
      FsdbStreamClient::State oldState,
      FsdbStreamClient::State newState) = 0;

 private:
  std::vector<std::string> basePath_;
  bool isStats_;
  FsdbPubSubManager* pubSubManager_;
  std::atomic<bool> readyForPublishing_{false};
};

template <typename DataT>
class FsdbStateComponentSyncer : public FsdbBaseComponentSyncer {
 public:
  FsdbStateComponentSyncer(
      folly::EventBase* evb,
      std::vector<std::string>&& basePath)
      : FsdbBaseComponentSyncer(std::move(basePath), false /* isStats */),
        evb_(evb) {
    CHECK(evb);
  }

  virtual DataT getCurrentState() = 0;

  void publisherStateChanged(
      FsdbStreamClient::State oldState,
      FsdbStreamClient::State newState) override {
    CHECK(oldState != newState);
    if (newState == FsdbStreamClient::State::CONNECTED) {
      // schedule a full sync
      evb_->runInEventBaseThreadAndWait([this] {
        auto deltaUnit = createDeltaUnit(
            getBasePath(),
            std::optional<DataT>(),
            std::optional<DataT>(getCurrentState()));
        publishDelta(createDelta({deltaUnit}), true /* initialSync */);
        start();
      });
    } else if (newState != FsdbStreamClient::State::CONNECTED) {
      // stop publishing
      stop();
    }
  }

  void stop() override {
    // for state we need to make sure stop happens on our update evb otherwise
    // we may race shutdown with a currently running update
    evb_->runInEventBaseThreadAndWait(
        [this] { FsdbBaseComponentSyncer::stop(); });
  }

 private:
  folly::EventBase* evb_;
};

class FsdbStatsComponentSyncer : public FsdbBaseComponentSyncer {
 public:
  explicit FsdbStatsComponentSyncer(std::vector<std::string>&& basePath)
      : FsdbBaseComponentSyncer(std::move(basePath), true /* isStats */) {}

  void publisherStateChanged(
      FsdbStreamClient::State oldState,
      FsdbStreamClient::State newState) override;
};
} // namespace facebook::fboss::fsdb
