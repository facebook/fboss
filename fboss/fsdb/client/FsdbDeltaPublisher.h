// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/experimental/coro/AsyncGenerator.h>

namespace facebook::fboss::fsdb {
class FsdbDeltaPublisher : public FsdbStreamClient {
 public:
  FsdbDeltaPublisher(
      const std::string& clientId,
      const std::vector<std::string>& publishPath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(clientId, streamEvb, connRetryEvb),
        publishPath_(publishPath) {}

  void write(OperDelta pubUnit);

  ssize_t queueSize() const {
    return toPublishQueue_.size();
  }

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<OperDelta> createGenerator();
  folly::coro::Task<void> serviceLoop() override;
#endif
  static constexpr auto kPubQueueCapacity{2000};
  folly::DMPSCQueue<OperDelta, true /*may block*/> toPublishQueue_{
      kPubQueueCapacity};
  std::vector<std::string> publishPath_;
};
} // namespace facebook::fboss::fsdb
