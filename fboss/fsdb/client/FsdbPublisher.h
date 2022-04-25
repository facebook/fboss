// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <atomic>
#include <shared_mutex>

namespace facebook::fboss::fsdb {
template <typename PubUnit>
class FsdbPublisher : public FsdbStreamClient {
  using QueueT = folly::DMPSCQueue<PubUnit, true /*may block*/>;
  static std::unique_ptr<QueueT> makeQueue() {
    static constexpr auto kPubQueueCapacity{2000};
    return std::make_unique<QueueT>(kPubQueueCapacity);
  }

 public:
  FsdbPublisher(
      const std::string& clientId,
      const std::vector<std::string>& publishPath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      bool publishStats,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(
            clientId,
            streamEvb,
            connRetryEvb,
            [this, stateChangeCb](State oldState, State newState) {
              stateChangeCb(oldState, newState);
              handleStateChange(oldState, newState);
            }),
        toPublishQueue_(makeQueue()),
        publishPath_(publishPath),
        publishStats_(publishStats) {}

  void write(PubUnit pubUnit);

  ssize_t queueSize() const {
    return (*toPublishQueue_.rlock())->size();
  }
  bool publishStats() const {
    return publishStats_;
  }

 protected:
#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<std::optional<PubUnit>> createGenerator();
#endif
  OperPubRequest createRequest() const;

 private:
  void handleStateChange(State oldState, State newState);
  // Note unique_ptr is synchronized, not QueueT. The latter manages its
  // own synchronization
  folly::Synchronized<std::unique_ptr<QueueT>> toPublishQueue_;
  const std::vector<std::string> publishPath_;
  const bool publishStats_;
};
} // namespace facebook::fboss::fsdb
