// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include "fboss/fsdb/client/FsdbStreamClient.h"

namespace facebook::fboss::fsdb {
template <typename PubUnit>
class FsdbPublisher : public FsdbStreamClient {
 public:
  FsdbPublisher(
      const std::string& clientId,
      const std::vector<std::string>& publishPath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(clientId, streamEvb, connRetryEvb),
        publishPath_(publishPath) {}

  void write(const PubUnit& pubUnit);

  ssize_t queueSize() const {
    return toPublishQueue_.size();
  }

 protected:
#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<PubUnit> createGenerator();
#endif
  const std::vector<std::string> publishPath_;

 private:
  static constexpr auto kPubQueueCapacity{2000};
  folly::DMPSCQueue<PubUnit, true /*may block*/> toPublishQueue_{
      kPubQueueCapacity};
};
} // namespace facebook::fboss::fsdb
