// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/experimental/coro/AsyncGenerator.h>

namespace facebook::fboss::fsdb {
class FsdbDeltaSubscriber : public FsdbStreamClient {
 public:
  FsdbDeltaSubscriber(
      const std::string& clientId,
      const std::vector<std::string>& subscribePath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(clientId, streamEvb, connRetryEvb, stateChangeCb),
        subscribePath_(subscribePath) {}

  ssize_t queueSize() const {
    return publishedQueue_.size();
  }

  OperDelta tryGetNextFor(
      std::chrono::milliseconds ms = std::chrono::milliseconds{10});
  OperDelta getNextBlocking();

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serviceLoop() override;
#endif
  static constexpr auto kPubQueueCapacity{2000};
  folly::DSPMCQueue<OperDelta, true /*may block*/> publishedQueue_{
      kPubQueueCapacity};
  std::vector<std::string> subscribePath_;
};
} // namespace facebook::fboss::fsdb
