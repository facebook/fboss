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

class FsdbComponentSyncer {
 public:
  explicit FsdbComponentSyncer(std::vector<std::string>&& basePath)
      : basePath_(std::move(basePath)) {}
  virtual ~FsdbComponentSyncer() {
    CHECK(!readyForPublishing_.load());
  }

  void publishDelta(OperDelta&& deltas, bool initialSync = false);

  void publishPath(OperState&& data, bool initialSync = false);

  virtual void start() {
    readyForPublishing_.store(true);
  }

  void stop() {
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
  std::atomic<bool> readyForPublishing_{false};
};

template <typename DataT>
class FsdbStateComponentSyncer : public FsdbComponentSyncer {
 public:
  FsdbStateComponentSyncer(
      folly::EventBase* evb,
      std::vector<std::string>&& basePath)
      : FsdbComponentSyncer(std::move(basePath)), evb_(evb) {
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
      evb_->runInEventBaseThreadAndWait([this] { stop(); });
    }
  }

 private:
  folly::EventBase* evb_;
};
} // namespace facebook::fboss::fsdb
