// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include "fboss/fsdb/if/FsdbModel.h"

#include "fboss/fsdb/client/FsdbPubSubManager.h"

namespace facebook::fboss::fsdb::test {

template <typename Path>
class TestSubscription {
 public:
  using LockedData = folly::Synchronized<std::optional<typename Path::DataT>>;

  explicit TestSubscription(
      FsdbPubSubManager& pubSubManager,
      Path path,
      bool isStats)
      : pubSubManager_(pubSubManager),
        path_(path.tokens()),
        pathStr_(path.str()) {
    auto clientStateChange =
        [](facebook::fboss::fsdb::SubscriptionState /* old */,
           facebook::fboss::fsdb::SubscriptionState /* new */,
           std::optional<bool> /*initialSyncHasData*/) {};
    auto dataUpdate = [&](facebook::fboss::fsdb::OperState state) {
      if (auto contents = state.contents()) {
        auto obj =
            apache::thrift::BinarySerializer::deserialize<typename Path::DataT>(
                *contents);
        *data_.wlock() = obj;
      } else {
        *data_.wlock() = std::nullopt;
      }
    };
    if (isStats) {
      pubSubManager_.addStatPathSubscription(
          path_, clientStateChange, dataUpdate);
    } else {
      pubSubManager_.addStatePathSubscription(
          path_, clientStateChange, dataUpdate);
    }
  }

  ~TestSubscription() {
    pubSubManager_.removeStatePathSubscription(path_);
  }

  const typename LockedData::ConstLockedPtr rlock() const {
    return data_.rlock();
  }

 private:
  facebook::fboss::fsdb::FsdbPubSubManager& pubSubManager_;
  folly::Synchronized<std::optional<typename Path::DataT>> data_;
  std::vector<std::string> path_;
  std::string pathStr_;
};

class FsdbTestSubscriber {
 public:
  FsdbTestSubscriber(const std::string& clientId) : pubSubMgr_(clientId) {}

  template <typename Path>
  TestSubscription<Path> subscribe(Path path, bool isStats = false) {
    return TestSubscription<Path>(pubSubMgr_, path, isStats);
  }

  const thriftpath::RootThriftPath<FsdbOperStateRoot>& getRootStatePath()
      const {
    return rootPath_;
  }

 private:
  FsdbPubSubManager pubSubMgr_;
  thriftpath::RootThriftPath<FsdbOperStateRoot> rootPath_;
};

} // namespace facebook::fboss::fsdb::test
